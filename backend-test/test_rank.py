"""
Test rank.py.
"""

import pytest
import pytest_redis

from backend.game_server import GameServerState, GameServerType
from backend.rank import rank
from backend.player import Player

redis_instance = pytest_redis.factories.redis_noproc(
    host='database-test'
)

_redis = pytest_redis.factories.redisdb(
    'redis_instance'
)


@pytest.fixture(name='old')
def fixture_old(_redis):
    """
    Create a game server state.
    """
    old = GameServerState(GameServerType.VANILLA)

    old.info = {
        'game_type': 'CTF',
        'map_name': 'ctf1',
        'num_clients': 2
    }

    old.clients = {
        'player1': {
            'score': 0,
            'ingame': True
        },

        'player2': {
            'score': 0,
            'ingame': True
        }
    }

    return old


@pytest.fixture(name='new')
def fixture_new(_redis):
    """
    Create a game server state identical to old state.
    """
    new = GameServerState(GameServerType.VANILLA)

    new.info = {
        'game_type': 'CTF',
        'map_name': 'ctf1',
        'num_clients': 2
    }

    new.clients = {
        'player1': {
            'score': 0,
            'ingame': True
        },

        'player2': {
            'score': 0,
            'ingame': True
        }
    }

    return new


def test_rank(old, new):
    """
    Test rank with a valid game state.
    """

    Player.set_elo('player1', None, None, 1500)
    Player.set_elo('player2', None, None, 1500)

    new.clients['player1']['score'] += 1

    assert rank(old, new)

    elo1 = Player.get_elo('player1', None, None)
    elo2 = Player.get_elo('player2', None, None)

    assert elo1 - 1500 == 12.5
    assert elo2 - 1500 == -12.5


def test_rank_gametype_not_supported(old, new):
    """
    Make sure rank() fails when gametype is not supported.
    """

    new.info['game_type'] = old.info['game_type'] = 'BAD_GAMETYPE'
    assert not rank(old, new)


def test_rank_gametype_changed(old, new):
    """
    Make sure rank() fails when gametype changed.
    """

    new.info['game_type'] = 'DM'
    assert not rank(old, new)


def test_rank_map_changed(old, new):
    """
    Make sure rank() fails when map changed.
    """

    new.info['map_name'] = 'DM'
    assert not rank(old, new)


def test_rank_score_regressed(old, new):
    """
    Make sure rank() fails when score regressed.
    """

    old.clients['player1']['score'] = 1
    assert not rank(old, new)


def test_rank_not_enough_players(old, new):
    """
    Make sure rank() fails when there is only one player.
    """

    old.info['num_clients'] = 1
    del old.clients['player2']
    new.info['num_clients'] = 1
    del new.clients['player2']

    assert not rank(old, new)
