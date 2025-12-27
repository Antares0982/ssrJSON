# pylint: skip-file
from typing import Any, Dict

__all__ = [
    # Classes
    "JSONDecodeError",
    "JSONEncodeError",
    # General API
    "dumps",
    "dumps_to_bytes",
    "loads",
    # Utilities
    "__version__",
    "get_current_features",
    "suppress_api_warning",
    "strict_argparse",
    "write_utf8_cache",
]

__default_value: Any = object()

__version__: str

class JSONDecodeError(ValueError): ...
class JSONEncodeError(ValueError): ...

def dumps(
    obj,
    *,
    indent: int | None = None,
    skipkeys: Any = False,  # invalid
    ensure_ascii: Any = True,  # invalid
    check_circular: Any = True,  # invalid
    allow_nan: Any = True,  # invalid
    cls: Any = None,  # invalid
    separators: Any = None,  # invalid
    default: Any = None,  # invalid
    sort_keys: Any = False,  # invalid
) -> str: ...
def dumps_to_bytes(
    obj,
    *,
    indent: int | None = None,
    is_write_cache: bool = __default_value,
) -> bytes: ...
def loads(
    s: str | bytes,
    *,
    cls: Any = None,  # invalid
    object_hook: Any = None,  # invalid
    parse_float: Any = None,  # invalid
    parse_int: Any = None,  # invalid
    parse_constant: Any = None,  # invalid
    object_pairs_hook: Any = None,  # invalid
): ...
def get_current_features() -> Dict[str, str]: ...
def suppress_api_warning() -> None: ...
def strict_argparse(v: bool) -> None: ...
def write_utf8_cache(v: bool) -> None: ...
