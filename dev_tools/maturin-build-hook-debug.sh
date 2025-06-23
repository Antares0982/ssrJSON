maturinBuildHook() {
    echo "Executing maturinBuildHook"

    runHook preBuild

    # Put the wheel to dist/ so that regular Python tooling can find it.
    echo "PWD=$PWD"
    local dist="$PWD/dist"

    if [ ! -z "${buildAndTestSubdir-}" ]; then
        pushd "${buildAndTestSubdir}"
    fi

    (
    set -x
    @setEnv@ maturin build \
        --jobs=$NIX_BUILD_CORES \
        --offline \
        --target @rustTargetPlatformSpec@ \
        --manylinux off \
        --out "$dist" \
        ${maturinBuildFlags-}
    )

    if [ ! -z "${buildAndTestSubdir-}" ]; then
        popd
    fi

    # These are python build hooks and may depend on ./dist
    runHook postBuild

    echo "dist: $dist"
    ls -l $dist

    echo "Finished maturinBuildHook"
}

buildPhase=maturinBuildHook
