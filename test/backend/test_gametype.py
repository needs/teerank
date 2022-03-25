"""
Test operations on GameType.
"""

import pytest
from backend.database.gametype import get

pytestmark = pytest.mark.parametrize(
    "name", [ 'gametype-test', '', None]
)

def test_gametype(name):
    """
    Test GameType creation.
    """

    assert get(name)['name'] == name


def test_gametype_duplicate(name):
    """
    Test creation of two duplicate gametype.
    """

    assert get(name) == get(name)
