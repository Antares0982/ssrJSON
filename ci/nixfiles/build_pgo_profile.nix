{
  clangStdenv,
  python,
  cmake,
  lib,
  callPackage,
  useNoGIL ? false,
  sde ? null,
  patchelf,
  llvmPackages,
  ...
}:
let
  system = clangStdenv.hostPlatform.system;
  ssrJSONVersion = callPackage ./ssrjson_version.nix { };
  commonCmakeFlags = [
    "-DPREDEFINED_VERSION=${ssrJSONVersion}"
    "-DBUILD_CTESTS=OFF"
    "-DBUILD_SHIPPING_SIMD=ON"
  ]
  ++ lib.optional useNoGIL "-DBUILD_FREE_THREADING=ON";
  cmakeFlagStr = lib.concatStringsSep " " commonCmakeFlags;
  useSDE = system == "x86_64-linux";
  srcFilter = import ./source_filter.nix { inherit lib; };
in
clangStdenv.mkDerivation {
  pname = "ssrjson-pgo-profile";
  version = ssrJSONVersion;
  src = srcFilter.mkSrc "ssrjson-src";
  nativeBuildInputs = [
    cmake
    python
    patchelf
    llvmPackages.llvm
  ];

  buildPhase = ''
    runHook preBuild

    cmake .. -B pgo-instr \
      -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_PGO_GENERATE=ON \
      ${cmakeFlagStr}
    cmake --build pgo-instr -j $NIX_BUILD_CORES

    mkdir -p pgo_data
  ''
  + (
    if useSDE then
      ''
        ${sde}/bin/sde64 -clx -- ${python}/bin/python ../ci/pgo_train.py \
          --build-dir "$PWD/pgo-instr" --bench-dir "$PWD/../bench" \
          --profile-dir "$PWD/pgo_data/avx512" & pid1=$!
        ${sde}/bin/sde64 -rpl -- ${python}/bin/python ../ci/pgo_train.py \
          --build-dir "$PWD/pgo-instr" --bench-dir "$PWD/../bench" \
          --profile-dir "$PWD/pgo_data/avx2" & pid2=$!
        ${sde}/bin/sde64 -ivb -- ${python}/bin/python ../ci/pgo_train.py \
          --build-dir "$PWD/pgo-instr" --bench-dir "$PWD/../bench" \
          --profile-dir "$PWD/pgo_data/sse42" & pid3=$!
        wait $pid1 > /dev/null 2>&1 || exit $?
        wait $pid2 > /dev/null 2>&1 || exit $?
        wait $pid3 > /dev/null 2>&1 || exit $?
      ''
    else
      ''
        ${python}/bin/python ../ci/pgo_train.py \
        --build-dir "$PWD/pgo-instr" --bench-dir "$PWD/../bench" \
        --profile-dir "$PWD/pgo_data/neon"
      ''
  )
  + ''
    mkdir -p _out
    llvm-profdata merge \
      -o _out/ssrjson.profdata \
      -sparse \
      pgo_data/*/*.profraw

    runHook postBuild
  '';

  installPhase = ''
    runHook preInstall
    mkdir -p $out
    cp _out/ssrjson.profdata $out/
    runHook postInstall
  '';
}
