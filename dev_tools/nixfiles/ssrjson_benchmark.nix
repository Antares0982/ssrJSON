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
  system = pkgs.stdenv.hostPlatform.system;
  minorVer = lib.strings.toInt pypkgs.python.sourceVersion.minor;
  versionUtils = pkgs.callPackage ./version_utils.nix { inherit pkgs-legacy; };
  pythonVerConfig = versionUtils.pythonVerConfig;
  useNixpkgsUnstable = (minorVer >= pythonVerConfig.latestStableVer);
in
pypkgs.buildPythonPackage rec {
  pname = "ssrjson_benchmark";
  version = "0.0.6";
  pyproject = true;

  disabled = pypkgs.pythonOlder "3.10";

  # src = pkgs.fetchFromGitHub {
  #   owner = "Nambers";
  #   repo = "ssrJSON-benchmark";
  #   rev = "aadba2eaaed1fb53c2330d18b8ee03b715c10214";
  #   sha256 = "sha256-yRnD88aPiq60z7/DTupk11DBHR11OAHdIspsmxIVgP8=";
  # };

  src = fetchPypi {
    inherit pname version;
    sha256 = "sha256-OkZfxvJGVuWE1/GyyAQ6JzyD4EbCHcvFNDEgSyL42T4=";
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
    ++ (lib.optionals (system == "x86_64-linux") [ pypkgs.psutil ]);

  configurePhase = ":";

  pythonRuntimeDepsCheckHook = ":";
}
