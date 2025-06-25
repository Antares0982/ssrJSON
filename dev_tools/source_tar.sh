set -e
rm -rf dist wheelhouse ssrjson ssrjson.egg-info
cp -r pysrc ssrjson
nix develop .#packShell313 -c python -m build
