pypkgs:
let
  pkgs = pypkgs.pkgs;
  lib = pkgs.lib;
  minorVer = lib.strings.toInt pypkgs.python.sourceVersion.minor;
  pythonVerConfig = lib.importJSON ./pyver.json;
  useNixpkgsUnstable = (minorVer >= pythonVerConfig.latestStableVer);
in
with pypkgs;
[
  objgraph
  psutil
  pytz
  pytest
  pytest-random-order
]
++ (
  with pypkgs; # needed by tests, but cannot be built in python3.14
  (lib.optionals (minorVer < 14) [
    arrow
    orjson
    pip
    pytest-xdist
  ])
)
# some dependecies of orjson cannot be built in python3.14
# ++ (lib.optionals (minorVer < 14) [
#   (
#     (pypkgs.buildPythonPackage rec {
#       pname = "orjson";
#       version = if useNixpkgsUnstable then "3.10.15" else "3.10.1";
#       pyproject = true;
#       useFetchCargoVendor = true;

#       disabled = pythonOlder "3.8";

#       src = pkgs.fetchFromGitHub {
#         owner = "ijl";
#         repo = "orjson";
#         rev = "refs/tags/${version}";
#         hash = if useNixpkgsUnstable then "sha256-FlcWf6BhUP2Y5ivRQx1W0G8sgfvbuAQN7qpBJbd3N2I=" else "";
#       };

#       cargoDeps =
#         (
#           if useNixpkgsUnstable then
#             (pkgs.rustPlatform.fetchCargoVendor)
#           else
#             (pkgs.rustPlatform.fetchCargoTarball)
#         )
#           {
#             inherit src;
#             name = "${pname}-${version}";
#             hash =
#               if useNixpkgsUnstable then
#                 "sha256-fHp5Rh2Mzn62ZUoVHETl/6kZ6Iztxkd5mjxira7NVBU="
#               else
#                 "sha256-YvZl0zYuUBTIBAdIh6IDR3vIWlk5ye5e3cLB0j/41pk=";
#           };

#       maturinBuildFlags = [ "--interpreter ${python.executable}" ];

#       nativeBuildInputs =
#         [ cffi ]
#         ++ (with pkgs.rustPlatform; [
#           cargoSetupHook
#           (pypkgs.callPackage (
#             { pkgsHostTarget }:
#             pkgs.makeSetupHook {
#               name = "maturin-build-hook.sh";
#               propagatedBuildInputs = [
#                 pkgsHostTarget.maturin
#                 pkgsHostTarget.cargo
#                 pkgsHostTarget.rustc
#                 pypkgs.wrapPython
#               ];
#               substitutions = {
#                 inherit (pkgs.rust.envVars) rustTargetPlatformSpec setEnv;
#               };
#             } "${pkgs.path}/pkgs/build-support/rust/hooks/maturin-build-hook.sh"
#           ) { })
#         ]);

#       buildInputs = lib.optionals stdenv.isDarwin [ libiconv ];

#       nativeCheckInputs = [
#         psutil
#         pytestCheckHook
#         python-dateutil
#         pytz
#         # xxhash
#       ];

#       preBuild = ''
#         cp -r . ../orjson
#         cd ../orjson
#       '';

#       pythonImportsCheck = [ "orjson" ];

#       passthru.tests = {
#         inherit
#           falcon
#           fastapi
#           gradio
#           mashumaro
#           ufolib2
#           ;
#       };
#     })
#   )
# ])
# benchmark is only needed for python3.13
++ (with pypkgs; (lib.optionals (minorVer == pythonVerConfig.latestStableVer) [ pytest-benchmark ]))
