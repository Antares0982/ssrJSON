{
  parentShell,
  nix_pyenv_directory,
  pyenv,
  pyenvs,
  debuggable_py,
  pyenv_nodebug,
  using_python,
  pkgs,
  pkgs-24-05,
  inputDerivation,
  lib,
}:
let
  versionUtils = pkgs.callPackage ./version_utils.nix { inherit pkgs-24-05; };
  versions = versionUtils.versions;
  debugSourceDir = "debug_source";
  path_concate = x: builtins.toString "${x}";
  env_concate = builtins.map path_concate pyenvs;
  minSupportVer = versionUtils.pythonVerConfig.minSupportVer;
  latestStableVer = versionUtils.pythonVerConfig.latestStableVer;
  link_python_cmd =
    ver:
    let
      python_env = builtins.elemAt pyenvs (ver - minSupportVer);
      debuggable_python = builtins.elemAt debuggable_py (ver - minSupportVer);
      dev_python = pkgs.callPackage ./dev_python.nix {
        pyenv_with_site_packages = python_env;
        inherit debuggable_python ver;
      };
    in
    ''
      ensure_symlink ${nix_pyenv_directory}/bin/${python_env.executable} ${dev_python}/bin/python3.${builtins.toString ver}
      # echo "PYTHONPATH=\$PYTHONPATH:${python_env}/${python_env.sitePackages} exec ${debuggable_python}/bin/${debuggable_python.executable} \"\$@\"" > ${nix_pyenv_directory}/bin/${python_env.executable}
      # chmod 755 ${nix_pyenv_directory}/bin/${python_env.executable}
      # ensure_symlink ${nix_pyenv_directory}/bin/${python_env.executable} ${python_env.interpreter}
      # creating python library symlinks
      NIX_LIB_DIR=${nix_pyenv_directory}/lib/${python_env.libPrefix}
      mkdir -p $NIX_LIB_DIR
      # adding site packages
      for file in ${python_env}/${python_env.sitePackages}/*; do
          basefile=$(basename $file)
          if [ -d "$file" ]; then
              if [[ "$basefile" != *dist-info && "$basefile" != __pycache__ ]]; then
                  ensure_symlink "$NIX_LIB_DIR/$basefile" $file
              fi
          else
              # the typing_extensions.py will make the vscode type checker not working!
              if [[ $basefile == *.so ]] || ([[ $basefile == *.py ]] && [[ $basefile != typing_extensions.py ]]); then
                  ensure_symlink "$NIX_LIB_DIR/$basefile" $file
              fi
          fi
      done
      for file in $NIX_LIB_DIR/*; do
          if [[ -L "$file" ]] && [[ "$(dirname $(readlink "$file"))" != "${python_env}/${python_env.sitePackages}" ]]; then
              rm -f "$file"
          fi
      done
      # ensure the typing_extensions.py is not in the lib directory
      rm $NIX_LIB_DIR/typing_extensions.py > /dev/null 2>&1
      unset NIX_LIB_DIR

      mkdir -p ${debugSourceDir}
      if [[ ! -d ${debugSourceDir}/Python-${python_env.python.version} ]]; then
        tar xvf ${python_env.python.src} -C ${debugSourceDir} --exclude='Doc' --exclude='Grammar' --exclude='Lib' > /dev/null 2>&1
        chmod -R 755 ${debugSourceDir}/Python-${python_env.python.version}
      fi
    '';
  orjsonSource =
    lib.optionalString (using_python.sourceVersion.minor != "14")
      (builtins.elemAt (builtins.filter (x: x.pname == "orjson") (
        (pkgs.callPackage ./py_requirements.nix { inherit pkgs-24-05; }) using_python.pkgs
      )) 0).src;
  pythonpathEnvLiteral = "\${" + "PYTHONPATH+x}";
  sde = pkgs.callPackage ./sde.nix { };
  runSdeClxPath = "${nix_pyenv_directory}/bin/run-sde-clx";
  runSdeRplPath = "${nix_pyenv_directory}/bin/run-sde-rpl";
  runSdeIvbPath = "${nix_pyenv_directory}/bin/run-sde-ivb";
  sdeScript = ''
    if [ -z ${pythonpathEnvLiteral} ]; then
        PYTHONPATH=$(pwd)/build @sde64@ @cpuid@ -- "$@"
    else
        @sde64@ @cpuid@ -- "$@"
    fi
  '';
  sde64Path = lib.optionalString (pkgs.system == "x86_64-linux") "${sde}/bin/sde64";
  sdeClxScript = builtins.replaceStrings [ "@cpuid@" "@sde64@" ] [ "-clx" sde64Path ] sdeScript;
  sdeRplScript = builtins.replaceStrings [ "@cpuid@" "@sde64@" ] [ "-rpl" sde64Path ] sdeScript;
  sdeIvbScript = builtins.replaceStrings [ "@cpuid@" "@sde64@" ] [ "-ivb" sde64Path ] sdeScript;
in
''
  _SOURCE_ROOT=$(readlink -f ${builtins.toString ./.}/../..)
  if [[ $_SOURCE_ROOT == /nix/store* ]]; then
      IN_FLAKE=true
      _SOURCE_ROOT=$(readlink -f .)
  fi
  cd $_SOURCE_ROOT

  if [ "$IN_FLAKE" = "true" ] && [ ! -f "flake.nix" ]; then
    _SHOULD_CREATE_PYENV=false
  else
    _SHOULD_CREATE_PYENV=true
  fi

  ensure_symlink() {
      local link_path="$1"
      local target_path="$2"
      if [[ -L "$link_path" ]] && [[ "$(readlink "$link_path")" = "$target_path" ]]; then
          return 0
      fi
      rm -f "$link_path" > /dev/null 2>&1
      ln -s "$target_path" "$link_path"
  }

  if [ "$_SHOULD_CREATE_PYENV" = "false" ]; then
    echo "Not creating pyenv because not in the root directory"
    exit 0
  fi

  # ensure the nix-pyenv directory exists
  mkdir -p ${nix_pyenv_directory}
  mkdir -p ${nix_pyenv_directory}/lib
  mkdir -p ${nix_pyenv_directory}/bin
''
+ (pkgs.lib.strings.concatStrings (builtins.map link_python_cmd versions))
+ ''
  # add python executable to the bin directory
  ensure_symlink "${nix_pyenv_directory}/bin/python" python3.${builtins.toString latestStableVer}
  # export PATH=${using_python}/bin:${nix_pyenv_directory}/bin:$PATH
  export PATH=${nix_pyenv_directory}/bin:$PATH

  # prevent gc
  nix-store --add-root ${nix_pyenv_directory}/.nix-shell-inputs --realise ${inputDerivation}

  # custom
  ensure_symlink "${nix_pyenv_directory}/bin/python_nodebug" ${pyenv_nodebug}/bin/python
  ensure_symlink "${nix_pyenv_directory}/bin/valgrind" ${pkgs.valgrind}/bin/valgrind
  export CC=${pkgs.clang}/bin/clang
  export CXX=${pkgs.clang}/bin/clang++
  export Python3_ROOT_DIR=${using_python}
  ensure_symlink "${nix_pyenv_directory}/bin/clang" $CC
  ensure_symlink "${nix_pyenv_directory}/bin/clang++" $CXX
  ensure_symlink "${nix_pyenv_directory}/bin/cmake" ${pkgs.cmake}/bin/cmake
  ensure_symlink "${nix_pyenv_directory}/bin/clang-format" ${pkgs.clang-tools}/bin/clang-format
  ensure_symlink "${nix_pyenv_directory}/bin/cmake-format" ${pkgs.cmake-format}/bin/cmake-format
  # clang -print-file-name will give wrong asan path, use gcc version
  ensure_symlink "${nix_pyenv_directory}/lib/libasan.so" $(readlink -f $(gcc -print-file-name=libasan.so))

  # unzip orjson source
  mkdir -p ${debugSourceDir}
  if [[ ! -d ${debugSourceDir}/orjson && ${using_python.sourceVersion.minor} != "14" ]]; then
      # this is a directory, not a tarball
      cp -r ${orjsonSource} ${debugSourceDir}/orjson
      chmod -R 700 ${debugSourceDir}/orjson
  fi
''
+ lib.optionalString parentShell.debugLLVM ''
  export PATH=${nix_pyenv_directory}/debugLLVM/bin:$PATH
''
+ ''
  # save env for external use
  echo "PATH=$PATH" > ${nix_pyenv_directory}/.shell-env
  echo "CC=$CC" >> ${nix_pyenv_directory}/.shell-env
  echo "CXX=$CXX" >> ${nix_pyenv_directory}/.shell-env
''
+ lib.optionalString parentShell.debugLLVM ''
  ensure_symlink "${nix_pyenv_directory}/debugLLVM" ${parentShell.__drvs.llvmDbg}
  if [[ ! -d ${debugSourceDir}/llvm-src-${parentShell.__drvs.llvmDbg.version} ]]; then
    mkdir -p ${debugSourceDir}/llvm-src-${parentShell.__drvs.llvmDbg.version}
    cp -r ${parentShell.__drvs.llvmDbg.src}/llvm ${debugSourceDir}/llvm-src-${parentShell.__drvs.llvmDbg.version}/llvm
    chmod -R 700 ${debugSourceDir}/llvm-src-${parentShell.__drvs.llvmDbg.version}
    # mv ${debugSourceDir}/llvm-src-${parentShell.__drvs.llvmDbg.version}/llvm/build/lib ${debugSourceDir}/llvm-src-${parentShell.__drvs.llvmDbg.version}/lib
  fi
''
+ lib.optionalString (pkgs.system == "x86_64-linux") ''
  # sde wrapper script
  cat > ${runSdeClxPath} << 'EOF'
  ${sdeClxScript}
  EOF
  chmod +x ${runSdeClxPath}
  #
  cat > ${runSdeRplPath} << 'EOF'
  ${sdeRplScript}
  EOF
  chmod +x ${runSdeRplPath}
  #
  cat > ${runSdeIvbPath} << 'EOF'
  ${sdeIvbScript}
  EOF
  chmod +x ${runSdeIvbPath}
''
