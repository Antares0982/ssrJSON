{
  description = "ssrjson flake";

  inputs = {
    ssrjson-nix-dev.url = "github:Antares0982/ssrjson-nix-dev/main";
  };

  outputs =
    inputs@{
      self,
      ssrjson-nix-dev,
      ...
    }:
    let
      nixpkgs = ssrjson-nix-dev.ssrjson-nixpkgs;
      nixpkgs-legacy = ssrjson-nix-dev.ssrjson-nixpkgs-legacy;
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
      inherit (ssrjson-nix-dev) devShells;
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
