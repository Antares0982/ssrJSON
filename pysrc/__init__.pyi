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

__version__: str

class JSONDecodeError(ValueError): ...
class JSONEncodeError(ValueError): ...

def dumps(
    obj,
    *,
    indent: int | None = None,
    skipkeys: Any = False,
    ensure_ascii: Any = True,
    check_circular: Any = True,
    allow_nan: Any = True,
    cls: Any = None,
    separators: Any = None,
    default: Any = None,
    sort_keys: Any = False,
) -> str: ...
def dumps_to_bytes(
    obj,
    *,
    indent: int | None = None,
    is_write_cache: bool | None = None,
) -> bytes: ...
def loads(
    s: str | bytes,
    *,
    cls: Any = None,
    object_hook: Any = None,
    parse_float: Any = None,
    parse_int: Any = None,
    parse_constant: Any = None,
    object_pairs_hook: Any = None,
): ...
def get_current_features() -> Dict[str, str]: ...
def suppress_api_warning() -> None: ...
def strict_argparse(v: bool) -> None: ...
def write_utf8_cache(v: bool) -> None: ...
