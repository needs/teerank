"""
Test operations on GameType.
"""

import pytest
from backend.database.map import get


pytestmark = pytest.mark.parametrize(
    "name", [ 'map-test', '', None]
)

def test_map(gametype, name):
    """
    Test Map creation.
    """

    map_ = get(gametype['id'], name)

    assert map_['name'] == name
    assert map_['gameType']['id'] == gametype['id']


def test_map_duplicate(gametype, name):
    """
    Test that getting twice the same map does not create a duplicate.
    """

    assert get(gametype['id'], name) == get(gametype['id'], name)
