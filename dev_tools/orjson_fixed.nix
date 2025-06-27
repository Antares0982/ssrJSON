{
  super,
  pkgs,
  pkgs-24-05,
  lib,
  fetchFromGitHub,
  stdenv,
  rustPlatform,
  ...
}:
let
  minorVer = lib.strings.toInt super.python.sourceVersion.minor;
  versionUtils = pkgs.callPackage ./version_utils.nix { inherit pkgs-24-05; };
  pythonVerConfig = versionUtils.pythonVerConfig;
  useNixpkgsUnstable = (minorVer >= pythonVerConfig.latestStableVer);
in
super.buildPythonPackage rec {
  pname = "orjson";
  version = if useNixpkgsUnstable then "3.10.16" else "3.10.13";
  pyproject = true;

  disabled = super.pythonOlder "3.8";

  src = fetchFromGitHub {
    owner = "ijl";
    repo = "orjson";
    rev = version;
    hash =
      if useNixpkgsUnstable then
        "sha256-hgyW3bff70yByxPFqw8pwPMPMAh9FxL1U+LQoJI6INo="
      else
        "sha256-7i4vrVSXJvwqmOsH9OWdeg/VoJeXnzacqhVAcf2Dex8=";
  };

  cargoDeps =
    (if useNixpkgsUnstable then rustPlatform.fetchCargoVendor else pkgs.rustPlatform.fetchCargoTarball)
      {
        inherit src;
        name = "${pname}-${version}";
        hash =
          if useNixpkgsUnstable then
            "sha256-mOHOIKmcXjPwZ8uPth+yvreHG4IpiS6SFhWY+IZS69E="
          else
            "sha256-2YCXJLJ101OaW74okRYtmFazoS4o0n7psXBWJXRaFh4=";
      };

  nativeBuildInputs =
    [ super.cffi ]
    ++ (with rustPlatform; [
      cargoSetupHook
      maturinBuildHook
    ]);

  buildInputs = lib.optionals stdenv.hostPlatform.isDarwin [ super.libiconv ];

  nativeCheckInputs = with super; [
    # numpy
    psutil
    pytestCheckHook
    python-dateutil
    pytz
    # xxhash
  ];

  pythonImportsCheck = [ "orjson" ];

  # passthru.tests = {
  #   inherit (super)
  #     falcon
  #     fastapi
  #     gradio
  #     mashumaro
  #     ufolib2
  #     ;
  # };

  # meta = with lib; {
  #   description = "Fast, correct Python JSON library supporting dataclasses, datetimes, and numpy";
  #   homepage = "https://github.com/ijl/orjson";
  #   changelog = "https://github.com/ijl/orjson/blob/${version}/CHANGELOG.md";
  #   license = with licenses; [
  #     asl20
  #     mit
  #   ];
  #   platforms = platforms.unix;
  #   maintainers = with maintainers; [ misuzu ];
  # };
}
