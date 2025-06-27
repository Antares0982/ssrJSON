{
  clangStdenv,
  python,
  cmake,
  ...
}:
let
  pyenv = python.withPackages (
    pypkgs: with pypkgs; [
      build
      setuptools
      wheel
    ]
  );
in
clangStdenv.mkDerivation {
  pname = "ssrjson-tarball";
  version = builtins.readFile ../version_file;
  src = ./.;
  unpackPhase = ''
    cp -r ${./..}/* .
    chmod -R 700 .
  '';
  buildPhase = ''
    export PATH=${cmake}/bin:$PATH
    cp -r pysrc ssrjson
    python -m build --no-isolation
    mkdir -p $out
    cp dist/*.tar.gz $out
  '';
  buildInputs = [
    pyenv
  ];
}
