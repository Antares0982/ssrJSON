{
  lib,
  system,
  callPackage,
  pkgs-legacy,
  verInt, # building python version
  curVer, # default version of development
  orjsonDebug ? false,
  ...
}:
(
  self: super:
  let
    noCheckPackage = (
      name:
      super.${name}.overrideAttrs {
        pytestCheckPhase = ":";
      }
    );
    noCheckPackages = lst: lib.genAttrs lst noCheckPackage;
  in
  {
    orjson =
      if (verInt != curVer || orjsonDebug) then
        (callPackage ./orjson_fixed.nix {
          pypkgs = self;
          inherit pkgs-legacy;
          isDebug = orjsonDebug;
        })
      else
        (callPackage ./orjson-pypi.nix { pypkgs = self; });
    ssrjson-benchmark = callPackage ./ssrjson_benchmark.nix {
      pypkgs = self;
      inherit pkgs-legacy;
    };
  }
  // (lib.optionalAttrs (verInt >= 14) {
    pytest-random-order =
      (super.pytest-random-order.override {
        pytest-xdist = null;
      }).overrideAttrs
        {
          pytestCheckPhase = ":";
        };
  })
  // (lib.optionalAttrs (verInt >= 14 && system == "aarch64-linux") (noCheckPackages [
    "numpy"
    "virtualenv"
  ]))
)
