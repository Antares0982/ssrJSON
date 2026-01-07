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
  version = "0.0.10";
  pyproject = true;

  disabled = pypkgs.pythonOlder "3.10";

  # src = pkgs.fetchFromGitHub {
  #   owner = "Nambers";
  #   repo = "ssrJSON-benchmark";
  #   rev = "f86c546d0865c9fdf349bb66b28d75736a630dd7";
  #   sha256 = "sha256-eWRmUsbBRaBwOIm5fq2i8M1/MgEU3i4GfVQN6UtrcQg=";
  # };

  src = fetchPypi {
    inherit pname version;
    sha256 = "sha256-I2ET7TftccHjCZpwkVc6E4qk1lW8ao2F9fY1kFnWWw4=";
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
