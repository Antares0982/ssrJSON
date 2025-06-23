{
  pkgs ? import <nixpkgs> { },
  pkgs-24-05,
  debugLLVM,
  ...
}:
let
  nix_pyenv_directory = ".nix-pyenv";
  # define version
  pythonVerConfig = pkgs.lib.importJSON ./pyver.json;
  curVer = pythonVerConfig.curVer;
  leastVer = pythonVerConfig.minSupportVer;
  drvs = pkgs.callPackage ./_drvs.nix { inherit pkgs-24-05; };
  using_pythons = drvs.using_pythons;
  using_python = builtins.elemAt using_pythons (curVer - leastVer);
  pyenvs = drvs.pyenvs;
  pyenv = builtins.elemAt pyenvs (curVer - leastVer);
in
(pkgs.mkShell {
  buildInputs = pkgs.lib.optionals debugLLVM [ drvs.llvmDbg ];
  packages = pkgs.callPackage ./packages.nix { inherit pkgs-24-05; };
  hardeningDisable = [ "fortify" ];
})
// {
  __drvs = drvs;
  inherit debugLLVM;
}
