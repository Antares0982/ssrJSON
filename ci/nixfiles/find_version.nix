{ pkgs, lib, ... }:
path:
let
  splitString = lib.strings.splitString;
  content = builtins.readFile path;
  prefix = "version = \"";
  sep1 = splitString prefix content;
  part1 =
    if (builtins.length sep1 > 1) then (builtins.elemAt sep1 1) else (throw "Invalid pyproject.toml");
  sep2 = splitString "\"" part1;
  version =
    if (builtins.length sep2 > 1) then (builtins.elemAt sep2 0) else (throw "Invalid pyproject.toml");
in
version
