#include "monitorwidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QMenu>
#include <QScreen>
#include <QGuiApplication>
#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QTextStream>
#include <QStandardPaths>
#include <QRegularExpression>
#include <QResizeEvent>
#include <cmath>
#include <cstring>
#include <algorithm>

/* DRM headers for connector enumeration */
extern "C" {
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
}

/* ── DRM connector type → wlroots name prefix ────────────────────── */

static const char *connectorTypeName(uint32_t type)
{
    switch (type) {
    case DRM_MODE_CONNECTOR_DisplayPort: return "DP";
    case DRM_MODE_CONNECTOR_HDMIA:      return "HDMI-A";
    case DRM_MODE_CONNECTOR_HDMIB:      return "HDMI-B";
    case DRM_MODE_CONNECTOR_eDP:        return "eDP";
    case DRM_MODE_CONNECTOR_VGA:        return "VGA";
    case DRM_MODE_CONNECTOR_DVII:       return "DVI-I";
    case DRM_MODE_CONNECTOR_DVID:       return "DVI-D";
    case DRM_MODE_CONNECTOR_DVIA:       return "DVI-A";
    case DRM_MODE_CONNECTOR_LVDS:       return "LVDS";
    case DRM_MODE_CONNECTOR_DSI:        return "DSI";
    case DRM_MODE_CONNECTOR_DPI:        return "DPI";
    default:                            return "Unknown";
    }
}

/* ── PNP manufacturer ID → brand name ────────────────────────────── */

static const char *pnpToMake(const char *pnp)
{
    static const struct { const char *id; const char *name; } table[] = {
        {"SAM", "Samsung"}, {"SEC", "Samsung"}, {"SDC", "Samsung"},
        {"HWP", "HP"},      {"HPN", "HP"},
        {"DEL", "Dell"},
        {"ACI", "ASUS"},    {"AUS", "ASUS"},
        {"BNQ", "BenQ"},
        {"ACR", "Acer"},
        {"LEN", "Lenovo"},
        {"VSC", "ViewSonic"},
        {"AOC", "AOC"},
        {"GSM", "LG"},      {"LGD", "LG"},
        {"PHL", "Philips"},
        {"NEC", "NEC"},
        {"EIZ", "EIZO"},
        {"IVM", "Iiyama"},
        {"MSI", "MSI"},
        {"GBT", "Gigabyte"},
        {"WAC", "Wacom"},
        {"APP", "Apple"},
        {"CMN", "Innolux"},
        {"BOE", "BOE"},
        {"AUO", "AU Optronics"},
        {"SHP", "Sharp"},
        {"SNY", "Sony"},
        {"MEI", "Panasonic"},
        {"TSB", "Toshiba"},
        {"HSD", "HannStar"},
        {"MED", "Medion"},
    };
    for (auto &t : table)
        if (strcmp(pnp, t.id) == 0)
            return t.name;
    return nullptr;
}

/* ── Config path helper ──────────────────────────────────────────── */

static QString monitorsConfPath()
{
    QString home = QDir::homePath();
    return home + "/.local/nixlyos/monitors.conf";
}

/* ── Overlay file path (nixlytile reads via inotify) ─────────────── */

static QString overlayFilePath()
{
    return QDir::homePath() + "/.local/nixlyos/monitor_overlay";
}

/* ── Constructor ─────────────────────────────────────────────────── */

MonitorSetupWidget::MonitorSetupWidget(QWidget *parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setMinimumSize(400, 300);

    m_animTimer.setTimerType(Qt::PreciseTimer);
    connect(&m_animTimer, &QTimer::timeout, this, [this]() { animationStep(); });

    enumerateMonitors();
    m_configExists = QFile::exists(monitorsConfPath());
    loadConfig();
    compactGrid();
    computeBoxLayout();
    snapshotSavedState();

    /* Snap animation to targets on init */
    for (auto &e : m_entries) {
        e.anim_x = e.target_x;
        e.anim_y = e.target_y;
        e.anim_w = e.target_w;
        e.anim_h = e.target_h;
    }
}

/* ── DRM enumeration ─────────────────────────────────────────────── */

void MonitorSetupWidget::enumerateMonitors()
{
    m_entries.clear();

    /* Try each /dev/dri/card* */
    DIR *dir = opendir("/dev/dri");
    if (!dir)
        return;

    struct dirent *ent;
    while ((ent = readdir(dir)) != nullptr) {
        if (strncmp(ent->d_name, "card", 4) != 0)
            continue;
        /* Skip render nodes */
        if (strstr(ent->d_name, "render"))
            continue;

        char path[256];
        snprintf(path, sizeof(path), "/dev/dri/%s", ent->d_name);

        int fd = open(path, O_RDONLY | O_CLOEXEC);
        if (fd < 0)
            continue;

        drmModeRes *res = drmModeGetResources(fd);
        if (!res) {
            ::close(fd);
            continue;
        }

        for (int i = 0; i < res->count_connectors; i++) {
            /* GetConnectorCurrent uses the kernel's cached state instead of
             * forcing a re-probe, which can block for seconds per connector. */
            drmModeConnector *conn = drmModeGetConnectorCurrent(fd, res->connectors[i]);
            if (!conn)
                continue;
            if (conn->connection == DRM_MODE_CONNECTED && conn->count_modes == 0) {
                /* Cached state has no modes; do a full probe for this one. */
                drmModeFreeConnector(conn);
                conn = drmModeGetConnector(fd, res->connectors[i]);
                if (!conn)
                    continue;
            }

            if (conn->connection == DRM_MODE_CONNECTED && conn->count_modes > 0) {
                MonitorEntry entry;
                const char *prefix = connectorTypeName(conn->connector_type);
                char name[64];
                snprintf(name, sizeof(name), "%s-%d", prefix, conn->connector_type_id);
                entry.name = name;

                /* Read manufacturer from EDID */
                for (int p = 0; p < conn->count_props; p++) {
                    drmModePropertyPtr prop = drmModeGetProperty(fd, conn->props[p]);
                    if (!prop) continue;
                    if (strcmp(prop->name, "EDID") == 0 && (prop->flags & DRM_MODE_PROP_BLOB)) {
                        drmModePropertyBlobPtr blob = drmModeGetPropertyBlob(fd, conn->prop_values[p]);
                        if (blob && blob->length >= 16) {
                            auto *edid = (const uint8_t *)blob->data;
                            uint16_t mfg = ((uint16_t)edid[8] << 8) | edid[9];
                            char pnp[4];
                            pnp[0] = 'A' + (char)((mfg >> 10) & 0x1F) - 1;
                            pnp[1] = 'A' + (char)((mfg >> 5) & 0x1F) - 1;
                            pnp[2] = 'A' + (char)(mfg & 0x1F) - 1;
                            pnp[3] = '\0';
                            const char *brand = pnpToMake(pnp);
                            if (brand)
                                entry.make = brand;
                            else
                                entry.make = pnp;  /* fallback to raw PNP ID */
                        }
                        if (blob) drmModeFreePropertyBlob(blob);
                    }
                    drmModeFreeProperty(prop);
                }

                /* Current mode = first preferred, or first available */
                drmModeModeInfo *best = &conn->modes[0];
                for (int j = 0; j < conn->count_modes; j++) {
                    if (conn->modes[j].type & DRM_MODE_TYPE_PREFERRED) {
                        best = &conn->modes[j];
                        break;
                    }
                }
                entry.width = (int)best->hdisplay;
                entry.height = (int)best->vdisplay;
                entry.refresh = (float)best->vrefresh;

                /* Collect all modes, dedup */
                for (int j = 0; j < conn->count_modes; j++) {
                    drmModeModeInfo *m = &conn->modes[j];
                    int mw = (int)m->hdisplay;
                    int mh = (int)m->vdisplay;
                    int mr = (int)(m->vrefresh * 1000);

                    bool dup = false;
                    for (auto &em : entry.modes) {
                        if (em.width == mw && em.height == mh && em.refresh_mhz == mr) {
                            dup = true;
                            break;
                        }
                    }
                    if (!dup)
                        entry.modes.push_back({mw, mh, mr});
                }

                entry.grid_col = (int)m_entries.size();
                entry.grid_row = 0;
                entry.transform = 0;

                m_entries.push_back(std::move(entry));
            }
            drmModeFreeConnector(conn);
        }
        drmModeFreeResources(res);
        ::close(fd);
    }
    closedir(dir);

    /* Try to match with QScreen to get current resolution/position */
    for (auto &e : m_entries) {
        for (auto *screen : QGuiApplication::screens()) {
            if (screen->name().toStdString() == e.name) {
                QSize sz = screen->size();
                e.width = sz.width();
                e.height = sz.height();
                e.refresh = (float)screen->refreshRate();
                break;
            }
        }
    }
}

/* ── Config loading ──────────────────────────────────────────────── */

void MonitorSetupWidget::loadConfig()
{
    QString path = monitorsConfPath();
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#'))
            continue;

        /* Parse: monitor = NAME grid=C,R WxH@Hz [transform=...] */
        int eq = line.indexOf('=');
        if (eq < 0) continue;

        QString key = line.left(eq).trimmed();
        QString value = line.mid(eq + 1).trimmed();

        if (key != "monitor")
            continue;

        QStringList parts = value.split(QRegularExpression("\\s+"));
        if (parts.size() < 2)
            continue;

        QString name = parts[0];

        /* Find matching entry */
        MonitorEntry *found = nullptr;
        for (auto &e : m_entries) {
            if (QString::fromStdString(e.name) == name) {
                found = &e;
                break;
            }
        }
        if (!found)
            continue;

        for (int i = 1; i < parts.size(); i++) {
            QString tok = parts[i];

            /* grid=C,R */
            if (tok.startsWith("grid=")) {
                QString gr = tok.mid(5);
                QStringList cr = gr.split(',');
                if (cr.size() == 2) {
                    found->grid_col = cr[0].toInt();
                    found->grid_row = cr[1].toInt();
                }
            }
            /* WxH@Hz */
            else if (tok.contains('x') && tok.contains('@')) {
                int xi = tok.indexOf('x');
                int ai = tok.indexOf('@');
                found->width = tok.left(xi).toInt();
                found->height = tok.mid(xi + 1, ai - xi - 1).toInt();
                found->refresh = tok.mid(ai + 1).toFloat();
            }
            /* WxH */
            else if (tok.contains('x') && !tok.contains('=')) {
                int xi = tok.indexOf('x');
                found->width = tok.left(xi).toInt();
                found->height = tok.mid(xi + 1).toInt();
            }
            /* transform=... */
            else if (tok.startsWith("transform=")) {
                QString t = tok.mid(10);
                if (t == "rotate-90") {
                    found->transform = 1;
                } else if (t == "rotate-270") {
                    found->transform = 3;
                }
            }
        }
    }

    /* Ensure no duplicate grid cells */
    for (int i = 0; i < (int)m_entries.size(); i++) {
        for (int j = i + 1; j < (int)m_entries.size(); j++) {
            if (m_entries[i].grid_col == m_entries[j].grid_col &&
                m_entries[i].grid_row == m_entries[j].grid_row) {
                int mc = 0;
                for (auto &e : m_entries)
                    if (e.grid_col > mc) mc = e.grid_col;
                m_entries[j].grid_col = mc + 1;
            }
        }
    }
}

/* ── Config writing (matches write_monitors_conf) ────────────────── */

void MonitorSetupWidget::writeConfig()
{
    QString path = monitorsConfPath();
    QDir().mkpath(QFileInfo(path).absolutePath());

    /* Atomic write (temp + rename): nixlytile watches this file with
     * inotify and must never read a half-written config. */
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    QTextStream out(&file);
    out << "# Auto-generated by nixlytile monitor setup\n";

    for (auto &e : m_entries) {
        out << "monitor = " << QString::fromStdString(e.name)
            << " grid=" << e.grid_col << "," << e.grid_row
            << " " << e.width << "x" << e.height
            << "@" << (int)e.refresh;

        if (e.transform == 1)
            out << " transform=rotate-90";
        else if (e.transform == 3)
            out << " transform=rotate-270";

        out << "\n";
    }

    out.flush();
    if (!file.commit())
        return;

    m_configExists = true;
    snapshotSavedState();
    update();
}

/* ── Grid compaction (matches compact_grid) ──────────────────────── */

void MonitorSetupWidget::compactGrid()
{
    int n = (int)m_entries.size();
    if (n <= 0)
        return;

    /* Normalize: ensure minimum col and row are 0 */
    int min_col = m_entries[0].grid_col;
    int min_row = m_entries[0].grid_row;
    for (int i = 1; i < n; i++) {
        if (m_entries[i].grid_col < min_col) min_col = m_entries[i].grid_col;
        if (m_entries[i].grid_row < min_row) min_row = m_entries[i].grid_row;
    }
    if (min_col != 0 || min_row != 0) {
        for (auto &e : m_entries) {
            e.grid_col -= min_col;
            e.grid_row -= min_row;
        }
    }

    /* Find grid extents */
    int max_col = 0, max_row = 0;
    for (auto &e : m_entries) {
        if (e.grid_col > max_col) max_col = e.grid_col;
        if (e.grid_row > max_row) max_row = e.grid_row;
    }

    /* Remove empty columns */
    for (int c = max_col; c >= 0; c--) {
        bool used = false;
        for (auto &e : m_entries) {
            if (e.grid_col == c) { used = true; break; }
        }
        if (!used) {
            for (auto &e : m_entries)
                if (e.grid_col > c) e.grid_col--;
        }
    }

    /* Remove empty rows */
    for (int r = max_row; r >= 0; r--) {
        bool used = false;
        for (auto &e : m_entries) {
            if (e.grid_row == r) { used = true; break; }
        }
        if (!used) {
            for (auto &e : m_entries)
                if (e.grid_row > r) e.grid_row--;
        }
    }
}

/* ── Box layout (matches compute_box_layout) ─────────────────────── */

void MonitorSetupWidget::computeBoxLayout()
{
    int n = (int)m_entries.size();
    if (n <= 0)
        return;

    int popup_w = width();
    int popup_h = height();

    /* The reserved drop columns (see below) provide the gap to the widget
     * edges, so padding only needs to be a small margin. */
    int padding = 20;
    int spacing = std::max(16, popup_w / 40);
    m_spacing = spacing;
    int button_area_h = 50;
    int avail_w = popup_w - 2 * padding;
    int avail_h = popup_h - button_area_h - 2 * padding;

    int max_col = 0, max_row = 0;
    for (auto &e : m_entries) {
        if (e.grid_col > max_col) max_col = e.grid_col;
        if (e.grid_row > max_row) max_row = e.grid_row;
    }

    m_gridCols = max_col + 1;
    m_gridRows = max_row + 1;

    /* Cell width is derived from the total width so 1.5 empty cells fit on
     * each side of the grid: a grabbed box always has more than a full cell
     * of room to be dropped left of the first and right of the last box. */
    float slot_cols = (float)m_gridCols + 3.0f;
    int cell_w = (int)((avail_w - (m_gridCols + 1) * spacing) / slot_cols);
    if (cell_w > 340) cell_w = 340;
    if (cell_w < 60) cell_w = 60;

    /* Height follows the width so boxes keep a monitor-like shape, then is
     * capped by what the available height can actually fit. */
    int cell_h = (avail_h - (m_gridRows - 1) * spacing) / m_gridRows;
    int cell_h_from_w = (int)(cell_w * 0.62f);
    if (cell_h > cell_h_from_w) cell_h = cell_h_from_w;
    if (cell_h < 50) cell_h = 50;

    m_cellW = cell_w;
    m_cellH = cell_h;

    int land_w = cell_w - 8;
    int land_h = cell_h - 8;
    int port_w = (int)(land_w * 0.52f);
    int port_h = land_h;

    int grid_w = m_gridCols * cell_w + (m_gridCols - 1) * spacing;
    int grid_h = m_gridRows * cell_h + (m_gridRows - 1) * spacing;

    int origin_x = padding + (avail_w - grid_w) / 2;
    int origin_y = padding + (avail_h - grid_h) / 2;

    m_gridOriginX = origin_x;
    m_gridOriginY = origin_y;

    for (auto &e : m_entries) {
        int cx = origin_x + e.grid_col * (cell_w + spacing);
        int cy = origin_y + e.grid_row * (cell_h + spacing);

        bool portrait = (e.transform == 1 || e.transform == 3);
        int bw = portrait ? port_w : land_w;
        int bh = portrait ? port_h : land_h;

        e.target_w = (float)bw;
        e.target_h = (float)bh;
        e.target_x = (float)(cx + (cell_w - bw) / 2);
        e.target_y = (float)(cy + (cell_h - bh) / 2);
        e.box_x = (int)e.target_x;
        e.box_y = (int)e.target_y;
        e.box_w = bw;
        e.box_h = bh;
    }

    /* Apply button — bottom right */
    int btn_h = 32;
    int btn_pad = 20;
    m_applyRect = QRect(popup_w - btn_pad - 80, popup_h - btn_pad - btn_h, 80, btn_h);
}

/* ── Animation ───────────────────────────────────────────────────── */

void MonitorSetupWidget::animationStep()
{
    bool still_moving = false;
    const float ease = 0.45f;

    for (int i = 0; i < (int)m_entries.size(); i++) {
        if (i == m_dragging)
            continue;

        auto &e = m_entries[i];
        float d[4] = {e.target_x - e.anim_x, e.target_y - e.anim_y,
                      e.target_w - e.anim_w, e.target_h - e.anim_h};
        float *v[4] = {&e.anim_x, &e.anim_y, &e.anim_w, &e.anim_h};
        const float t[4] = {e.target_x, e.target_y, e.target_w, e.target_h};

        for (int k = 0; k < 4; k++) {
            if (std::fabs(d[k]) > 1.0f) {
                *v[k] += d[k] * ease;
                still_moving = true;
            } else {
                *v[k] = t[k];
            }
        }
    }

    if (!still_moving) {
        m_animTimer.stop();
        m_animating = false;
    }
    update();
}

void MonitorSetupWidget::startAnimation()
{
    if (!m_animating) {
        m_animating = true;
        m_animTimer.start(16);
    }
    /* First step immediately — no waiting for the first timer tick. */
    animationStep();
}

/* ── Saved state / dirty checking ─────────────────────────────────── */

void MonitorSetupWidget::snapshotSavedState()
{
    m_savedState.clear();
    for (auto &e : m_entries) {
        SavedMonitorState s;
        s.name = e.name;
        s.width = e.width;
        s.height = e.height;
        s.refresh = e.refresh;
        s.transform = e.transform;
        s.grid_col = e.grid_col;
        s.grid_row = e.grid_row;
        m_savedState.push_back(s);
    }
}

bool MonitorSetupWidget::hasUnsavedChanges() const
{
    if (!m_configExists)
        return true;

    if (m_entries.size() != m_savedState.size())
        return true;

    for (int i = 0; i < (int)m_entries.size(); i++) {
        auto &e = m_entries[i];
        auto &s = m_savedState[i];
        if (e.name != s.name || e.width != s.width || e.height != s.height ||
            e.transform != s.transform || e.grid_col != s.grid_col ||
            e.grid_row != s.grid_row || std::fabs(e.refresh - s.refresh) > 0.5f)
            return true;
    }
    return false;
}

/* ── Resize handling ─────────────────────────────────────────────── */

void MonitorSetupWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    computeBoxLayout();
    /* Snap positions when not animating to avoid lag */
    if (!m_animating && m_dragging < 0) {
        for (auto &e : m_entries) {
            e.anim_x = e.target_x;
            e.anim_y = e.target_y;
            e.anim_w = e.target_w;
            e.anim_h = e.target_h;
        }
    }
}

/* ── Hit testing ─────────────────────────────────────────────────── */

int MonitorSetupWidget::entryAt(int x, int y) const
{
    for (int i = 0; i < (int)m_entries.size(); i++) {
        auto &e = m_entries[i];
        int bx = (int)e.anim_x, by = (int)e.anim_y;
        int bw = (int)e.anim_w, bh = (int)e.anim_h;
        if (x >= bx && x < bx + bw && y >= by && y < by + bh)
            return i;
    }
    return -1;
}

int MonitorSetupWidget::entryAtGrid(int col, int row) const
{
    for (int i = 0; i < (int)m_entries.size(); i++)
        if (m_entries[i].grid_col == col && m_entries[i].grid_row == row)
            return i;
    return -1;
}

int MonitorSetupWidget::entryAtGridExcl(int col, int row, int excl) const
{
    for (int i = 0; i < (int)m_entries.size(); i++) {
        if (i == excl) continue;
        if (m_entries[i].grid_col == col && m_entries[i].grid_row == row)
            return i;
    }
    return -1;
}

bool MonitorSetupWidget::cellAdjacentExcl(int col, int row, int excl) const
{
    static const int dx[] = {-1, 1, 0, 0};
    static const int dy[] = {0, 0, -1, 1};
    for (int d = 0; d < 4; d++)
        if (entryAtGridExcl(col + dx[d], row + dy[d], excl) >= 0)
            return true;
    return false;
}

int MonitorSetupWidget::gridLabelNumber(int idx) const
{
    int num = 1;
    auto &target = m_entries[idx];
    for (auto &e : m_entries) {
        if (e.grid_row < target.grid_row ||
            (e.grid_row == target.grid_row && e.grid_col < target.grid_col))
            num++;
    }
    return num;
}

QString MonitorSetupWidget::gridMakeLabel(int idx) const
{
    auto &target = m_entries[idx];
    QString make = QString::fromStdString(target.make);
    if (make.isEmpty())
        make = "Screen";

    /* Count how many of the same make come before this one in grid order */
    int num = 1;
    for (auto &e : m_entries) {
        if (e.make != target.make)
            continue;
        if (e.grid_row < target.grid_row ||
            (e.grid_row == target.grid_row && e.grid_col < target.grid_col))
            num++;
    }
    return QString("%1 #%2").arg(make).arg(num);
}

/* ── Context menu ────────────────────────────────────────────────── */

void MonitorSetupWidget::showContextMenu(int idx, const QPoint &pos)
{
    if (idx < 0 || idx >= (int)m_entries.size())
        return;

    auto &e = m_entries[idx];
    QMenu menu(this);
    menu.setStyleSheet(
        "QMenu { background-color: #2E2E32; color: white; border: 1px solid #555; }"
        "QMenu::item:selected { background-color: #4A4A50; }");

    /* Rotate */
    menu.addAction("Rotate", [&]() {
        if (e.transform == 1 || e.transform == 3)
            e.transform = 0;
        else
            e.transform = 1;
        computeBoxLayout();
        startAnimation();
        update();
    });

    /* Resolution submenu */
    QMenu *resMenu = menu.addMenu("Resolution");
    /* Collect unique resolutions, sorted descending by pixel count */
    struct Res { int w, h; };
    std::vector<Res> resolutions;
    for (auto &m : e.modes) {
        bool found = false;
        for (auto &r : resolutions)
            if (r.w == m.width && r.h == m.height) { found = true; break; }
        if (!found)
            resolutions.push_back({m.width, m.height});
    }
    std::sort(resolutions.begin(), resolutions.end(), [](const Res &a, const Res &b) {
        return (long)a.w * a.h > (long)b.w * b.h;
    });

    for (int ri = 0; ri < (int)resolutions.size(); ri++) {
        auto &r = resolutions[ri];
        bool current = (r.w == e.width && r.h == e.height);
        QString label;
        if (ri == 0)
            label = QString("%1%2x%3 (max)").arg(current ? "> " : "  ").arg(r.w).arg(r.h);
        else
            label = QString("%1%2x%3").arg(current ? "> " : "  ").arg(r.w).arg(r.h);

        int rw = r.w, rh = r.h;
        resMenu->addAction(label, [this, idx, rw, rh]() {
            auto &entry = m_entries[idx];
            entry.width = rw;
            entry.height = rh;
            /* Reset refresh to highest for this resolution */
            int best = 0;
            for (auto &m : entry.modes)
                if (m.width == rw && m.height == rh && m.refresh_mhz > best)
                    best = m.refresh_mhz;
            if (best > 0)
                entry.refresh = (float)best / 1000.0f;
            update();
        });
    }

    /* Refresh rate submenu */
    QMenu *rateMenu = menu.addMenu("Refresh Rate");
    std::vector<int> rates;
    for (auto &m : e.modes) {
        if (m.width == e.width && m.height == e.height) {
            bool dup = false;
            for (auto r : rates)
                if (std::abs(r - m.refresh_mhz) < 500) { dup = true; break; }
            if (!dup)
                rates.push_back(m.refresh_mhz);
        }
    }
    std::sort(rates.begin(), rates.end(), std::greater<int>());

    for (int ri = 0; ri < (int)rates.size(); ri++) {
        float hz = (float)rates[ri] / 1000.0f;
        bool current = (std::fabs(hz - e.refresh) < 1.0f);
        QString label;
        if (ri == 0)
            label = QString("%1%2 Hz (max)").arg(current ? "> " : "  ").arg(hz, 0, 'f', 2);
        else
            label = QString("%1%2 Hz").arg(current ? "> " : "  ").arg(hz, 0, 'f', 2);

        int rate = rates[ri];
        rateMenu->addAction(label, [this, idx, rate]() {
            m_entries[idx].refresh = (float)rate / 1000.0f;
            update();
        });
    }

    menu.exec(mapToGlobal(pos));
}

/* ── Painting ────────────────────────────────────────────────────── */

void MonitorSetupWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    if (m_entries.empty()) {
        p.setPen(QColor(180, 180, 180));
        p.setFont(QFont("sans-serif", 14));
        p.drawText(rect(), Qt::AlignCenter, "No monitors detected");
        return;
    }

    /* Drop indicator */
    if (m_dragging >= 0) {
        int spacing = m_spacing;
        int cell_step_x = m_cellW + spacing;
        int cell_step_y = m_cellH + spacing;

        if (m_dragInsertDir >= 0) {
            p.setBrush(QColor(102, 179, 255, 128));
            p.setPen(Qt::NoPen);
            if (m_dragInsertDir == 1) {
                p.drawRoundedRect(
                    m_gridOriginX + m_dragTargetCol * cell_step_x,
                    m_gridOriginY + m_dragInsertAfter * cell_step_y + m_cellH,
                    m_cellW, spacing, 4, 4);
            } else {
                p.drawRoundedRect(
                    m_gridOriginX + m_dragInsertAfter * cell_step_x + m_cellW,
                    m_gridOriginY + m_dragTargetRow * cell_step_y,
                    spacing, m_cellH, 4, 4);
            }
        } else {
            p.setBrush(QColor(77, 128, 204, 64));
            p.setPen(Qt::NoPen);
            p.drawRoundedRect(
                m_gridOriginX + m_dragTargetCol * cell_step_x,
                m_gridOriginY + m_dragTargetRow * cell_step_y,
                m_cellW, m_cellH, 6, 6);
        }
    }

    /* Draw monitor boxes (dragged box last = on top) */
    auto drawBox = [&](int i) {
        auto &e = m_entries[i];
        int bw = (int)e.anim_w;
        int bh = (int)e.anim_h;
        int bx = (int)e.anim_x;
        int by = (int)e.anim_y;

        if (bw <= 0 || bh <= 0)
            return;

        QRect boxRect(bx, by, bw, bh);

        /* Background — translucent so the window background shows through */
        p.setBrush(QColor(255, 255, 255, (m_dragging == i) ? 28 : 16));
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(boxRect, 6, 6);

        /* Border */
        QColor bordColor = (m_dragging == i) ? QColor(128, 179, 255) : QColor(102, 128, 179);
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(bordColor, 2));
        p.drawRoundedRect(boxRect, 6, 6);

        /* Labels — sized from box height, only drawn if they fit */
        const int mainPt = std::clamp(bh / 9, 12, 18);
        const int smallPt = std::clamp(bh / 14, 10, 14);
        QFont font("sans-serif", mainPt);
        QFont smallFont("sans-serif", smallPt);

        /* Make #N (XXHz) — upper area */
        QString line1 = QString("%1 (%2Hz)").arg(gridMakeLabel(i)).arg((int)e.refresh);
        p.setFont(font);
        QFontMetrics fm(font);
        p.setPen(QColor(230, 230, 230));
        int lineH = fm.height();
        int textY = by + bh / 2 - lineH - 4;
        if (fm.horizontalAdvance(line1) < bw - 8)
            p.drawText(bx + 4, textY, bw - 8, lineH, Qt::AlignCenter, line1);

        /* Connector name — below with clear gap */
        p.setFont(smallFont);
        QFontMetrics fms(smallFont);
        p.setPen(QColor(153, 153, 166));
        int nameY = by + bh / 2 + 4;
        QString nameStr = QString::fromStdString(e.name);
        if (fms.horizontalAdvance(nameStr) < bw - 8)
            p.drawText(bx + 4, nameY, bw - 8, fms.height(), Qt::AlignCenter, nameStr);

        /* Resolution — bottom */
        p.setPen(QColor(128, 128, 140));
        QString res;
        if (e.transform == 1 || e.transform == 3)
            res = QString("%1x%2 (R)").arg(e.height).arg(e.width);
        else
            res = QString("%1x%2").arg(e.width).arg(e.height);
        if (fms.horizontalAdvance(res) < bw - 8)
            p.drawText(bx + 4, by + bh - fms.height() - 6, bw - 8, fms.height(),
                        Qt::AlignHCenter, res);
    };

    for (int i = 0; i < (int)m_entries.size(); i++) {
        if (i != m_dragging)
            drawBox(i);
    }
    /* Draw dragged box on top */
    if (m_dragging >= 0 && m_dragging < (int)m_entries.size())
        drawBox(m_dragging);

    /* Apply button — only shown when there are unsaved changes */
    if (hasUnsavedChanges()) {
        p.setBrush(QColor(51, 128, 77));
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(m_applyRect, 4, 4);
        p.setPen(Qt::white);
        p.setFont(QFont("sans-serif", 11));
        p.drawText(m_applyRect, Qt::AlignCenter, "Apply");
    }
}

/* ── Mouse press ─────────────────────────────────────────────────── */

void MonitorSetupWidget::mousePressEvent(QMouseEvent *event)
{
    int x = event->pos().x();
    int y = event->pos().y();

    if (event->button() == Qt::LeftButton) {
        /* Apply button (only active when visible) */
        if (hasUnsavedChanges() && m_applyRect.contains(event->pos())) {
            compactGrid();
            writeConfig();
            return;
        }

        /* Start drag */
        int idx = entryAt(x, y);
        if (idx >= 0) {
            m_dragging = idx;
            m_dragOffsetX = x - (int)m_entries[idx].anim_x;
            m_dragOffsetY = y - (int)m_entries[idx].anim_y;
            m_dragOrigCol = m_entries[idx].grid_col;
            m_dragOrigRow = m_entries[idx].grid_row;
            m_dragTargetCol = m_entries[idx].grid_col;
            m_dragTargetRow = m_entries[idx].grid_row;
            m_dragInsertDir = -1;
            m_dragInsertAfter = -1;
            m_dragSavedCol.resize(m_entries.size());
            m_dragSavedRow.resize(m_entries.size());
            for (int k = 0; k < (int)m_entries.size(); k++) {
                m_dragSavedCol[k] = m_entries[k].grid_col;
                m_dragSavedRow[k] = m_entries[k].grid_row;
            }
            setHighlight(idx);
            update();
        }
    } else if (event->button() == Qt::RightButton) {
        int idx = entryAt(x, y);
        if (idx >= 0)
            showContextMenu(idx, event->pos());
    }
}

/* ── Mouse move (drag logic matching monitor_setup_handle_motion) ── */

void MonitorSetupWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging < 0 || m_dragging >= (int)m_entries.size())
        return;

    int lx = event->pos().x();
    int ly = event->pos().y();
    int di = m_dragging;
    auto &e = m_entries[di];
    int spacing = m_spacing;
    int cell_step_x = m_cellW + spacing;
    int cell_step_y = m_cellH + spacing;

    /* Follow cursor */
    e.anim_x = (float)(lx - m_dragOffsetX);
    e.anim_y = (float)(ly - m_dragOffsetY);
    e.target_x = e.anim_x;
    e.target_y = e.anim_y;

    float drag_cx = e.anim_x + e.anim_w / 2.0f;
    float drag_cy = e.anim_y + e.anim_h / 2.0f;

    float rel_x = drag_cx - (float)m_gridOriginX;
    float rel_y = drag_cy - (float)m_gridOriginY;

    int slot_col = (int)std::floor(rel_x / (float)cell_step_x);
    int slot_row = (int)std::floor(rel_y / (float)cell_step_y);
    float in_cell_x = rel_x - slot_col * (float)cell_step_x;
    float in_cell_y = rel_y - slot_row * (float)cell_step_y;

    int new_target_col = m_dragTargetCol;
    int new_target_row = m_dragTargetRow;
    int new_insert_dir = -1;
    int new_insert_after = -1;
    bool found = false;

    /* --- 1. Between insertion (expanded detection zone) --- */
    {
        float extend_y = m_cellH * 0.18f;
        float extend_x = m_cellW * 0.18f;
        float v_thresh = (float)spacing * 0.5f + extend_y;
        float h_thresh = (float)spacing * 0.5f + extend_x;

        float dv_below = std::fabs(in_cell_y - ((float)m_cellH + (float)spacing * 0.5f));
        float dv_above = in_cell_y + (float)spacing * 0.5f;
        float dh_right = std::fabs(in_cell_x - ((float)m_cellW + (float)spacing * 0.5f));
        float dh_left  = in_cell_x + (float)spacing * 0.5f;

        bool near_any_v = (dv_below < v_thresh) || (dv_above < v_thresh);
        bool near_any_h = (dh_right < h_thresh) || (dh_left < h_thresh);

        /* Vertical gap below */
        if (dv_below < v_thresh && !near_any_h) {
            int top = entryAtGridExcl(slot_col, slot_row, di);
            int bot = entryAtGridExcl(slot_col, slot_row + 1, di);
            if (top >= 0 && bot >= 0) {
                new_target_col = slot_col;
                new_target_row = slot_row + 1;
                new_insert_dir = 1;
                new_insert_after = slot_row;
                found = true;
            }
        }

        /* Vertical gap above */
        if (!found && dv_above < v_thresh && !near_any_h) {
            int top = entryAtGridExcl(slot_col, slot_row - 1, di);
            int bot = entryAtGridExcl(slot_col, slot_row, di);
            if (top >= 0 && bot >= 0) {
                new_target_col = slot_col;
                new_target_row = slot_row;
                new_insert_dir = 1;
                new_insert_after = slot_row - 1;
                found = true;
            }
        }

        /* Horizontal gap right */
        if (!found && dh_right < h_thresh && !near_any_v) {
            int left = entryAtGridExcl(slot_col, slot_row, di);
            int right = entryAtGridExcl(slot_col + 1, slot_row, di);
            if (left >= 0 && right >= 0) {
                new_target_col = slot_col + 1;
                new_target_row = slot_row;
                new_insert_dir = 0;
                new_insert_after = slot_col;
                found = true;
            }
        }

        /* Horizontal gap left */
        if (!found && dh_left < h_thresh && !near_any_v) {
            int left = entryAtGridExcl(slot_col - 1, slot_row, di);
            int right = entryAtGridExcl(slot_col, slot_row, di);
            if (left >= 0 && right >= 0) {
                new_target_col = slot_col;
                new_target_row = slot_row;
                new_insert_dir = 0;
                new_insert_after = slot_col - 1;
                found = true;
            }
        }
    }

    /* --- 2. Snap to nearest valid empty adjacent cell --- */
    if (!found) {
        int hover_col = (int)std::round(rel_x / (float)cell_step_x);
        int hover_row = (int)std::round(rel_y / (float)cell_step_y);
        int occ = entryAtGridExcl(hover_col, hover_row, di);

        if (occ >= 0) {
            float cell_cx = (float)m_gridOriginX + hover_col * cell_step_x + m_cellW / 2.0f;
            float cell_cy = (float)m_gridOriginY + hover_row * cell_step_y + m_cellH / 2.0f;
            float ddx = drag_cx - cell_cx;
            float ddy = drag_cy - cell_cy;
            int dc[4], dr[4];

            if (std::fabs(ddy) >= std::fabs(ddx)) {
                if (ddy <= 0) {
                    dc[0]=0; dr[0]=-1; dc[1]=-1; dr[1]=0; dc[2]=1; dr[2]=0; dc[3]=0; dr[3]=1;
                } else {
                    dc[0]=0; dr[0]=1; dc[1]=-1; dr[1]=0; dc[2]=1; dr[2]=0; dc[3]=0; dr[3]=-1;
                }
            } else {
                if (ddx <= 0) {
                    dc[0]=-1; dr[0]=0; dc[1]=0; dr[1]=-1; dc[2]=0; dr[2]=1; dc[3]=1; dr[3]=0;
                } else {
                    dc[0]=1; dr[0]=0; dc[1]=0; dr[1]=-1; dc[2]=0; dr[2]=1; dc[3]=-1; dr[3]=0;
                }
            }

            for (int d = 0; d < 4; d++) {
                int tc = hover_col + dc[d];
                int tr = hover_row + dr[d];
                if (entryAtGridExcl(tc, tr, di) < 0) {
                    new_target_col = tc;
                    new_target_row = tr;
                    found = true;
                    break;
                }
            }
        } else if (cellAdjacentExcl(hover_col, hover_row, di)) {
            new_target_col = hover_col;
            new_target_row = hover_row;
            found = true;
        }
    }

    /* --- 3. Fallback: nearest valid cell by distance --- */
    if (!found) {
        float best_dist = 1e18f;
        for (int r = -1; r <= m_gridRows; r++) {
            for (int c = -1; c <= m_gridCols; c++) {
                if (entryAtGridExcl(c, r, di) >= 0) continue;
                if (!cellAdjacentExcl(c, r, di)) continue;
                float cx = (float)m_gridOriginX + c * cell_step_x + m_cellW / 2.0f;
                float cy = (float)m_gridOriginY + r * cell_step_y + m_cellH / 2.0f;
                float dx = drag_cx - cx;
                float dy = drag_cy - cy;
                float dist = dx * dx + dy * dy;
                if (dist < best_dist) {
                    best_dist = dist;
                    new_target_col = c;
                    new_target_row = r;
                    found = true;
                }
            }
        }
    }

    if (found && (new_target_col != m_dragTargetCol ||
                  new_target_row != m_dragTargetRow ||
                  new_insert_dir != m_dragInsertDir)) {
        m_dragTargetCol = new_target_col;
        m_dragTargetRow = new_target_row;
        m_dragInsertDir = new_insert_dir;
        m_dragInsertAfter = new_insert_after;
    }

    update();
}

/* ── Mouse release (drop logic matching monitor_setup_handle_button) */

void MonitorSetupWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton || m_dragging < 0)
        return;

    int di = m_dragging;
    auto &de = m_entries[di];

    /* Restore saved grid positions */
    for (int i = 0; i < (int)m_entries.size(); i++) {
        if (i == di) continue;
        m_entries[i].grid_col = m_dragSavedCol[i];
        m_entries[i].grid_row = m_dragSavedRow[i];
    }

    /* Handle between insertion */
    if (m_dragInsertDir >= 0) {
        if (m_dragInsertDir == 1) {
            for (int i = 0; i < (int)m_entries.size(); i++) {
                if (i == di) continue;
                if (m_entries[i].grid_row > m_dragInsertAfter)
                    m_entries[i].grid_row++;
            }
        } else {
            for (int i = 0; i < (int)m_entries.size(); i++) {
                if (i == di) continue;
                if (m_entries[i].grid_col > m_dragInsertAfter)
                    m_entries[i].grid_col++;
            }
        }
    }

    de.grid_col = m_dragTargetCol;
    de.grid_row = m_dragTargetRow;

    m_dragging = -1;
    m_dragInsertDir = -1;

    setHighlight(-1);

    compactGrid();
    computeBoxLayout();
    startAnimation();
    updateOverlayLabels();
    update();
}

/* ── Destructor ──────────────────────────────────────────────────── */

MonitorSetupWidget::~MonitorSetupWidget()
{
    destroyOverlays();
}

/* ── Show/Hide events ────────────────────────────────────────────── */

void MonitorSetupWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    createOverlays();
}

void MonitorSetupWidget::hideEvent(QHideEvent *event)
{
    destroyOverlays();
    QWidget::hideEvent(event);
}

/* ── Overlay management (writes file for nixlytile to render) ────── */

void MonitorSetupWidget::writeOverlayFile(const QString &highlightName)
{
    QString path = overlayFilePath();
    QDir().mkpath(QFileInfo(path).absolutePath());

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    QTextStream out(&file);
    for (int i = 0; i < (int)m_entries.size(); i++) {
        out << QString::fromStdString(m_entries[i].name)
            << " " << gridMakeLabel(i) << " (" << (int)m_entries[i].refresh << "Hz)\n";
    }
    if (!highlightName.isEmpty())
        out << "highlight=" << highlightName << "\n";
}

void MonitorSetupWidget::createOverlays()
{
    writeOverlayFile();
}

void MonitorSetupWidget::destroyOverlays()
{
    QFile::remove(overlayFilePath());
}

void MonitorSetupWidget::updateOverlayLabels()
{
    writeOverlayFile();
}

void MonitorSetupWidget::setHighlight(int entryIdx)
{
    if (entryIdx >= 0 && entryIdx < (int)m_entries.size())
        writeOverlayFile(QString::fromStdString(m_entries[entryIdx].name));
    else
        writeOverlayFile();
}
