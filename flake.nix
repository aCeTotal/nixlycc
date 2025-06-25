{
  description = "Wayland Qt6 C++ Nixlycc";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };
      in {
        devShells.default = pkgs.mkShell {
          buildInputs = [
            pkgs.qt6.qtbase
            pkgs.qt6.qtwayland
            pkgs.qt6.qttools
            pkgs.qt6.qtsvg
            pkgs.wayland
            pkgs.libxkbcommon
            pkgs.meson
            pkgs.ninja
            pkgs.pkg-config
            pkgs.cmake
            pkgs.gcc
            pkgs.gdb
            pkgs.wayland-protocols
            pkgs.wayland-scanner
            pkgs.wayland-utils
            pkgs.weston
            pkgs.libglvnd # For EGL support
          ];
          shellHook = ''
            # Set up Qt environment for Wayland
            export QT_QPA_PLATFORM=wayland
            export QT_QPA_PLATFORMTHEME=wayland
            export QT_WAYLAND_DISABLE_WINDOWDECORATION=1
            
            # Set plugin paths
            export QT_PLUGIN_PATH=${pkgs.qt6.qtbase}/lib/qt-6/plugins:${pkgs.qt6.qtwayland}/lib/qt-6/plugins
            export QT_QPA_PLATFORM_PLUGIN_PATH=${pkgs.qt6.qtwayland}/lib/qt-6/plugins/platforms
            
            # Set Wayland shell integration
            export QT_WAYLAND_SHELL_INTEGRATION=xdg-shell
            
            # Enable debugging
            export QT_DEBUG_PLUGINS=1
            export QT_LOGGING_RULES="qt.qpa.*=true"
            
            # Print diagnostic information
            echo "Qt plugin path: $QT_PLUGIN_PATH"
            echo "Qt platform plugin path: $QT_QPA_PLATFORM_PLUGIN_PATH"
          '';
        };
      }
    );
}
