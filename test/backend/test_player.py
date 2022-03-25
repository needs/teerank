"""
Test player.py
"""

import backend.database.clan
from backend.database.player import upsert, get, get_clan

def test_player_unescaped_name():
    """
    Some strange string can confuse Dgraph parser.  Make sure Dgraph accept the
    request.
    """

    player = {
        'name': r'\U000e0021',
        'clan': backend.database.clan.ref(None)
    }

    upsert(player)


def test_player_add():
    """
    Test that upsert add a player when the player does not exist.
    """

    player = {
        'name': 'Test Player',
        'clan': backend.database.clan.ref(None)
    }

    assert get(player['name']) == {}
    upsert(player)
    assert get(player['name']) == player


def test_player_upsert():
    """
    Test that updating a player that already exist works.
    """

    player = {
        'name': 'Test Player',
        'clan': backend.database.clan.ref(None)
    }

    upsert(player)

    player = {
        'name': 'Test Player',
        'clan': backend.database.clan.ref('Test Clan')
    }

    upsert(player)

    assert get(player['name']) == player


def test_player_get_clan():
    """
    Test get_clan()
    """

    player = {
        'name': 'Test Player',
        'clan': backend.database.clan.ref('Test Clan')
    }

    upsert(player)
    assert get_clan([player['name']]) == {player['name']: player['clan']}
