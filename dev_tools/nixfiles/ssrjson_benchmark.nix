{
  self,
  pkgs,
  pkgs-24-05,
  lib,
  fetchPypi,
  cmake,
  ...
}:
let
  minorVer = lib.strings.toInt self.python.sourceVersion.minor;
  versionUtils = pkgs.callPackage ./version_utils.nix { inherit pkgs-24-05; };
  pythonVerConfig = versionUtils.pythonVerConfig;
  useNixpkgsUnstable = (minorVer >= pythonVerConfig.latestStableVer);
in
self.buildPythonPackage rec {
  pname = "ssrjson_benchmark";
  version = "0.0.2";
  pyproject = true;

  disabled = self.pythonOlder "3.8";

  src = fetchPypi {
    inherit pname version;
    sha256 = "sha256-DggYqXL3DwXvwf78tMcGWP6R/nhmcCNGQB/gZn1G5mQ=";
  };

  build-system = with self; [ setuptools ];

  nativeBuildInputs = with self; [
    cmake
  ];

  dependencies = with self; [
    matplotlib
    orjson
    psutil
    reportlab
    svglib
  ];

  configurePhase = ":";

  pythonRuntimeDepsCheckHook = ":";
}
