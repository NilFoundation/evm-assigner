{
  description = "Nix flake for EVM-assigner";

  inputs = {
    nixpkgs = { url = "github:NixOS/nixpkgs/nixos-23.11"; };
    nil_crypto3 = {
      url = "https://github.com/NilFoundation/crypto3";
      type = "git";
      submodules = true;
    };
    nil_zkllvm_blueprint = {
      url = "https://github.com/NilFoundation/zkllvm-blueprint";
      type = "git";
      rev = "01964c21ab68e0cd77e08c0a9225f220a5b6bf2d";
      submodules = true;
    };
    intx = { url = "github:chfast/intx"; flake = false; };
    evmc = { url = "github:ethereum/evmc"; flake = false; };
  };

  outputs = { self, nixpkgs, nil_crypto3, nil_zkllvm_blueprint, ... } @ repos:
    let
      supportedSystems = [
        "x86_64-linux"
      ];

      # For all supported systems call `f` providing system pkgs, crypto3 and blueprint
      forAllSystems = f: nixpkgs.lib.genAttrs supportedSystems (system: f {
        pkgs = import nixpkgs {
          inherit system;
        };
      });

      nonFlakeDependencies = { pkgs, repos, enable_debug }:
        let
          lib = pkgs.lib;
          stdenv = pkgs.gcc13Stdenv;
        in
        rec {
          intx = (pkgs.callPackage ./nix/intx.nix) {
            repo = repos.intx;
            inherit pkgs lib stdenv enable_debug;
          };

          evmc = (pkgs.callPackage ./nix/evmc.nix) {
            repo = repos.evmc;
            inherit pkgs lib stdenv enable_debug;
          };
        };

      makeReleaseBuild = { pkgs }:
        let
          deps = nonFlakeDependencies { enable_debug = false; inherit repos pkgs; };
          crypto3 = nil_crypto3.packages.${pkgs.system}.default;
          blueprint = nil_zkllvm_blueprint.packages.${pkgs.system}.default;
        in
        pkgs.gcc13Stdenv.mkDerivation {
          name = "EVM-assigner";

          buildInputs = with pkgs; [
            cmake
            ninja
            gtest
            boost
            ethash
            deps.intx
            deps.evmc
            crypto3
            blueprint
          ];

          src = self;

          cmakeFlags = [
            "-DHUNTER_ENABLED=OFF" # TODO: this will be removed after we get rid of Hunter
          ];

          doCheck = false;
        };

      makeTests = { pkgs }:
        let
          deps = nonFlakeDependencies { enable_debug = false; inherit repos pkgs; };
          crypto3 = nil_crypto3.packages.${pkgs.system}.default;
          blueprint = nil_zkllvm_blueprint.packages.${pkgs.system}.default;
        in
        pkgs.gcc13Stdenv.mkDerivation {
          # TODO: rewrite this using overrideAttr on makeReleaseBuild
          name = "EVM-assigner-tests";

          buildInputs = with pkgs; [
            cmake
            ninja
            gtest
            boost
            ethash
            deps.intx
            deps.evmc
            crypto3
            blueprint
          ];

          src = self;

          cmakeFlags = [
            "-DHUNTER_ENABLED=OFF" # TODO: this will be removed after we get rid of Hunter
            "-DBUILD_ASSIGNER_TESTS=TRUE"
          ];

          dontInstall = true;

          doCheck = true;

          GTEST_OUTPUT = "xml:${placeholder "out"}/test-reports/";

          checkPhase = ''
            ./lib/assigner/test/assigner_tests
          '';
        };

      makeDevShell = { pkgs }:
        let
          deps = nonFlakeDependencies { enable_debug = true; inherit repos pkgs; };
          crypto3 = nil_crypto3.packages.${pkgs.system}.default;
          blueprint = nil_zkllvm_blueprint.packages.${pkgs.system}.default;
        in
        pkgs.mkShell {
          buildInputs = with pkgs; [
            cmake
            ninja
            gtest
            boost
            clang_17
            ethash
            deps.intx
            crypto3
            blueprint
          ];

          shellHook = ''
            echo "evm-assigner dev environment activated"
          '';
        };
    in
    {
      packages = forAllSystems ({ pkgs }: { default = makeReleaseBuild { inherit pkgs; }; });
      checks = forAllSystems ({ pkgs }: { default = makeTests { inherit pkgs; }; });
      devShells = forAllSystems ({ pkgs }: { default = makeDevShell { inherit pkgs; }; });
    };
}
