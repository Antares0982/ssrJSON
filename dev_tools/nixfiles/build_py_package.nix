{
  pypkgs,
  pkgs,
  cmake,
  buildPythonPackage,
  callPackage,
  ...
}:
let
  wheel = callPackage ./build_wheel.nix {
    inherit (pypkgs) python;
    forNonNix = false;
  };
  ssrJSONVersion = callPackage ./ssrjson_version.nix { };
in
buildPythonPackage rec {
  pname = "ssrjson";
  version = ssrJSONVersion;
  format = "wheel";
  disabled = pypkgs.pythonOlder "3.10";
  src = wheel;
  preUnpack = ''
    src=${wheel}/$(ls ${wheel})
  '';
}
