{
  pkgs,
  mkShell,
  py,
  ...
}:
let
  pyenv = py.withPackages (
    pypkgs: with pypkgs; [
      auditwheel
      packaging
      build
      twine
    ]
  );
in
mkShell {
  # buildInputs = pkgs.lib.optionals debugLLVM [ drvs.llvmDbg ];
  packages = with pkgs; [
    pyenv
  ];
}
