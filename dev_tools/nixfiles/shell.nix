{
  pkgs ? import <nixpkgs> { },
  pkgs-legacy,
  ...
}:
let
  nix_pyenv_directory = ".nix-devenv";
  # define version
  versionUtils = pkgs.callPackage ./version_utils.nix { inherit pkgs-legacy; };
  pythonVerConfig = versionUtils.pythonVerConfig;
  curVer = pythonVerConfig.curVer;
  leastVer = pythonVerConfig.minSupportVer;
  drvs = pkgs.callPackage ./_drvs.nix { inherit pkgs-legacy; };
  using_pythons = drvs.using_pythons;
  using_python = builtins.elemAt using_pythons (curVer - leastVer);
  pyenvs = drvs.pyenvs;
  pyenv = builtins.elemAt pyenvs (curVer - leastVer);
in
(pkgs.mkShell {
  packages = pkgs.callPackage ./packages.nix { inherit pkgs-legacy; };
  hardeningDisable = [ "fortify" ];
})
// {
  __drvs = drvs;
}
