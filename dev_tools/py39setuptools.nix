{ super, pkgs }:
super.setuptools.overridePythonAttrs (superAttr: rec {
  version = "75.1.1";
  src = pkgs.fetchFromGitHub {
    owner = "pypa";
    repo = "setuptools";
    rev = "refs/tags/v${version}";
    hash = "sha256-b8O/DrDWAbD6ht9M762fFN6kPtV8hAbn1gAN9SS7H5g=";
  };
})
