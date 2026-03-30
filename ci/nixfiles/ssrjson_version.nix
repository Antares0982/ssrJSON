{ pkgs, ... }:
let
  findVersion = pkgs.callPackage ./find_version.nix { };
  version = findVersion ./../../pyproject.toml;
in
version
