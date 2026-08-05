qt6 = import('qt6')
qt6_dep = dependency('qt6', modules: ['Core', 'Gui', 'Widgets', 'Network', 'Svg'])
drm_dep = dependency('libdrm')
pam_dep = dependency('pam')

hwdata_dep = dependency('hwdata', required: false)
if hwdata_dep.found()
  add_project_arguments(
    '-DHWDATA_DIR="' + hwdata_dep.get_variable(pkgconfig: 'pkgdatadir') + '"',
    language: 'cpp',
  )
endif

sources = files(
  'main.cpp',
  'mainwindow.cpp',
  'monitorwidget.cpp',
  'page_sysinfo.cpp',
  'sysinfo/battery.cpp',
  'sysinfo/collect.cpp',
  'sysinfo/network.cpp',
  'sysinfo/pciids.cpp',
  'sysinfo/storage.cpp',
  'page_usersettings.cpp',
  'page_monitors.cpp',
  'page_applications.cpp',
  'apps/pkgindex.cpp',
  'apps/icons.cpp',
  'apps/iconfarm.cpp',
  'apps/rowdelegate.cpp',
  'page_mousekeyboard.cpp',
  'page_shortcuts.cpp',
  'page_statusbar.cpp',
  'page_neovim.cpp',
  'page_storage.cpp',
  'page_firewall.cpp',
  'page_git.cpp',
  'git/auth.cpp',
  'git/keys.cpp',
  'git/module.cpp',
  'git/panel.cpp',
  'git/rebuild.cpp',
  'git/repo.cpp',
  'git/style.cpp',
  'git/task.cpp',
  'page_backup.cpp',
  'page_gaming.cpp',
  'page_winvm.cpp',
  'page_cleanup.cpp',
)

executable('nixlycc',
  sources,
  dependencies: [qt6_dep, drm_dep, pam_dep],
  install: true,
)
