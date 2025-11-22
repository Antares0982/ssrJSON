# Some dependencies are dropped because of build failure,
# and tests are skipped.
# The others are same as orjson in Nixpkgs
{
  pypkgs,
  pkgs,
  pkgs-legacy,
  lib,
  fetchFromGitHub,
  stdenv,
  rustPlatform,
  isDebug ? false,
  ...
}:
let
  minorVer = lib.strings.toInt pypkgs.python.sourceVersion.minor;
  versionUtils = pkgs.callPackage ./version_utils.nix { inherit pkgs-legacy; };
  pythonVerConfig = versionUtils.pythonVerConfig;
  useNixpkgsUnstable = (minorVer > pythonVerConfig.latestUseStableNixpkgsVer);
  oldstdenv = stdenv;
in
pypkgs.buildPythonPackage rec {
  pname = "orjson";
  version = if useNixpkgsUnstable then "3.11.3" else "3.10.13";
  pyproject = true;

  disabled = pypkgs.pythonOlder "3.10";

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

  nativeBuildInputs = [
    pypkgs.cffi
  ]
  ++ (
    with rustPlatform;
    [
      cargoSetupHook
    ]
    ++ (
      if useNixpkgsUnstable then
        (
          if isDebug then
            [
              (pkgs.callPackage (
                { pkgsHostTarget }:
                pkgs.makeSetupHook {
                  name = "maturin-build-hook-debug.sh";
                  propagatedBuildInputs = [
                    pkgsHostTarget.maturin
                    pkgsHostTarget.cargo
                    pkgsHostTarget.rustc
                  ];
                  substitutions = {
                    inherit (stdenv.targetPlatform.rust) rustcTarget;
                    inherit (pkgs.rust.envVars) setEnv;
                  };
                } ./maturin-build-hook-debug.sh
              ) { })
            ]
          else
            [
              maturinBuildHook
            ]
        )
      else
        [ maturinBuildHook ]
    )
  );

  stdenv = pkgs.stdenvAdapters.keepDebugInfo oldstdenv;

  buildInputs = lib.optionals stdenv.hostPlatform.isDarwin [ pypkgs.libiconv ];

  nativeCheckInputs = with pypkgs; [
    # numpy
    pytestCheckHook
    python-dateutil
    pytz
    # xxhash
  ];

  pythonImportsCheck = [ "orjson" ];
}
