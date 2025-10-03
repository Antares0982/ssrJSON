# Some dependencies are dropped because of build failure,
# and tests are skipped.
# The others are same as orjson in Nixpkgs
{
  self,
  pkgs,
  pkgs-legacy,
  lib,
  fetchFromGitHub,
  stdenv,
  rustPlatform,
  ...
}:
let
  minorVer = lib.strings.toInt self.python.sourceVersion.minor;
  versionUtils = pkgs.callPackage ./version_utils.nix { inherit pkgs-legacy; };
  pythonVerConfig = versionUtils.pythonVerConfig;
  useNixpkgsUnstable = (minorVer >= pythonVerConfig.latestStableVer);
in
self.buildPythonPackage rec {
  pname = "orjson";
  version = if useNixpkgsUnstable then "3.11.3" else "3.10.13";
  pyproject = true;

  disabled = self.pythonOlder "3.8";

  src = fetchFromGitHub {
    owner = "ijl";
    repo = "orjson";
    rev = version;
    hash =
      if useNixpkgsUnstable then
        "sha256-oTrmDYmUHXMKxgxzBIStw7nnWXcyH9ir0ohnbX4bdjU="
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
            "sha256-y6FmK1RR1DAswVoTlnl19CmoYXAco1dY7lpV/KTypzE="
          else
            "sha256-2YCXJLJ101OaW74okRYtmFazoS4o0n7psXBWJXRaFh4=";
      };

  nativeBuildInputs =
    [ self.cffi ]
    ++ (with rustPlatform; [
      cargoSetupHook
      maturinBuildHook
    ]);

  buildInputs = lib.optionals stdenv.hostPlatform.isDarwin [ self.libiconv ];

  nativeCheckInputs = with self; [
    # numpy
    psutil
    pytestCheckHook
    python-dateutil
    pytz
    # xxhash
  ];

  pythonImportsCheck = [ "orjson" ];
}
