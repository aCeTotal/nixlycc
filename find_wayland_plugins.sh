#!/usr/bin/env bash

# Find Qt dirs with plugins
find /nix/store -path "*/qt-6/plugins" -type d 2>/dev/null | while read qt_dir; do
  if [ -d "$qt_dir/wayland-shell-integration" ]; then
    echo "Found wayland-shell-integration directory: $qt_dir/wayland-shell-integration"
    ls -la "$qt_dir/wayland-shell-integration"
  fi
done

# Find shell integration plugins
find /nix/store -name "*xdg-shell*.so" 2>/dev/null
find /nix/store -name "*wl-shell*.so" 2>/dev/null
find /nix/store -name "*ivi-shell*.so" 2>/dev/null
find /nix/store -name "*qt-shell*.so" 2>/dev/null
find /nix/store -name "*wayland*shell*.so" 2>/dev/null

# Search for any shell integration plugins with broader patterns
find /nix/store -path "*/qtwayland*" -type d 2>/dev/null | while read dir; do
  echo "Checking directory: $dir"
  find "$dir" -name "*.so" | grep -i shell 2>/dev/null
done

find /nix/store -path "*/qtwayland-*/lib/qt-6/plugins" -type d 2>/dev/null | while read plugins_dir; do
  echo "Found qtwayland plugins directory: $plugins_dir"
  find "$plugins_dir" -name "*.so" 2>/dev/null | sort
done

find /nix/store -path "*/qt-6/plugins/platforms/libqwayland*.so" 2>/dev/null | while read wayland_plugin; do
  echo "Found Wayland platform plugin: $wayland_plugin"
  ldd "$wayland_plugin" | grep -i shell
done
