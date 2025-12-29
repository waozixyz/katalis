# shell.nix - Katalis development environment
{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  buildInputs = with pkgs; [
    # Core C toolchain
    gcc
    gnumake
    pkg-config

    # Kryon framework
    nim
    nimble

    # SDL3 rendering backend
    sdl3
    sdl3-ttf
    sdl3-image

    # Raylib
    raylib

    # Text rendering
    harfbuzz
    freetype
    fribidi

    # System libraries for graphics
    libGL
    libglvnd
    xorg.libX11
    xorg.libXrandr
    xorg.libXi
    xorg.libXcursor
    libxkbcommon

    # Development tools
    git
    gdb
    tree
  ];

  shellHook = ''
    echo "Katalis Development Environment"
    echo "================================"
    echo "GCC version: $(gcc --version | head -1)"
    echo "Nim version: $(nim --version | head -1)"
    echo ""
    echo "Project: Katalis (Kryon C Application)"
    echo "Frontend: C"
    echo "Renderer: SDL3"
    echo ""
    echo "Build with: kryon build"
    echo ""
  '';
}
