{
  pkgs,
  callPackage,
  pkgs-legacy,
  stdenvNoCC,
  pyenv,
  pyenvs,
  using_python,
  debuggable_py,
  sitePackagesString,
  inputDerivation,
  cmake,
  llvmPackages,
  clang-tools,
  pyenv_nodebug,
  cmake-format,
  ...
}:
let
  system = pkgs.stdenv.hostPlatform.system;
  llvmClang = llvmPackages.libcxxClang;
  libcxx = llvmPackages.libcxx;
  debugSourceDir = "debug_source";
  versionUtils = callPackage ./version_utils.nix { inherit pkgs-legacy; };
  versions = versionUtils.versions;
  minSupportVer = versionUtils.pythonVerConfig.minSupportVer;
  curVer = versionUtils.pythonVerConfig.curVer;
  link_python_cmd =
    ver:
    let
      python_env = builtins.elemAt pyenvs (ver - minSupportVer);
      debuggable_python = builtins.elemAt debuggable_py (ver - minSupportVer);
      dev_python = callPackage ./dev_python.nix {
        pyenv_with_site_packages = python_env;
        inherit debuggable_python ver;
      };
    in
    ''
        ln -s "${dev_python}/bin/python3.${builtins.toString ver}" "$out/bin/${python_env.executable}"
        # creating python library symlinks
        NIX_LIB_DIR="$out/lib/${python_env.libPrefix}"
        mkdir -p "$NIX_LIB_DIR"
        # adding site packages
        for file in ${python_env}/${python_env.sitePackages}/*; do
            basefile=$(basename "$file")
            if [ -d "$file" ]; then
                if [[ "$basefile" != *dist-info && "$basefile" != __pycache__ ]]; then
                    ln -s "$file" "$NIX_LIB_DIR/$basefile"
                fi
            else
                # the typing_extensions.py will make the vscode type checker not working!
                if [[ $basefile == *.so ]] || ([[ $basefile == *.py ]] && [[ $basefile != typing_extensions.py ]]); then
                    ln -s "$file" "$NIX_LIB_DIR/$basefile"
                fi
            fi
        done
        for file in $NIX_LIB_DIR/*; do
            if [[ -L "$file" ]] && [[ "$(dirname $(readlink "$file"))" != "${python_env}/${python_env.sitePackages}" ]]; then
                rm -f "$file"
            fi
        done
        # ensure the typing_extensions.py is not in the lib directory
        rm -f "$NIX_LIB_DIR/typing_extensions.py"
        unset NIX_LIB_DIR

      # TODO
      #   mkdir -p "${debugSourceDir}"
      #   if [[ ! -d ${debugSourceDir}/Python-${debuggable_python.version} ]]; then
      #     cp -r ${debuggable_python.src} ${debugSourceDir}/Python-${debuggable_python.version}
      #     chmod -R 755 ${debugSourceDir}/Python-${debuggable_python.version}
      #     rm -rf ${debugSourceDir}/Python-${debuggable_python.version}/Doc
      #     rm -rf ${debugSourceDir}/Python-${debuggable_python.version}/Grammar
      #     rm -rf ${debugSourceDir}/Python-${debuggable_python.version}/Lib
      #   fi
    '';
  pythonpathEnvLiteral = "\${" + "PYTHONPATH+x}";
  runSdeClxPath = "$out/bin/run-sde-clx";
  runSdeRplPath = "$out/bin/run-sde-rpl";
  runSdeIvbPath = "$out/bin/run-sde-ivb";
  sdeScript = ''
    if [ -z ${pythonpathEnvLiteral} ]; then
        PYTHONPATH=$(pwd)/build exec @sde64@ @cpuid@ -- "$@"
    else
        exec @sde64@ @cpuid@ -- "$@"
    fi
  '';
  sde = pkgs.callPackage ./sde.nix { };
  sde64Path = pkgs.lib.optionalString (system == "x86_64-linux") "${sde}/bin/sde64";
  sdeClxScript = builtins.replaceStrings [ "@cpuid@" "@sde64@" ] [ "-clx" sde64Path ] sdeScript;
  sdeRplScript = builtins.replaceStrings [ "@cpuid@" "@sde64@" ] [ "-rpl" sde64Path ] sdeScript;
  sdeIvbScript = builtins.replaceStrings [ "@cpuid@" "@sde64@" ] [ "-ivb" sde64Path ] sdeScript;
in
stdenvNoCC.mkDerivation {
  name = "ssrjson-dev-env";
  script = "";

  phases = [ "installPhase" ];

  installPhase = ''
    mkdir -p $out/bin
    mkdir -p $out/lib
    mkdir -p $out/nix-support
  ''
  # creating python library symlinks
  + (pkgs.lib.strings.concatStrings (builtins.map link_python_cmd versions))
  + ''
    # bin
    ln -s "${pyenv_nodebug}/bin/python" "$out/bin/python_nodebug"
    ln -s "${llvmClang}/bin/clang" "$out/bin/clang"
    ln -s "${llvmClang}/bin/clang++" "$out/bin/clang++"
    ln -s "${cmake}/bin/cmake" "$out/bin/cmake"
    ln -s "${clang-tools}/bin/clang-format" "$out/bin/clang-format"
    ln -s "${cmake-format}/bin/cmake-format" "$out/bin/cmake-format"
    ln -s "$out/bin/python3.${toString curVer}" "$out/bin/python"
    # lib
    ln -s "$(readlink -f $(${pkgs.gcc}/bin/gcc -print-file-name=libasan.so))" "$out/lib/libasan.so"
    # nix-support
    ln -s "${inputDerivation}" "$out/nix-support/nix-shell-inputs"
    LIBSTDCXX=$(dirname $(readlink -f $(${pkgs.gcc}/bin/gcc -print-file-name=libstdc++.so)))
    echo "export CC=${llvmClang}/bin/clang" > "$out/nix-support/shell-env"
    echo "export CXX=${llvmClang}/bin/clang++" >> "$out/nix-support/shell-env"
    echo "export LD_LIBRARY_PATH=$LIBSTDCXX:\$LD_LIBRARY_PATH" >> "$out/nix-support/shell-env"
    echo "export LIBRARY_PATH=${libcxx}/lib:\$LIBRARY_PATH" >> "$out/nix-support/shell-env"
    echo "export Python3_ROOT_DIR=${using_python}" >> "$out/nix-support/shell-env"
  ''
  # SDE (x86_64)
  + pkgs.lib.optionalString (system == "x86_64-linux") ''
    # sde wrapper script
    cat > "${runSdeClxPath}" << 'EOF'
    ${sdeClxScript}
    EOF
    chmod +x "${runSdeClxPath}"
    #
    cat > "${runSdeRplPath}" << 'EOF'
    ${sdeRplScript}
    EOF
    chmod +x "${runSdeRplPath}"
    #
    cat > "${runSdeIvbPath}" << 'EOF'
    ${sdeIvbScript}
    EOF
    chmod +x "${runSdeIvbPath}"
  '';
}
