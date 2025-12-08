{
  description = "ssrjson flake";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    nixpkgs-legacy.url = "github:NixOS/nixpkgs/nixos-25.05";
  };

  outputs =
    inputs@{
      self,
      nixpkgs,
      nixpkgs-legacy,
      ...
    }:
    let
      forAllSystems =
        function:
        nixpkgs.lib.genAttrs
          [
            "x86_64-linux"
            "aarch64-linux"
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
          pkgs-legacy = import nixpkgs-legacy { inherit (pkgs.stdenv.hostPlatform) system; };
          versionUtils = pkgs.callPackage ./dev_tools/nixfiles/version_utils.nix { inherit pkgs-legacy; };
          defaultShell = pkgs.callPackage ./dev_tools/nixfiles/shell.nix {
            inherit pkgs-legacy;
          };
          _drvs = pkgs.callPackage ./dev_tools/nixfiles/_drvs.nix { inherit pkgs-legacy; };
          pythonVerConfig = versionUtils.pythonVerConfig;
          curVer = pythonVerConfig.curVer;
          leastVer = pythonVerConfig.minSupportVer;
          verLength = curVer - leastVer;
          mkMyShell =
            { shell, ... }:
            (
              (shell.overrideAttrs {
                shellHook = pkgs.callPackage ./dev_tools/nixfiles/shellhook.nix {
                  parentShell = shell;
                  inherit pkgs-legacy;
                  inherit (shell) inputDerivation;
                  inherit (_drvs) pyenvs debuggable_py pyenv_nodebug;
                  nix_pyenv_directory = ".nix-devenv";
                  pyenv = builtins.elemAt _drvs.pyenvs verLength;
                  using_python = builtins.elemAt _drvs.using_pythons verLength;
                };
              })
              // {
                super = shell;
              }
            );
          verToBuildEnvDef = ver: {
            name = "buildenv-py3" + (toString ver);
            value = pkgs.mkShell {
              buildInputs = [
                (
                  (builtins.getAttr ("python3" + (toString ver)) (if ver >= 10 then pkgs else pkgs-legacy))
                  .withPackages
                  (
                    pypkgs: with pypkgs; [
                      # this is needed unless `nix build nixpkgs#python314Packages.pip` can run correctly
                      (if ver < 14 then pip else pkgs.callPackage ./dev_tools/nixfiles/py314-pip.nix { inherit pypkgs; })
                      build
                      pytest
                      pytest-random-order
                    ]
                  )
                )
              ]
              ++ (with pkgs; [
                cmake
                clang
              ]);
              hardeningDisable = [ "fortify" ];
            };
          };
        in
        {
          internal = defaultShell;
          default = mkMyShell { shell = defaultShell; };
        }
        // (builtins.listToAttrs (map verToBuildEnvDef versionUtils.versions))
      );
      packages = forAllSystems (
        pkgs:
        let
          pkgs-legacy = import nixpkgs-legacy { inherit (pkgs.stdenv.hostPlatform) system; };
          versionUtils = pkgs.callPackage ./dev_tools/nixfiles/version_utils.nix { inherit pkgs-legacy; };
          pythonVerConfig = versionUtils.pythonVerConfig;
          stablePython = versionUtils.stablePython;
          verToPackageDef = ver: {
            name = "ssrjson-py3" + (builtins.toString ver);
            value = pkgs.callPackage ./dev_tools/nixfiles/build_package.nix {
              python = versionUtils.pyVerToPyPackage ver;
            };
          };
          verToWheelDef = ver: {
            name = "ssrjson-wheel-py3" + (builtins.toString ver);
            value = pkgs.callPackage ./dev_tools/nixfiles/build_wheel.nix {
              python = versionUtils.pyVerToPyPackage ver;
            };
          };
          verToPyPackageDef = ver: {
            name = "ssrjson-pypackage-py3" + (builtins.toString ver);
            value = pkgs.callPackage ./dev_tools/nixfiles/build_py_package.nix rec {
              pypkgs = builtins.getAttr ("python3" + (toString ver) + "Packages") pkgs;
              buildPythonPackage = pypkgs.buildPythonPackage;
            };
          };
          ssrjsonPackages = builtins.listToAttrs (map verToPackageDef versionUtils.versions);
          ssrjsonWheels = builtins.listToAttrs (map verToWheelDef versionUtils.wheelBuildableVersions);
          ssrjsonPyPackages = builtins.listToAttrs (
            map verToPyPackageDef versionUtils.wheelBuildableVersions
          );
          ssrjsonDefaultPackage = builtins.getAttr (
            "ssrjson-pypackage-py3" + (builtins.toString pythonVerConfig.curVer)
          ) ssrjsonPyPackages;
        in
        {
          ssrjson-tarball = pkgs.callPackage ./dev_tools/nixfiles/build_tarball.nix {
            python = stablePython;
          };
          default = ssrjsonDefaultPackage;
        }
        // ssrjsonPackages
        // ssrjsonWheels
        // ssrjsonPyPackages
      );
    };
}
