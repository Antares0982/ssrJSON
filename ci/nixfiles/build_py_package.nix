{
  python,
  pkgs,
  cmake,
  callPackage,
  useNoGIL ? false,
  ...
}:
let
  wheel = callPackage ./build_wheel.nix {
    inherit python useNoGIL;
    forNonNix = false;
  };
  ssrJSONVersion = callPackage ./ssrjson_version.nix { };
in
python.pkgs.buildPythonPackage rec {
  pname = "ssrjson";
  version = ssrJSONVersion;
  format = "wheel";
  src = wheel;
  preUnpack = ''
    src=${wheel}/$(ls ${wheel})
  '';
}
