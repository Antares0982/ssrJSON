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
  version = "0.0.9";
  pyproject = true;

  disabled = pypkgs.pythonOlder "3.10";

  # src = pkgs.fetchFromGitHub {
  #   owner = "Nambers";
  #   repo = "ssrJSON-benchmark";
  #   rev = "0cab0745e486b7f61559d50ea2dada34a477cc2e";
  #   sha256 = "sha256-wmiSxAlPMHgtnUnr0RlyXVSQaRxfmtVPh8cU0vmQtgw=";
  # };

  src = fetchPypi {
    inherit pname version;
    sha256 = "sha256-tcmS0HLaKIECxD9QftdzfEdFd8hKzKozZGQh5z1oU1Y=";
  };

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
