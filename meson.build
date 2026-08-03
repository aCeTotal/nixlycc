project('nixlycc', 'cpp',
  version: '0.2',
  default_options: ['cpp_std=c++17']
)

install_data('nixlycc.desktop', 'nixlycc-control-panel.desktop', 'nixlycc-monitors.desktop',
  install_dir: join_paths(get_option('datadir'), 'applications')
)

subdir('src')
