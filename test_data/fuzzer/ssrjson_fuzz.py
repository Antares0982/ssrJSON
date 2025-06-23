import json

import ssrjson


def success_test(func, *args, **kwargs):
    try:
        ret = func(*args, **kwargs)
        return True, ret
    except Exception:
        return False, None


def fuzz_bytes_input(input_bytes: bytes):
    should_success, _ = success_test(json.loads, input_bytes)

    success, decoded = success_test(ssrjson.loads, input_bytes)
    # if should_success and not success:
    #     print(f"ssrjson.loads failed on input_bytes: {input_bytes}")
    # if not should_success and success:
    #     print(f"ssrjson.loads should have failed on input_bytes: {input_bytes}")
    if success:
        success_test(ssrjson.dumps, decoded)
        success_test(ssrjson.dumps_to_bytes, decoded)
        # ssrjson.dumps(decoded)
        # ssrjson.dumps_to_bytes(decoded)
    _, input_str = success_test(input_bytes.decode, "utf-8")
    if input_str is not None:
        fuzz_str_input(input_str)


def fuzz_str_input(input_str: str):
    should_success, _ = success_test(json.loads, input_str)
    success, decoded = success_test(ssrjson.loads, input_str)
    # if should_success and not success:
    #     print(f"ssrjson.loads failed on input_str: {input_str}")
    # if not should_success and success:
    #     print(f"ssrjson.loads should have failed on input_str: {input_str}")
    if success:
        success_test(ssrjson.dumps, decoded)
        success_test(ssrjson.dumps_to_bytes, decoded)
