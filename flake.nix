{
  description = "Vulkan C++ development environment";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs = { nixpkgs, ... }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs {
        inherit system;
      };
    in
    {
      devShells.${system}.default = pkgs.mkShell {
        packages = with pkgs; [
          # Build tools
          gcc
          cmake
          ninja
          pkg-config

          # Vulkan
          vulkan-headers
          vulkan-loader
          vulkan-tools
          vulkan-validation-layers

          # GLSL -> SPIR-V
          shaderc

          # Wayland
          wayland
          wayland-scanner
          libxkbcommon

          libffi
          libGL
        ];

        LD_LIBRARY_PATH = with pkgs; pkgs.lib.makeLibraryPath [
          wayland
          libxkbcommon
          vulkan-loader
          libGL
        ] + ":/run/opengl-driver/lib:/run/opengl-driver-32/lib";
      };
    };
}
