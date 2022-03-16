"""
Test rank.py.
"""

import pytest

from backend.game_server import GameServerType
from backend.rank import rank
from shared.player import get_elo

import shared.player
import shared.game_server

def add_client(game_server, name, score, ingame=True):
    """
    Adds a client to the given game server.
    """

    game_server['clients'].append({
        'player': shared.player.ref(name),
        'clan': shared.clan.ref(None),
        'country': 0,
        'score': score,
        'ingame': ingame,
        'gameServer': shared.game_server.ref(game_server['address'])
    })

    game_server['numClients'] += 1
    if ingame:
        game_server['numPlayers'] += 1


def make_rankable(old, new):
    """
    Change old and new such that rank(old, new) return true.
    """

    add_client(old, 'player1', 0)
    add_client(old, 'player2', 0)

    add_client(new, 'player1', 1)
    add_client(new, 'player2', 0)


@pytest.fixture(name='old')
def fixture_old(_redis):
    """
    Create a game server state.
    """
    return {
        'type': GameServerType.VANILLA,
        'address': '0.0.0.0:8300',
        'version': '0.0.1',
        'name': 'old',
        'map': 'ctf1',
        'gameType': 'CTF',
        'numPlayers': 0,
        'maxPlayers': 16,
        'numClients': 0,
        'maxClients': 16,

        'clients': []
    }


@pytest.fixture(name='new')
def fixture_new(_redis):
    """
    Create a game server state identical to old state.
    """
    return {
        'type': GameServerType.VANILLA,
        'address': '0.0.0.0:8300',
        'version': '0.0.1',
        'name': 'old',
        'map': 'ctf1',
        'gameType': 'CTF',
        'numPlayers': 0,
        'maxPlayers': 16,
        'numClients': 0,
        'maxClients': 16,

        'clients': []
    }


def test_rank_player1_win(old, new):
    """
    Test rank() when player1 wins.
    """

    add_client(old, 'player1', 0)
    add_client(old, 'player2', 0)

    add_client(new, 'player1', 1)
    add_client(new, 'player2', 0)

    assert rank(old, new)

    assert get_elo('player1', None, None) == 1512.5
    assert get_elo('player2', None, None) == 1487.5


def test_rank_draw(old, new):
    """
    Test rank() when both players have the same score delta.
    """

    add_client(old, 'player1', 0)
    add_client(old, 'player2', 0)

    add_client(new, 'player1', 1)
    add_client(new, 'player2', 1)

    assert rank(old, new)

    assert get_elo('player1', None, None) == 1500
    assert get_elo('player2', None, None) == 1500


def test_rank_player2_win(old, new):
    """
    Test rank() when player2 wins.
    """

    add_client(old, 'player1', 0)
    add_client(old, 'player2', 0)

    add_client(new, 'player1', 0)
    add_client(new, 'player2', 1)

    assert rank(old, new)

    assert get_elo('player1', None, None) == 1487.5
    assert get_elo('player2', None, None) == 1512.5


def test_rank_no_old(new):
    """
    Make sure rank() fails when there is no old state to compare against.
    """

    assert not rank({}, new)


def test_rank_gametype_not_supported(old, new):
    """
    Make sure rank() fails when gametype is not supported.
    """

    make_rankable(old, new)

    new['gameType'] = old['gameType'] = 'BAD_GAMETYPE'
    assert not rank(old, new)


def test_rank_gametype_changed(old, new):
    """
    Make sure rank() fails when gametypes are different.
    """

    make_rankable(old, new)

    new['gameType'] = old['gameType'] + 'a'
    assert not rank(old, new)


def test_rank_map_changed(old, new):
    """
    Make sure rank() fails when maps are different.
    """

    make_rankable(old, new)

    new['map'] = old['map'] + 'a'
    assert not rank(old, new)


def test_rank_score_regressed(old, new):
    """
    Make sure rank() fails when score regressed.
    """

    add_client(old, 'player1', 1)
    add_client(old, 'player2', 1)

    add_client(new, 'player1', 0)
    add_client(new, 'player2', 0)

    assert not rank(old, new)


def test_rank_not_enough_players(old, new):
    """
    Make sure rank() fails when there is only one player.
    """

    add_client(old, 'player1', 0)
    add_client(new, 'player1', 1)

    assert not rank(old, new)


def test_rank_different_players(old, new):
    """
    Make sure rank() fails when players are different in old and new.
    """

    add_client(old, 'player1-old', 0)
    add_client(old, 'player2-old', 0)

    add_client(new, 'player1-new', 1)
    add_client(new, 'player2-new', 1)

    assert not rank(old, new)
