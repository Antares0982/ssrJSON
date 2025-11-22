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
  useNixpkgsUnstable = (minorVer > pythonVerConfig.latestUseStableNixpkgsVer);
in
pypkgs.buildPythonPackage rec {
  pname = "ssrjson_benchmark";
  version = "0.0.7";
  pyproject = true;

  disabled = pypkgs.pythonOlder "3.10";

  src = pkgs.fetchFromGitHub {
    owner = "Nambers";
    repo = "ssrJSON-benchmark";
    rev = "bac02bd715cb78d54ec4ec298246abb576e53e5e";
    sha256 = "sha256-XQaiHDuqrcI3JNpjvL9emzaPM+qHuXNrJOG4/5eFJj8=";
  };

  # src = fetchPypi {
  #   inherit pname version;
  #   sha256 = "sha256-C3Emj2fXSZmoV34rqf+fHy1M+Ta00dMRpCX9QOeshqk=";
  # };

  build-system = with pypkgs; [ setuptools ];

  nativeBuildInputs = with pypkgs; [
    cmake
  ];

  dependencies =
    with pypkgs;
    [
      matplotlib
      msgspec
      orjson
      reportlab
      svglib
      ujson
    ]
    ++ (lib.optionals (system == "x86_64-linux") [ pypkgs.psutil ]);

  configurePhase = ":";

  pythonRuntimeDepsCheckHook = ":";
}
