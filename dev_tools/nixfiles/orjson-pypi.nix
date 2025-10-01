# Use builds of orjson from PyPI to replace which in Nixpkgs
{
  pypkgs,
  pkgs,
  lib,
  fetchurl,
  version ? "3.11.3",
  system,
  ...
}:
let
  pythonVersionString = pypkgs.python.sourceVersion.major + "." + pypkgs.python.sourceVersion.minor;
  pythonAbiString = "cp" + pypkgs.python.sourceVersion.major + pypkgs.python.sourceVersion.minor;
  sourceUrl = {
    "x86_64-linux" = {
      "3.13" = {
        "3.11.3" = {
          urlpart = "d0/b4/f98355eff0bd1a38454209bbc73372ce351ba29933cb3e2eba16c04b9448";
          manyLinux = "manylinux_2_17_x86_64.manylinux2014_x86_64";
          hash = "sha256-uCLK9bl1K8byRusIEkw9Er8hdbZqt0usLvO7+SIc4bI=";
        };
      };
    };
    "aarch64-linux" = {
      "3.13" = {
        "3.11.3" = {
          urlpart = "a4/b8/2d9eb181a9b6bb71463a78882bcac1027fd29cf62c38a40cc02fc11d3495";
          manyLinux = "manylinux_2_17_aarch64.manylinux2014_aarch64";
          hash = "";
        };
      };
    };
  };
  orjsonPypiSource =
    let
      srcConfig = sourceUrl.${system}.${pythonVersionString}.${version};
    in
    fetchurl {
      # get url here: https://pypi.org/project/orjson/#files
      url =
        "https://files.pythonhosted.org/packages/"
        + srcConfig.urlpart
        + "/orjson-"
        + version
        + "-"
        + pythonAbiString
        + "-"
        + pythonAbiString
        + "-"
        + srcConfig.manyLinux
        + ".whl";
      inherit (srcConfig) hash;
    };
in

pypkgs.buildPythonPackage rec {
  pname = "orjson";
  inherit version;
  format = "wheel";

  src = orjsonPypiSource;
}
