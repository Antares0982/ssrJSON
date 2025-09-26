# SPDX-License-Identifier: (Apache-2.0 OR MIT)

import pytest

import ssrjson

from util import read_fixture_bytes, read_fixture_str


class TestFixture:
    def test_apache(self):
        """
        loads(), dumps() apache.json
        """
        val = read_fixture_str("apache.json")
        read = ssrjson.loads(val)
        assert ssrjson.loads(ssrjson.dumps(read)) == read
        assert ssrjson.loads(ssrjson.dumps_to_bytes(read)) == read

    def test_canada(self):
        """
        loads(), dumps() canada.json
        """
        val = read_fixture_str("canada.json")
        read = ssrjson.loads(val)
        assert ssrjson.loads(ssrjson.dumps(read)) == read
        assert ssrjson.loads(ssrjson.dumps_to_bytes(read)) == read

    def test_citm_catalog(self):
        """
        loads(), dumps() ctm.json
        """
        val = read_fixture_str("ctm.json")
        read = ssrjson.loads(val)
        assert ssrjson.loads(ssrjson.dumps(read)) == read
        assert ssrjson.loads(ssrjson.dumps_to_bytes(read)) == read

    def test_github(self):
        """
        loads(), dumps() github.json
        """
        val = read_fixture_str("github.json")
        read = ssrjson.loads(val)
        assert ssrjson.loads(ssrjson.dumps(read)) == read
        assert ssrjson.loads(ssrjson.dumps_to_bytes(read)) == read

    def test_instruments(self):
        """
        loads(), dumps() instruments.json
        """
        val = read_fixture_str("instruments.json")
        read = ssrjson.loads(val)
        assert ssrjson.loads(ssrjson.dumps(read)) == read
        assert ssrjson.loads(ssrjson.dumps_to_bytes(read)) == read

    def test_mesh(self):
        """
        loads(), dumps() mesh.json
        """
        val = read_fixture_str("mesh.json")
        read = ssrjson.loads(val)
        assert ssrjson.loads(ssrjson.dumps(read)) == read
        assert ssrjson.loads(ssrjson.dumps_to_bytes(read)) == read

    def test_mqaq2016(self):
        """
        loads(), dumps() MotionsQuestionsAnswersQuestions2016.json
        """
        val = read_fixture_str("MotionsQuestionsAnswersQuestions2016.json")
        read = ssrjson.loads(val)
        assert ssrjson.loads(ssrjson.dumps(read)) == read
        assert ssrjson.loads(ssrjson.dumps_to_bytes(read)) == read

    def test_truenull(self):
        """
        loads(), dumps() truenull.json
        """
        val = read_fixture_str("truenull.json")
        read = ssrjson.loads(val)
        assert read == ssrjson.loads(val.encode("utf-8"))
        assert ssrjson.loads(ssrjson.dumps(read)) == read
        assert ssrjson.loads(ssrjson.dumps_to_bytes(read)) == read

    def test_tweet(self):
        """
        loads(), dumps() tweet.json
        """
        val = read_fixture_str("tweet.json")
        read = ssrjson.loads(val)
        assert ssrjson.loads(ssrjson.dumps(read)) == read
        assert ssrjson.loads(ssrjson.dumps_to_bytes(read)) == read

    def test_twitter(self):
        """
        loads(),dumps() twitter.json
        """
        val = read_fixture_str("twitter.json")
        read = ssrjson.loads(val)
        assert ssrjson.loads(ssrjson.dumps(read)) == read
        assert ssrjson.dumps(read).encode("utf-8") == ssrjson.dumps_to_bytes(read)

    def test_blns(self):
        """
        loads() blns.json JSONDecodeError

        https://github.com/minimaxir/big-list-of-naughty-strings
        """
        val = read_fixture_bytes("blns.txt")
        for line in val.split(b"\n"):
            if line and not line.startswith(b"#"):
                with pytest.raises(ssrjson.JSONDecodeError):
                    _ = ssrjson.loads(b'"' + val + b'"')
