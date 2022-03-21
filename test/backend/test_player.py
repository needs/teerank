"""
Test player.py
"""

from backend.database.player import get_elo, set_elo, upsert, get, get_clan
import backend.database.clan


def test_player_elo(_redis):
    """
    Test ELO load and save.
    """

    combinations = (
        (None, None),
        ('CTF', None),
        ('CTF', 'ctf1'),
        (None, 'ctf1')
    )

    for combination in combinations:
        assert get_elo('player1', combination[0], combination[1]) == 1500
        set_elo('player1', combination[0], combination[1], 900)
        assert get_elo('player1', combination[0], combination[1]) == 900


def test_player_unescaped_name(_graphql):
    """
    Some strange string can confuse Dgraph parser.  Make sure Dgraph accept the
    request.
    """

    player = {
        'name': r'\U000e0021',
        'clan': backend.database.clan.ref(None)
    }

    upsert(player)


def test_player_add(_graphql):
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


def test_player_upsert(_graphql):
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


def test_player_get_clan(_graphql):
    """
    Test get_clan()
    """

    player = {
        'name': 'Test Player',
        'clan': backend.database.clan.ref('Test Clan')
    }

    upsert(player)
    assert get_clan([player['name']]) == {player['name']: player['clan']}
