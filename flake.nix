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
      sde = ssrjson-nix-dev.packages.x86_64-linux.sde;
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
          nixfiles = builtins.toPath ./ci/nixfiles;
          pkgs-legacy = import nixpkgs-legacy { inherit (pkgs.stdenv.hostPlatform) system; };
          versionUtils = pkgs.callPackage "${nixfiles}/version_utils.nix" { inherit pkgs-legacy; };
          pythonVerConfig = versionUtils.pythonVerConfig;
          stablePython = versionUtils.stablePython;
          verToPackageDef = ver: {
            name = "ssrjson-py3" + (builtins.toString ver);
            value = pkgs.callPackage "${nixfiles}/build_package.nix" {
              python = versionUtils.pyVerToPyPackage ver;
              inherit sde;
            };
          };
          verToPackageDefNoGIL = ver: {
            name = "ssrjson-py3" + (builtins.toString ver) + "t";
            value = pkgs.callPackage "${nixfiles}/build_package.nix" {
              python = versionUtils.pyVerToPyPackageNoGIL ver;
              useNoGIL = true;
              inherit sde;
            };
          };
          verToPgoProfileDef = ver: {
            name = "ssrjson-pgo-profile-py3" + (builtins.toString ver);
            value = pkgs.callPackage "${nixfiles}/build_pgo_profile.nix" {
              python = versionUtils.pyVerToPyPackage ver;
              inherit sde;
            };
          };
          verToPgoProfileDefNoGIL = ver: {
            name = "ssrjson-pgo-profile-py3" + (builtins.toString ver) + "t";
            value = pkgs.callPackage "${nixfiles}/build_pgo_profile.nix" {
              python = versionUtils.pyVerToPyPackageNoGIL ver;
              useNoGIL = true;
              inherit sde;
            };
          };
          verToWheelDef = ver: {
            name = "ssrjson-wheel-py3" + (builtins.toString ver);
            value = pkgs.callPackage "${nixfiles}/build_wheel.nix" {
              python = versionUtils.pyVerToPyPackage ver;
              inherit sde;
            };
          };
          verToWheelDefNoGIL = ver: {
            name = "ssrjson-wheel-py3" + (builtins.toString ver) + "t";
            value = pkgs.callPackage "${nixfiles}/build_wheel.nix" {
              python = versionUtils.pyVerToPyPackageNoGIL ver;
              useNoGIL = true;
              inherit sde;
            };
          };
          toPackageName = ver: "python3" + (toString ver) + "Packages";
          verToPyPackageDef = ver: {
            name = "ssrjson-pypackage-py3" + (builtins.toString ver);
            value = pkgs.callPackage "${nixfiles}/build_py_package.nix" rec {
              python = versionUtils.pyVerToPyPackage ver;
              inherit sde;
            };
          };
          verToPyPackageDefNoGIL = ver: {
            name = "ssrjson-pypackage-py3" + (builtins.toString ver) + "t";
            value = pkgs.callPackage "${nixfiles}/build_py_package.nix" rec {
              python = versionUtils.pyVerToPyPackageNoGIL ver;
              useNoGIL = true;
              inherit sde;
            };
          };
          ssrjsonPackages = builtins.listToAttrs (map verToPackageDef versionUtils.versions);
          ssrjsonPackagesNoGIL = builtins.listToAttrs (
            map verToPackageDefNoGIL versionUtils.versionsSupportNoGIL
          );
          ssrjsonPgoProfiles = builtins.listToAttrs (
            map verToPgoProfileDef versionUtils.wheelBuildableVersions
          );
          ssrjsonPgoProfilesNoGIL = builtins.listToAttrs (
            map verToPgoProfileDefNoGIL versionUtils.wheelBuildableVersionsSupportNoGIL
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
        // ssrjsonPgoProfiles
        // ssrjsonPgoProfilesNoGIL
      );
    };
}
