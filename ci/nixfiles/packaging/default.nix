{
  callPackage,
  lib,
  uv2nix,
  pyproject-nix,
  pyproject-build-systems,
}:
let
  workspace = uv2nix.lib.workspace.loadWorkspace { workspaceRoot = ./.; };
  overlay = workspace.mkPyprojectOverlay { sourcePreference = "wheel"; };
in
{
  python,
  repair ? false,
}:
let
  pythonSet = (callPackage pyproject-nix.build.packages { inherit python; }).overrideScope (
    lib.composeExtensions pyproject-build-systems.overlays.default overlay
  );
in
pythonSet.mkVirtualEnv "ssrjson-packaging-env" {
  ssrjson-packaging = lib.optional repair "repair";
}
