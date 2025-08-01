# SPDX-License-Identifier: (Apache-2.0 OR MIT)

import pytest

import ssrjson


class TestCircular:
    def test_circular_dict(self):
        """
        dumps() circular reference dict
        """
        obj = {}  # type: ignore
        obj["obj"] = obj
        with pytest.raises(ssrjson.JSONEncodeError):
            ssrjson.dumps(obj)
        with pytest.raises(ssrjson.JSONEncodeError):
            ssrjson.dumps_to_bytes(obj)

    # def test_circular_dict_sort_keys(self):
    #     """
    #     dumps() circular reference dict OPT_SORT_KEYS
    #     """
    #     obj = {}  # type: ignore
    #     obj["obj"] = obj
    #     with pytest.raises(ssrjson.JSONEncodeError):
    #         ssrjson.dumps(obj, option=ssrjson.OPT_SORT_KEYS)

    # def test_circular_dict_non_str_keys(self):
    #     """
    #     dumps() circular reference dict OPT_NON_STR_KEYS
    #     """
    #     obj = {}  # type: ignore
    #     obj["obj"] = obj
    #     with pytest.raises(ssrjson.JSONEncodeError):
    #         ssrjson.dumps(obj, option=ssrjson.OPT_NON_STR_KEYS)

    def test_circular_list(self):
        """
        dumps() circular reference list
        """
        obj = []  # type: ignore
        obj.append(obj)  # type: ignore
        with pytest.raises(ssrjson.JSONEncodeError):
            ssrjson.dumps(obj)
        with pytest.raises(ssrjson.JSONEncodeError):
            ssrjson.dumps_to_bytes(obj)

    def test_circular_nested(self):
        """
        dumps() circular reference nested dict, list
        """
        obj = {}  # type: ignore
        obj["list"] = [{"obj": obj}]
        with pytest.raises(ssrjson.JSONEncodeError):
            ssrjson.dumps(obj)
        with pytest.raises(ssrjson.JSONEncodeError):
            ssrjson.dumps_to_bytes(obj)

    # def test_circular_nested_sort_keys(self):
    #     """
    #     dumps() circular reference nested dict, list OPT_SORT_KEYS
    #     """
    #     obj = {}  # type: ignore
    #     obj["list"] = [{"obj": obj}]
    #     with pytest.raises(ssrjson.JSONEncodeError):
    #         ssrjson.dumps(obj, option=ssrjson.OPT_SORT_KEYS)

    # def test_circular_nested_non_str_keys(self):
    #     """
    #     dumps() circular reference nested dict, list OPT_NON_STR_KEYS
    #     """
    #     obj = {}  # type: ignore
    #     obj["list"] = [{"obj": obj}]
    #     with pytest.raises(ssrjson.JSONEncodeError):
    #         ssrjson.dumps(obj, option=ssrjson.OPT_NON_STR_KEYS)
