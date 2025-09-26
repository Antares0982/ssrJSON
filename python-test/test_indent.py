# SPDX-License-Identifier: (Apache-2.0 OR MIT)

import pytest
import json

import ssrjson

from util import read_fixture_obj


class TestIndentedOutput:
    def test_equivalent(self):
        """
        OPT_INDENT_2 is equivalent to indent=2
        """
        obj = {"a": "b", "c": {"d": True}, "e": [1, 2]}
        assert ssrjson.dumps(obj, indent=2) == json.dumps(obj, indent=2)
        assert ssrjson.dumps_to_bytes(obj, indent=2) == json.dumps(
            obj, indent=2
        ).encode("utf-8")

    def test_empty(self):
        obj = [{}, [[[]]], {"key": []}]
        ref = '[\n  {},\n  [\n    [\n      []\n    ]\n  ],\n  {\n    "key": []\n  }\n]'
        assert ssrjson.dumps(obj, indent=2) == ref
        assert ssrjson.dumps_to_bytes(obj, indent=2) == ref.encode("utf-8")

    # def test_twitter_pretty(self):
    #     """
    #     twitter.json pretty
    #     """
    #     obj = read_fixture_obj("twitter.json")
    #     assert ssrjson.dumps(obj, indent=2) == json.dumps(
    #         obj, indent=2, ensure_ascii=False
    #     )

    def test_github_pretty(self):
        """
        github.json pretty
        """
        obj = read_fixture_obj("github.json")
        assert ssrjson.dumps(obj, indent=2) == json.dumps(
            obj, indent=2, ensure_ascii=False
        )
        assert ssrjson.dumps_to_bytes(obj, indent=2) == json.dumps(
            obj, indent=2, ensure_ascii=False
        ).encode("utf-8")

    # def test_canada_pretty(self):
    #     """
    #     canada.json pretty
    #     """
    #     obj = read_fixture_obj("canada.json")
    #     assert ssrjson.dumps(obj, indent=2) == json.dumps(
    #         obj, indent=2, ensure_ascii=False
    #     )

    def test_citm_catalog_pretty(self):
        """
        citm_catalog.json pretty
        """
        obj = read_fixture_obj("ctm.json")
        assert ssrjson.dumps(obj, indent=2) == json.dumps(
            obj, indent=2, ensure_ascii=False
        )
        assert ssrjson.dumps_to_bytes(obj, indent=2) == json.dumps(
            obj, indent=2, ensure_ascii=False
        ).encode("utf-8")
        assert ssrjson.dumps(obj, indent=4) == json.dumps(
            obj, indent=4, ensure_ascii=False
        )
        assert ssrjson.dumps_to_bytes(obj, indent=4) == json.dumps(
            obj, indent=4, ensure_ascii=False
        ).encode("utf-8")

    def test_err_indent(self):
        obj = {"a": "b"}
        with pytest.raises(TypeError):
            ssrjson.dumps(obj, indent="2")
        with pytest.raises(TypeError):
            ssrjson.dumps_to_bytes(obj, indent="2")
        with pytest.raises(ValueError):
            ssrjson.dumps(obj, indent=1)
        with pytest.raises(ValueError):
            ssrjson.dumps(obj, indent=3)
        with pytest.raises(ValueError):
            ssrjson.dumps(obj, indent=5)
        with pytest.raises(ValueError):
            ssrjson.dumps_to_bytes(obj, indent=1)
        with pytest.raises(ValueError):
            ssrjson.dumps_to_bytes(obj, indent=3)
        with pytest.raises(ValueError):
            ssrjson.dumps_to_bytes(obj, indent=5)
