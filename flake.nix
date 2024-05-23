{
  description = "Nix flake for EVM-assigner";

  inputs = {
    nixpkgs = { url = "github:NixOS/nixpkgs/nixos-23.11"; };
    nil_crypto3 = {
      url = "https://github.com/NilFoundation/crypto3";
      type = "git";
      rev = "3de0775395bf06c0e4969ff7f921cc7523904269";
      submodules = true;
    };
    nil_zkllvm_blueprint = {
      url = "https://github.com/NilFoundation/zkllvm-blueprint";
      type = "git";
      rev = "ba5c1638ee5c222c7572cac511073cfc19b0a5ee";
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

      resolvePackage = system: is_debug: input:
        let
          base = input.packages.${system};
          debug_alternative = if base ? debug then base.debug else base.default;
          release_alternative = if base ? release then base.release else base.default;
        in
          if is_debug then debug_alternative else release_alternative;

      flake_inputs = [
        nil_crypto3
        nil_zkllvm_blueprint
      ];

      flake_packages = pkgs: is_debug:
        map (resolvePackage pkgs.system is_debug) flake_inputs;

      nonFlakeDependencies = { pkgs, enable_debug }:
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

      commonBuildInputs = pkgs: with pkgs; [
        cmake
        ninja
        gtest
        boost
      ];

      inputsToPropagate = pkgs: enable_debug:
        let
          resolvedDeps = nonFlakeDependencies {inherit pkgs enable_debug; };
        in
          with resolvedDeps; [ intx evmc ] ++ [ pkgs.ethash ] ++ flake_packages pkgs enable_debug;

      allBuildInputs = pkgs: enable_debug: commonBuildInputs pkgs ++ inputsToPropagate pkgs enable_debug;

      makePackage = { pkgs, enable_debug ? false }:
        pkgs.gcc13Stdenv.mkDerivation {
          name = "EVM-assigner${if enable_debug then "-debug" else ""}";

          buildInputs = commonBuildInputs pkgs;

          propagatedBuildInputs = inputsToPropagate pkgs false;

          src = self;

          cmakeBuildType = if enable_debug then "Debug" else "Release";

          cmakeFlags = [
            "-DHUNTER_ENABLED=OFF" # TODO: this will be removed after we get rid of Hunter
          ];

          doCheck = false;
          dontStrip = enable_debug;
        };

      makeTests = { pkgs, enable_debug ? false }:
        pkgs.gcc13Stdenv.mkDerivation {
          name = "EVM-assigner-tests";

          buildInputs = allBuildInputs pkgs enable_debug;

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

      makeDevShell = { pkgs, enable_debug ? false }:
        pkgs.mkShell {
          buildInputs = allBuildInputs pkgs false;

          shellHook = ''
            echo "evm-assigner dev environment activated"
          '';
        };
    in
    {
      packages = forAllSystems ({ pkgs }: rec {
        release = makePackage { inherit pkgs; };
        debug = makePackage { enable_debug = true; inherit pkgs; };
        default = release;
        });
      checks = forAllSystems ({ pkgs }: { default = makeTests { inherit pkgs; }; });
      devShells = forAllSystems ({ pkgs }: { default = makeDevShell { inherit pkgs; }; });
    };
}
