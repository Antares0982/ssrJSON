rm -rf ssrjson
mkdir ssrjson
nix build .#ssrjson-py39
cp result/ssrjson.cpython-39-x86_64-linux-gnu.so ssrjson/
nix build .#ssrjson-py310
cp result/ssrjson.cpython-310-x86_64-linux-gnu.so ssrjson/
nix build .#ssrjson-py311
cp result/ssrjson.cpython-311-x86_64-linux-gnu.so ssrjson/
nix build .#ssrjson-py312
cp result/ssrjson.cpython-312-x86_64-linux-gnu.so ssrjson/
nix build .#ssrjson-py313
cp result/ssrjson.cpython-313-x86_64-linux-gnu.so ssrjson/
nix build .#ssrjson-py314
cp result/ssrjson.cpython-314-x86_64-linux-gnu.so ssrjson/
sha256sum ssrjson/ssrjson.cpython-39-x86_64-linux-gnu.so | awk '{print $1}'
sha256sum ssrjson/ssrjson.cpython-310-x86_64-linux-gnu.so | awk '{print $1}'
sha256sum ssrjson/ssrjson.cpython-311-x86_64-linux-gnu.so | awk '{print $1}'
sha256sum ssrjson/ssrjson.cpython-312-x86_64-linux-gnu.so | awk '{print $1}'
sha256sum ssrjson/ssrjson.cpython-313-x86_64-linux-gnu.so | awk '{print $1}'
sha256sum ssrjson/ssrjson.cpython-314-x86_64-linux-gnu.so | awk '{print $1}'
