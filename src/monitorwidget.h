#pragma once
#include <QWidget>
#include <QTimer>
#include <vector>
#include <string>

struct MonitorMode {
    int width, height, refresh_mhz;
};

struct MonitorEntry {
    std::string name;
    std::string make;   /* Brand name from EDID, e.g. "Samsung", "HP" */
    int width = 0, height = 0;
    float refresh = 60.0f;
    int transform = 0;     /* 0=normal, 1=90, 2=180, 3=270 */
    int grid_col = 0, grid_row = 0;
    std::vector<MonitorMode> modes;

    /* Animation state */
    float anim_x = 0, anim_y = 0;
    float target_x = 0, target_y = 0;
    float anim_w = 0, anim_h = 0;
    float target_w = 0, target_h = 0;

    /* Computed box geometry */
    int box_x = 0, box_y = 0, box_w = 0, box_h = 0;
};

/* Subset of MonitorEntry used for dirty-checking against saved config */
struct SavedMonitorState {
    std::string name;
    int width = 0, height = 0;
    float refresh = 60.0f;
    int transform = 0;
    int grid_col = 0, grid_row = 0;
};

class MonitorSetupWidget : public QWidget
{
public:
    explicit MonitorSetupWidget(QWidget *parent = nullptr);
    ~MonitorSetupWidget() override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private:
    void enumerateMonitors();
    void loadConfig();
    void writeConfig();
    void compactGrid();
    void computeBoxLayout();
    void animationStep();
    void startAnimation();
    void snapshotSavedState();
    bool hasUnsavedChanges() const;
    int entryAt(int x, int y) const;
    int entryAtGrid(int col, int row) const;
    int entryAtGridExcl(int col, int row, int excl) const;
    bool cellAdjacentExcl(int col, int row, int excl) const;
    int gridLabelNumber(int idx) const;
    QString gridMakeLabel(int idx) const;
    void showContextMenu(int idx, const QPoint &pos);

    void writeOverlayFile(const QString &highlightName = QString());
    void createOverlays();
    void destroyOverlays();
    void updateOverlayLabels();
    void setHighlight(int entryIdx);

    std::vector<MonitorEntry> m_entries;

    /* Saved state for dirty-checking */
    std::vector<SavedMonitorState> m_savedState;
    bool m_configExists = false;

    /* Grid info */
    int m_gridCols = 0, m_gridRows = 0;
    int m_cellW = 0, m_cellH = 0;
    int m_gridOriginX = 0, m_gridOriginY = 0;
    int m_spacing = 20;

    /* Drag state */
    int m_dragging = -1;
    int m_dragOffsetX = 0, m_dragOffsetY = 0;
    int m_dragOrigCol = 0, m_dragOrigRow = 0;
    int m_dragTargetCol = 0, m_dragTargetRow = 0;
    int m_dragInsertDir = -1;
    int m_dragInsertAfter = -1;
    std::vector<int> m_dragSavedCol;
    std::vector<int> m_dragSavedRow;

    /* Animation */
    QTimer m_animTimer;
    bool m_animating = false;

    /* Button geometry */
    QRect m_applyRect;
};
