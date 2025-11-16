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
  version = "0.0.7";
  pyproject = true;

  disabled = pypkgs.pythonOlder "3.10";

  # src = pkgs.fetchFromGitHub {
  #   owner = "Nambers";
  #   repo = "ssrJSON-benchmark";
  #   rev = "6207d902010e86cabf99ddc3967926683aea02d8";
  #   sha256 = "sha256-5zIFa7+VkGu1aMrEODFHFDYNt3lMwEk5x453iRigkvs=";
  # };

  src = fetchPypi {
    inherit pname version;
    sha256 = "sha256-C3Emj2fXSZmoV34rqf+fHy1M+Ta00dMRpCX9QOeshqk=";
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
