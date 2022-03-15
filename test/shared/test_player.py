"""
Test player.py
"""

import shared.player
import shared.clan


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
        assert shared.player.get_elo('player1', combination[0], combination[1]) == 1500
        shared.player.set_elo('player1', combination[0], combination[1], 900)
        assert shared.player.get_elo('player1', combination[0], combination[1]) == 900


def test_player_add(_graphql):
    """
    Test that upsert add a player when the player does not exist.
    """

    player = {
        'name': 'Test Player',
        'clan': shared.clan.ref(None)
    }

    assert shared.player.get(player['name']) == {}
    shared.player.upsert(player)
    assert shared.player.get(player['name']) == player


def test_player_upsert(_graphql):
    """
    Test that updating a player that already exist works.
    """

    player = {
        'name': 'Test Player',
        'clan': shared.clan.ref(None)
    }

    shared.player.upsert(player)

    player = {
        'name': 'Test Player',
        'clan': shared.clan.ref('Test Clan')
    }

    shared.player.upsert(player)

    assert shared.player.get(player['name']) == player


def test_player_get_clan(_graphql):
    """
    Test shared.player.get_clan()
    """

    player = {
        'name': 'Test Player',
        'clan': shared.clan.ref('Test Clan')
    }

    shared.player.upsert(player)
    assert shared.player.get_clan([player['name']]) == {player['name']: player['clan']}
