{
  parentShell,
  nix_pyenv_directory,
  pyenv,
  pyenvs,
  debuggable_py,
  pyenv_nodebug,
  using_python,
  pkgs,
  pkgs-legacy,
  inputDerivation,
  lib,
}:
let
  versionUtils = pkgs.callPackage ./version_utils.nix { inherit pkgs-legacy; };
  versions = versionUtils.versions;
  debugSourceDir = "debug_source";
  minSupportVer = versionUtils.pythonVerConfig.minSupportVer;
  debug_source_cmd =
    ver:
    let
      debuggable_python = builtins.elemAt debuggable_py (ver - minSupportVer);
      debugSourceTargetDir = "${debugSourceDir}/Python-${debuggable_python.version}";
    in
    ''
      mkdir -p ${debugSourceDir}
      if [[ ! -d ${debugSourceTargetDir} ]]; then
        if [[ -d ${debuggable_python.src} ]]; then
          cp -r ${debuggable_python.src} ${debugSourceTargetDir}
        else
          tar -xf ${debuggable_python.src} -C ${debugSourceDir}
        fi
        chmod -R 755 ${debugSourceTargetDir}
        rm -rf ${debugSourceTargetDir}/Doc
        rm -rf ${debugSourceTargetDir}/Grammar
        rm -rf ${debugSourceTargetDir}/Lib
      fi
    '';

  nixPyEnv = pkgs.callPackage ./dev-env.nix {
    inherit
      pkgs-legacy
      pyenv
      pyenvs
      debuggable_py
      inputDerivation
      pyenv_nodebug
      using_python
      ;
    sitePackagesString = pyenv.sitePackages;
  };
in
''
  _SOURCE_ROOT=$(readlink -f ${builtins.toString ./.}/../..)
  if [[ $_SOURCE_ROOT == /nix/store* ]]; then
      IN_FLAKE=true
      _SOURCE_ROOT=$(readlink -f .)
  fi
  cd $_SOURCE_ROOT

  if [ "$IN_FLAKE" = "true" ] && [ ! -f "flake.nix" ]; then
    echo "Not creating pyenv because not in the root directory"
  else
    ${pkgs.nix}/bin/nix-store --add-root ${nix_pyenv_directory} --realise ${nixPyEnv} &>/dev/null
  fi

  export PATH=${nixPyEnv}/bin:$PATH
  source ${nixPyEnv}/nix-support/shell-env

  mkdir -p ${debugSourceDir}
''
+ (pkgs.lib.strings.concatStrings (builtins.map debug_source_cmd versions))
