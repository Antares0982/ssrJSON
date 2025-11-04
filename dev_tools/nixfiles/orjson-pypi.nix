# Use builds of orjson from PyPI to replace which in Nixpkgs.
# The PyPI version does not allow debugging.
{
  pypkgs,
  pkgs,
  lib,
  fetchurl,
  version ? "3.11.3",
  ...
}:
let
  system = pkgs.stdenv.hostPlatform.system;
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
      "3.14" = {
        "3.11.3" = {
          urlpart = "3b/94/11137c9b6adb3779f1b34fd98be51608a14b430dbc02c6d41134fbba484c";
          manyLinux = "manylinux_2_34_x86_64";
          hash = "sha256-1hzVQ9aXFdX8CmkMfG+NzDB7wjq++XOJV5gYhfXzgik=";
        };
      };
    };
    "aarch64-linux" = {
      "3.13" = {
        "3.11.3" = {
          urlpart = "a4/b8/2d9eb181a9b6bb71463a78882bcac1027fd29cf62c38a40cc02fc11d3495";
          manyLinux = "manylinux_2_17_aarch64.manylinux2014_aarch64";
          hash = "sha256-Ydza0W2lu0htciejei54nEKTl3k6aVUifO29clLrWic=";
        };
      };
      "3.14" = {
        "3.11.3" = {
          urlpart = "67/46/1e2588700d354aacdf9e12cc2d98131fb8ac6f31ca65997bef3863edb8ff";
          manyLinux = "manylinux_2_34_aarch64";
          hash = "sha256-iNz8UUz9Gw3gOEQ8ez5ql5f/sbNnTvH9FPcBoTOX+C0=";
        };
      };
    };
    "aarch64-darwin" = {
      "3.14" = {
        "3.11.3" = {
          urlpart = "e4/6d/468d21d49bb12f900052edcfbf52c292022d0a323d7828dc6376e6319703";
          manyLinux = "macosx_15_0_arm64";
          hash = "sha256-vIvIW4G2rJ/E2uOTqMFZuBf0wsne5dErdzvds7lfwH4=";
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
