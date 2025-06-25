executable('nixlycc',
  'main.cpp',
  dependencies: [
    dependency('qt6', modules: ['Core', 'Gui', 'Widgets', 'WaylandClient', 'WaylandCompositor']),
    dependency('wayland-client'),
    dependency('wayland-protocols'),
    dependency('wayland-egl'),
    dependency('wayland-cursor'),
    dependency('xkbcommon'),
    dependency('egl'),
  ],
  install: true
)
