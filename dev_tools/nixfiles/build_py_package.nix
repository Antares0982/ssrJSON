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
in
buildPythonPackage rec {
  pname = "ssrjson";
  version = builtins.readFile ../../version_file;
  format = "wheel";
  disabled = pypkgs.pythonOlder "3.8";
  src = wheel;
  preUnpack = ''
    src=${wheel}/$(ls ${wheel})
  '';
}
