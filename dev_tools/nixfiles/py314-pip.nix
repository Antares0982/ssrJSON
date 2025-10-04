# A clean build of pip, without useless dependencies
{
  pypkgs,
  pkgs,
  fetchFromGitHub,
  installShellFiles,
  ...
}:
pypkgs.buildPythonPackage rec {
  pname = "pip";
  version = "25.0.1";
  format = "pyproject";

  src = fetchFromGitHub {
    owner = "pypa";
    repo = pname;
    tag = version;
    hash = "sha256-V069rAL6U5KBnSc09LRCu0M7qQCH5NbMghVttlmIoRY=";
  };

  postPatch = ''
    # Remove vendored Windows PE binaries
    # Note: These are unused but make the package unreproducible.
    find -type f -name '*.exe' -delete
  '';

  nativeBuildInputs = with pypkgs; [
    installShellFiles
    setuptools
    wheel
  ];

  doCheck = false;
}
