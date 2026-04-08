qt6 = import('qt6')
qt6_dep = dependency('qt6', modules: ['Core', 'Gui', 'Widgets'])
drm_dep = dependency('libdrm')

sources = files(
  'main.cpp',
  'mainwindow.cpp',
  'monitorwidget.cpp',
  'page_sysinfo.cpp',
  'page_usersettings.cpp',
  'page_monitors.cpp',
  'page_applications.cpp',
  'page_mousekeyboard.cpp',
  'page_shortcuts.cpp',
  'page_statusbar.cpp',
  'page_neovim.cpp',
  'page_storage.cpp',
  'page_firewall.cpp',
  'page_backup.cpp',
  'page_gaming.cpp',
  'page_winvm.cpp',
  'page_cleanup.cpp',
)

executable('nixlycc',
  sources,
  dependencies: [qt6_dep, drm_dep],
  install: true,
)
