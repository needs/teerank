"""
Test player_info.py
"""

import pytest
from backend.database import player_info


@pytest.fixture(name='player_name')
def fixture_player_name():
    """
    Return player name.
    """

    return 'test-player'


def test_player_info_get(map_, player_name):
    """
    Test get().
    """

    info = player_info.get(map_['id'], player_name)

    assert info['player']['name'] == player_name
    assert info['score'] == 0
    assert info['map']['id'] == map_['id']


def test_player_info_update(map_, player_name):
    """
    Test update().
    """

    info = player_info.get(map_['id'], player_name)

    info['score'] += 1
    player_info.update(info)

    updated_info = player_info.get(map_['id'], player_name)

    assert updated_info == info


def test_player_info_consistent_get(map_, player_name):
    """
    Test that two call to get for the same player info return the same data.
    """

    info1 = player_info.get(map_['id'], player_name)
    info2 = player_info.get(map_['id'], player_name)

    assert info1 == info2
