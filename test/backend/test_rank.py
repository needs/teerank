"""
Test rank.py.
"""

import pytest

from backend.game_server import GameServerType
from backend.rank import rank

import backend.database.player_info
import backend.database.player
import backend.database.clan
import backend.database.game_server
import backend.database.map
import backend.database.gametype


def get_score(map_id, player_name: str) -> int:
    """
    Return score for the given player.
    """

    return backend.database.player_info.get(map_id, player_name)['score']


def add_client(game_server, name, score, ingame=True):
    """
    Adds a client to the given game server.
    """

    game_server['clients'].append({
        'player': backend.database.player.ref(name),
        'clan': backend.database.clan.ref(None),
        'country': 0,
        'score': score,
        'ingame': ingame,
        'gameServer': backend.database.game_server.ref(game_server['address'])
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
def fixture_old(map_):
    """
    Create a game server state.
    """
    return {
        'type': GameServerType.VANILLA,
        'address': '0.0.0.0:8300',
        'version': '0.0.1',
        'name': 'old',
        'map': backend.database.map.ref(map_['id']),
        'numPlayers': 0,
        'maxPlayers': 16,
        'numClients': 0,
        'maxClients': 16,

        'clients': []
    }


@pytest.fixture(name='new')
def fixture_new(map_):
    """
    Create a game server state identical to old state.
    """
    return {
        'type': GameServerType.VANILLA,
        'address': '0.0.0.0:8300',
        'version': '0.0.1',
        'name': 'old',
        'map': backend.database.map.ref(map_['id']),
        'numPlayers': 0,
        'maxPlayers': 16,
        'numClients': 0,
        'maxClients': 16,

        'clients': []
    }


@pytest.fixture(name='gametype_not_supported')
def fixture_gametype_not_supported():
    """
    Create a new gametype that is not supported and return it.
    """

    return backend.database.gametype.get('BAD-GAMETYPE')


@pytest.fixture(name='map_not_supported')
def fixture_map_not_supported(gametype_not_supported):
    """
    Create a new map that is not supported and return it.
    """

    return backend.database.map.get(gametype_not_supported['id'], 'ctf1')


def test_rank_player1_win(old, new, map_):
    """
    Test rank() when player1 wins.
    """

    add_client(old, 'player1', 0)
    add_client(old, 'player2', 0)

    add_client(new, 'player1', 1)
    add_client(new, 'player2', 0)

    assert rank(old, new)

    assert get_score(map_['id'], 'player1') == 12.5
    assert get_score(map_['id'], 'player2') == -12.5


def test_rank_draw(old, new, map_):
    """
    Test rank() when both players have the same score delta.
    """

    add_client(old, 'player1', 0)
    add_client(old, 'player2', 0)

    add_client(new, 'player1', 1)
    add_client(new, 'player2', 1)

    assert rank(old, new)

    assert get_score(map_['id'], 'player1') == 0
    assert get_score(map_['id'], 'player2') == 0


def test_rank_player2_win(old, new, map_):
    """
    Test rank() when player2 wins.
    """

    add_client(old, 'player1', 0)
    add_client(old, 'player2', 0)

    add_client(new, 'player1', 0)
    add_client(new, 'player2', 1)

    assert rank(old, new)

    assert get_score(map_['id'], 'player1') == -12.5
    assert get_score(map_['id'], 'player2') == 12.5


def test_rank_no_old(new):
    """
    Make sure rank() fails when there is no old state to compare against.
    """

    assert not rank({}, new)


def test_rank_not_supported(old, new, map_not_supported):
    """
    Make sure rank() fails when gametype is not supported.
    """

    make_rankable(old, new)

    new['map'] = old['map'] = backend.database.map.ref(map_not_supported['id'])

    assert not rank(old, new)


def test_rank_changed(old, new, map_not_supported):
    """
    Make sure rank() fails when map/gametype are different.
    """

    make_rankable(old, new)

    new['map'] = backend.database.map.ref(map_not_supported['id'])

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
