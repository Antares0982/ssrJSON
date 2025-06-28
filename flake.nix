{
  description = "ssrjson flake";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    nixpkgs-24-05.url = "github:NixOS/nixpkgs/nixos-24.05";
  };

  outputs =
    inputs@{
      self,
      nixpkgs,
      nixpkgs-24-05,
      ...
    }:
    let
      forAllSystems =
        function:
        nixpkgs.lib.genAttrs
          [
            "x86_64-linux"
            "aarch64-linux"
            "x86_64-darwin"
            "aarch64-darwin"
          ]
          (
            system:
            function (
              import nixpkgs {
                inherit system;
              }
            )
          );

    in
    {
      devShells = forAllSystems (
        pkgs:
        let
          pkgs-24-05 = import nixpkgs-24-05 { inherit (pkgs) system; };
          versionUtils = pkgs.callPackage ./dev_tools/version_utils.nix { inherit pkgs-24-05; };
          defaultShell = pkgs.callPackage ./dev_tools/shell.nix {
            inherit pkgs-24-05;
            debugLLVM = false;
          };
          debugLLVMInternal = pkgs.callPackage ./dev_tools/shell.nix {
            inherit pkgs-24-05;
            debugLLVM = true;
          };
          _drvs = pkgs.callPackage ./dev_tools/_drvs.nix { inherit pkgs-24-05; };
          pythonVerConfig = versionUtils.pythonVerConfig;
          curVer = pythonVerConfig.curVer;
          leastVer = pythonVerConfig.minSupportVer;
          verLength = curVer - leastVer;
          mkMyShell =
            { shell, ... }:
            (
              (shell.overrideAttrs {
                shellHook = pkgs.callPackage ./dev_tools/shellhook.nix {
                  parentShell = shell;
                  inherit pkgs-24-05;
                  inherit (shell) inputDerivation;
                  inherit (_drvs) pyenvs;
                  nix_pyenv_directory = if shell.debugLLVM then ".nix-pyenv-llvm" else ".nix-pyenv";
                  pyenv = builtins.elemAt _drvs.pyenvs verLength;
                  using_python = builtins.elemAt _drvs.using_pythons verLength;
                };
              })
              // {
                super = shell;
              }
            );
        in
        {
          internal = defaultShell;
          default = mkMyShell { shell = defaultShell; };
          inherit debugLLVMInternal;
          debugLLVM = mkMyShell { shell = debugLLVMInternal; };
        }
      );
      packages = forAllSystems (
        pkgs:
        let
          pkgs-24-05 = import nixpkgs-24-05 { inherit (pkgs) system; };
          versionUtils = pkgs.callPackage ./dev_tools/version_utils.nix { inherit pkgs-24-05; };
          pythonVerConfig = versionUtils.pythonVerConfig;
          stablePython = versionUtils.stablePython;
          verToPackageDef = ver: {
            name = "ssrjson-py3" + (builtins.toString ver);
            value = pkgs.callPackage ./dev_tools/build_package.nix {
              python = versionUtils.pyVerToPyPackage ver;
            };
          };
          verToWheelDef = ver: {
            name = "ssrjson-wheel-py3" + (builtins.toString ver);
            value = pkgs.callPackage ./dev_tools/build_wheel.nix {
              python = versionUtils.pyVerToPyPackage ver;
            };
          };
          ssrjsonPackages = builtins.listToAttrs (map verToPackageDef versionUtils.versions);
          ssrjsonWheels = builtins.listToAttrs (map verToWheelDef versionUtils.wheelBuildableVersions);
        in
        {
          ssrjson-tarball = pkgs.callPackage ./dev_tools/build_tarball.nix { python = stablePython; };
          default = ssrjsonPackages.ssrjson-py313;
        }
        // ssrjsonPackages
        // ssrjsonWheels
      );
    };
}
