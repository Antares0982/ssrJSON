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
      platforms = [
        "x86_64-linux"
        "aarch64-linux"
        "aarch64-darwin"
      ];
      nixpkgs = ssrjson-nix-dev.ssrjson-nixpkgs;
      nixpkgs-legacy = ssrjson-nix-dev.ssrjson-nixpkgs-legacy;
      forAllSystems =
        function:
        nixpkgs.lib.genAttrs platforms (
          system:
          function (
            import nixpkgs {
              inherit system;
            }
          )
        );
      ssrjson-devShells = ssrjson-nix-dev.devShells;
      FreeThreadingUpdate = map (p: {
        path = [
          p
          "default"
        ];
        update = old: ssrjson-devShells.${p}.devenv-py314t;
      }) platforms;
      UseFreeThreadingByDefault = false;
    in
    {
      inherit nixpkgs;
      devShells =
        if UseFreeThreadingByDefault then
          (nixpkgs.lib.attrsets.updateManyAttrsByPath FreeThreadingUpdate ssrjson-devShells)
        else
          ssrjson-devShells;
      packages = forAllSystems (
        pkgs:
        let
          nixfiles = builtins.toPath ./dev_tools/nixfiles;
          pkgs-legacy = import nixpkgs-legacy { inherit (pkgs.stdenv.hostPlatform) system; };
          versionUtils = pkgs.callPackage "${nixfiles}/version_utils.nix" { inherit pkgs-legacy; };
          pythonVerConfig = versionUtils.pythonVerConfig;
          stablePython = versionUtils.stablePython;
          verToPackageDef = ver: {
            name = "ssrjson-py3" + (builtins.toString ver);
            value = pkgs.callPackage "${nixfiles}/build_package.nix" {
              python = versionUtils.pyVerToPyPackage ver;
            };
          };
          verToPackageDefNoGIL = ver: {
            name = "ssrjson-py3" + (builtins.toString ver) + "t";
            value = pkgs.callPackage "${nixfiles}/build_package.nix" {
              python = versionUtils.pyVerToPyPackageNoGIL ver;
              useNoGIL = true;
            };
          };
          verToWheelDef = ver: {
            name = "ssrjson-wheel-py3" + (builtins.toString ver);
            value = pkgs.callPackage "${nixfiles}/build_wheel.nix" {
              python = versionUtils.pyVerToPyPackage ver;
            };
          };
          verToWheelDefNoGIL = ver: {
            name = "ssrjson-wheel-py3" + (builtins.toString ver) + "t";
            value = pkgs.callPackage "${nixfiles}/build_wheel.nix" {
              python = versionUtils.pyVerToPyPackageNoGIL ver;
              useNoGIL = true;
            };
          };
          toPackageName = ver: "python3" + (toString ver) + "Packages";
          verToPyPackageDef = ver: {
            name = "ssrjson-pypackage-py3" + (builtins.toString ver);
            value = pkgs.callPackage "${nixfiles}/build_py_package.nix" rec {
              python = versionUtils.pyVerToPyPackage ver;
            };
          };
          verToPyPackageDefNoGIL = ver: {
            name = "ssrjson-pypackage-py3" + (builtins.toString ver) + "t";
            value = pkgs.callPackage "${nixfiles}/build_py_package.nix" rec {
              python = versionUtils.pyVerToPyPackageNoGIL ver;
              useNoGIL = true;
            };
          };
          ssrjsonPackages = builtins.listToAttrs (map verToPackageDef versionUtils.versions);
          ssrjsonPackagesNoGIL = builtins.listToAttrs (
            map verToPackageDefNoGIL versionUtils.versionsSupportNoGIL
          );
          ssrjsonWheels = builtins.listToAttrs (map verToWheelDef versionUtils.wheelBuildableVersions);
          ssrjsonWheelsNoGIL = builtins.listToAttrs (
            map verToWheelDefNoGIL versionUtils.wheelBuildableVersionsSupportNoGIL
          );
          ssrjsonPyPackages = builtins.listToAttrs (
            map verToPyPackageDef versionUtils.wheelBuildableVersions
          );
          ssrjsonPyPackagesNoGIL = builtins.listToAttrs (
            map verToPyPackageDefNoGIL versionUtils.wheelBuildableVersionsSupportNoGIL
          );
          ssrjsonDefaultPackage = builtins.getAttr (
            "ssrjson-pypackage-py3" + (builtins.toString pythonVerConfig.curVer)
          ) ssrjsonPyPackages;
        in
        {
          ssrjson-tarball = pkgs.callPackage "${nixfiles}/build_tarball.nix" {
            python = stablePython;
          };
          default = ssrjsonDefaultPackage;
        }
        // ssrjsonPackages
        // ssrjsonWheels
        // ssrjsonPyPackages
        // ssrjsonPackagesNoGIL
        // ssrjsonWheelsNoGIL
        // ssrjsonPyPackagesNoGIL
      );
    };
}
