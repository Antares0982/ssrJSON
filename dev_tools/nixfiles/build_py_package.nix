{
  pypkgs,
  pkgs,
  cmake,
  buildPythonPackage,
  ...
}:
let
  wheel = pkgs.callPackage ./build_wheel.nix {
    inherit (pypkgs) python;
    forNonNix = false;
  };
  ssrJSONVersion = pkgs.callPackage ./ssrjson_version.nix { };
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
