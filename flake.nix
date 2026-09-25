{
  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

  outputs = { self, nixpkgs }: {
    packages = nixpkgs.lib.genAttrs nixpkgs.lib.systems.flakeExposed (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
      in rec {
        qthash = pkgs.clangStdenv.mkDerivation {
          name = "qthash";
          src = self.sourceInfo;
          strictDeps = true;
          doCheck = true;
          mesonBuildType = "debugoptimized";
          nativeBuildInputs = with pkgs; [
            desktop-file-utils
            meson
            ninja
            pkg-config
            qt6.wrapQtAppsHook
          ];
          buildInputs = with pkgs; [
            glib
            libb2
            libblake3
            libgcrypt
            qt6.qtbase
            xxhash
          ];
          preConfigure = ''
            # Required for LTO
            export AR="${pkgs.llvm}/bin/llvm-ar"
          '';
          meta = {
            mainProgram = "qthash";
            license = pkgs.lib.licenses.gpl2Plus;
          };
        };
        default = qthash;
      }
    );

    devShells = nixpkgs.lib.genAttrs nixpkgs.lib.systems.flakeExposed (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
      in {
        default = pkgs.mkShell.override { stdenv = pkgs.clangStdenv; } {
          packages = with pkgs; [
            desktop-file-utils
            gettext
            glib
            libb2
            libblake3
            libgcrypt
            libxml2 # xmllint
            mbedtls
            meson
            nettle
            ninja
            openssl
            pkg-config
            qt6.qtbase
            xxhash
          ];
          shellHook = ''
            export AR="${pkgs.llvm}/bin/llvm-ar"
          '';
        };
      }
    );
  };
}
