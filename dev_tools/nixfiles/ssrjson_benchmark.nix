{
  pypkgs,
  pkgs,
  pkgs-legacy,
  lib,
  fetchPypi,
  cmake,
  ...
}:
let
  minorVer = lib.strings.toInt pypkgs.python.sourceVersion.minor;
  versionUtils = pkgs.callPackage ./version_utils.nix { inherit pkgs-legacy; };
  pythonVerConfig = versionUtils.pythonVerConfig;
  useNixpkgsUnstable = (minorVer >= pythonVerConfig.latestStableVer);
in
pypkgs.buildPythonPackage rec {
  pname = "ssrjson_benchmark";
  version = "0.0.5";
  pyproject = true;

  disabled = pypkgs.pythonOlder "3.10";

  # src = pkgs.fetchFromGitHub {
  #   owner = "Nambers";
  #   repo = "ssrJSON-benchmark";
  #   rev = "14b7ef1ee694b27af42df76dbfdd7d6c08fbc818";
  #   sha256 = "sha256-2jsbit0G7UEIUrCEaQbI6O8gURRmX/APmFsWNCjiJQo=";
  # };

  src = fetchPypi {
    inherit pname version;
    sha256 = "sha256-EdNUXcbZk8P6U15I2vPRzDuzqZVuddKJ3q4oBtglRHo=";
  };

  build-system = with pypkgs; [ setuptools ];

  nativeBuildInputs = with pypkgs; [
    cmake
  ];

  dependencies =
    with pypkgs;
    [
      matplotlib
      orjson
      reportlab
      svglib
      ujson
    ]
    ++ (lib.optionals (pkgs.system == "x86_64-linux") [ pypkgs.psutil ]);

  configurePhase = ":";

  pythonRuntimeDepsCheckHook = ":";
}
