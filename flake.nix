{
  description = "Nix flake for EVM-assigner";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-23.11";
    flake-utils.url = "github:numtide/flake-utils";
    nix-3rdparty = {
      url = "github:NilFoundation/nix-3rdparty";
      inputs = {
        nixpkgs.follows = "nixpkgs";
        flake-utils.follows = "flake-utils";
      };
    };
    nil_crypto3 = {
      url = "https://github.com/NilFoundation/crypto3";
      type = "git";
      submodules = true;
      inputs = {
        nixpkgs.follows = "nixpkgs";
      };
    };
    nil_zkllvm_blueprint = {
      url = "https://github.com/NilFoundation/zkllvm-blueprint";
      type = "git";
      submodules = true;
      inputs = {
        nixpkgs.follows = "nixpkgs";
        flake-utils.follows = "flake-utils";
        nil_crypto3.follows = "nil_crypto3";
      };
    };
  };

  outputs =
    {
      self,
      nixpkgs,
      flake-utils,
      nix-3rdparty,
      ...
    }@inputs:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = import nixpkgs {
          overlays = [ nix-3rdparty.overlays.${system}.default ];
          inherit system;
        };

        # Add more complicated logic here if you need to have debug packages
        resolveInput = input: input.packages.${system}.default;

        nil_inputs = with inputs; [
          nil_crypto3
          nil_zkllvm_blueprint
        ];

        nil_packages = map resolveInput nil_inputs;

        defaultNativeBuildInputs = with pkgs; [
          cmake
          ninja
        ];

        defaultCheckInputs = [ pkgs.gtest ];

        commonBuildInputs = [ pkgs.boost ];

        inputsToPropagate =
          {
            enableDebug ? false,
          }:
          let
            deps = with pkgs; [
              (evmc.override { inherit enableDebug; })
              (intx.override { inherit enableDebug; })
              ethash
            ];
          in
          deps ++ nil_packages;

        devInputs = [ pkgs.clang_17 ];

        makePackage =
          {
            enableDebug ? false,
          }:
          pkgs.gcc13Stdenv.mkDerivation {
            name = "EVM-assigner" + "${if enableDebug then "-debug" else ""}";

            nativeBuildInputs = defaultNativeBuildInputs;

            buildInputs = commonBuildInputs;

            propagatedBuildInputs = inputsToPropagate { inherit enableDebug; };

            src = self;

            cmakeBuildType = if enableDebug then "Debug" else "Release";

            doCheck = false;
            dontStrip = enableDebug;
          };

        makeTests =
          {
            enableDebug ? false,
          }:
          (makePackage { inherit enableDebug; }).overrideAttrs (
            finalAttrs: previousAttrs: {
              name = previousAttrs.name + "-tests";

              checkInputs = defaultCheckInputs;

              cmakeFlags = [ "-DBUILD_ASSIGNER_TESTS=TRUE" ];

              dontInstall = true;

              doCheck = true;

              GTEST_OUTPUT = "xml:${placeholder "out"}/test-reports/";
            }
          );

        makeDevShell =
          {
            enableDebug ? false,
          }:
          pkgs.mkShell {
            nativeBuildInputs = defaultNativeBuildInputs;
            buildInputs =
              commonBuildInputs ++ inputsToPropagate { inherit enableDebug; } ++ defaultCheckInputs ++ devInputs;

            shellHook = ''
              echo "evm-assigner dev ${if enableDebug then "debug" else "release"} environment activated"
            '';
          };
      in
      {
        packages = rec {
          release = makePackage { };
          debug = makePackage { enableDebug = true; };
          default = release;
        };
        checks = {
          default = makeTests { };
        };
        devShells = {
          default = makeDevShell { };
          debug = makeDevShell { enableDebug = true; };
        };
      }
    );
}
