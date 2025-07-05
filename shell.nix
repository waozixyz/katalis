{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  buildInputs = with pkgs; [
    # Rust toolchain
    rustc
    cargo
    rustfmt
    clippy
    rust-analyzer
    
    # Build tools
    cmake
    pkg-config
    gcc
    gnumake
    
    # Clang/LLVM for bindgen (CRITICAL - this was missing!)
    clang
    llvm
    libclang.lib
    
    # Graphics and windowing dependencies
    xorg.libX11
    xorg.libX11.dev
    xorg.libXrandr
    xorg.libXrandr.dev
    xorg.libXinerama
    xorg.libXcursor
    xorg.libXi
    xorg.libXext
    xorg.libXxf86vm
    xorg.libXfixes
    
    # OpenGL
    libGL
    libGL.dev
    libGLU
    mesa
    mesa.dev
    
    # Audio
    alsa-lib
    alsa-lib.dev
    pulseaudio
    
    # Wayland support (optional)
    wayland
    wayland.dev
    wayland-protocols
    libxkbcommon
    libxkbcommon.dev
    
    # Additional system libraries
    systemd
    systemd.dev
    udev
    
    # Development tools
    gdb
    strace
  ];

  # Environment variables for proper library detection
  shellHook = ''
    # Bindgen/libclang configuration (CRITICAL!)
    export LIBCLANG_PATH="${pkgs.libclang.lib}/lib"
    export BINDGEN_EXTRA_CLANG_ARGS="-I${pkgs.glibc.dev}/include -I${pkgs.clang}/resource-root/include"
    
    # PKG_CONFIG_PATH for all the development headers
    export PKG_CONFIG_PATH="${pkgs.lib.makeSearchPathOutput "dev" "lib/pkgconfig" [
      pkgs.alsa-lib
      pkgs.libGL
      pkgs.mesa
      pkgs.xorg.libX11
      pkgs.xorg.libXrandr
      pkgs.xorg.libXinerama
      pkgs.xorg.libXcursor
      pkgs.xorg.libXi
      pkgs.wayland
      pkgs.libxkbcommon
      pkgs.systemd
    ]}:$PKG_CONFIG_PATH"
    
    # LD_LIBRARY_PATH for runtime libraries
    export LD_LIBRARY_PATH="${pkgs.lib.makeLibraryPath [
      pkgs.libGL
      pkgs.mesa
      pkgs.xorg.libX11
      pkgs.xorg.libXrandr
      pkgs.xorg.libXinerama
      pkgs.xorg.libXcursor
      pkgs.xorg.libXi
      pkgs.alsa-lib
      pkgs.wayland
      pkgs.libxkbcommon
      pkgs.libclang.lib
      pkgs.systemd
    ]}:$LD_LIBRARY_PATH"
    
    # CMake configuration
    export CMAKE_PREFIX_PATH="${pkgs.lib.makeSearchPathOutput "dev" "lib/cmake" [
      pkgs.xorg.libX11
      pkgs.libGL
      pkgs.alsa-lib
    ]}:$CMAKE_PREFIX_PATH"
    
    # C compiler flags for proper header detection
    export CFLAGS="-I${pkgs.glibc.dev}/include"
    export CPPFLAGS="-I${pkgs.glibc.dev}/include"
    
    echo "🎮 Katalis Development Environment Loaded!"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "📦 Available tools:"
    echo "   • Rust $(rustc --version)"
    echo "   • Cargo $(cargo --version)"
    echo "   • Clang $(clang --version | head -1)"
    echo "   • CMake $(cmake --version | head -1)"
    echo ""
    echo "🔧 Graphics libraries configured for Raylib"
    echo "🔊 Audio support (ALSA/PulseAudio) ready"
    echo "⚡ Bindgen/libclang configured"
    echo ""
    echo "📍 LIBCLANG_PATH: $LIBCLANG_PATH"
    echo ""
    echo "▶️  Run 'cargo run' to start development!"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
  '';

  # Rust-specific environment
  RUST_SRC_PATH = "${pkgs.rust.packages.stable.rustPlatform.rustLibSrc}";
  RUST_BACKTRACE = "1";
  
  # Additional environment variables for C compilation
  CC = "${pkgs.gcc}/bin/gcc";
  CXX = "${pkgs.gcc}/bin/g++";
}