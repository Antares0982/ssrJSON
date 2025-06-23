{
  super,
  lib,
  aspectlib ? super.aspectlib,
  buildPythonPackage ? super.buildPythonPackage,
  elasticsearch,
  fetchFromGitHub,
  git,
  py-cpuinfo ? super.py-cpuinfo,
  pygal ? super.pygal,
  pytest ? super.pytest,
  pytest-xdist ? super.pytest-xdist,
  setuptools ? super.setuptools,
}:
with super;
buildPythonPackage rec {
  pname = "pytest-benchmark";
  version = super.pytest-benchmark.version;
  pyproject = true;

  src = fetchFromGitHub {
    owner = "ionelmc";
    repo = "pytest-benchmark";
    tag = "v${version}";
    hash = "sha256-4fD9UfZ6jtY7Gx/PVzd1JNWeQNz+DJ2kQmCku2TgxzI=";
  };

  build-system = [ setuptools ];

  buildInputs = [ pytest ];

  dependencies = [ py-cpuinfo ];

  optional-dependencies = {
    aspect = [ aspectlib ];
    histogram = [
      pygal
      # FIXME package pygaljs
      setuptools
    ];
    elasticsearch = [ elasticsearch ];
  };

  pythonImportsCheck = [ "pytest_benchmark" ];

  __darwinAllowLocalNetworking = true;

  doCheck = false;

  # nativeCheckInputs = [
  #   freezegun
  #   git
  #   mercurial
  #   nbmake
  #   pytestCheckHook
  #   pytest-xdist
  # ] ++ lib.flatten (lib.attrValues optional-dependencies);

  # preCheck = ''
  #   export PATH="$out/bin:$PATH"
  #   export HOME=$(mktemp -d)
  # '';

  # disabledTests =
  #   lib.optionals (pythonOlder "3.12") [
  #     # AttributeError: 'PluginImportFixer' object has no attribute 'find_spec'
  #     "test_compare_1"
  #     "test_compare_2"
  #     "test_regression_checks"
  #     "test_regression_checks_inf"
  #     "test_rendering"
  #   ]
  #   ++ lib.optionals (pythonAtLeast "3.13") [
  #     # argparse usage changes mismatches test artifact
  #     "test_help"
  #   ];

  # meta = {
  #   changelog = "https://github.com/ionelmc/pytest-benchmark/blob/${src.rev}/CHANGELOG.rst";
  #   description = "Pytest fixture for benchmarking code";
  #   homepage = "https://github.com/ionelmc/pytest-benchmark";
  #   license = lib.licenses.bsd2;
  #   maintainers = with lib.maintainers; [ dotlambda ];
  # };
}
