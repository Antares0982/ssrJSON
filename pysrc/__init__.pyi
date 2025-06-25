from typing import Any, Dict

__all__ = [
    "JSONDecodeError",
    "JSONEncodeError",
    "dumps",
    "dumps_to_bytes",
    "loads",
    "get_current_features",
    "print_current_features",
]

__version__: str

class JSONDecodeError(ValueError): ...
class JSONEncodeError(ValueError): ...

def dumps(
    obj,
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
def dumps_to_bytes(obj, indent: int | None = None) -> bytes: ...
def loads(
    s: str | bytes,
    cls: Any = None,
    object_hook: Any = None,
    parse_float: Any = None,
    parse_int: Any = None,
    parse_constant: Any = None,
    object_pairs_hook: Any = None,
): ...
def print_current_features() -> None: ...
def get_current_features() -> Dict[str, str]: ...
