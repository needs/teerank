"""
Test rank.py.
"""

import pytest

from backend.game_server import GameServerType
from backend.rank import rank
import shared.player
import shared.game_server

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
        'numPlayers': 2,
        'maxPlayers': 16,
        'numClients': 2,
        'maxClients': 2,

        'clients': [
            {
                'player': shared.player.ref('player1'),
                'clan': shared.clan.ref(None),
                'country': 0,
                'score': 0,
                'ingame': True,
                'gameServer': shared.game_server.ref('0.0.0.0:8300')
            }, {
                'player': shared.player.ref('player2'),
                'clan': shared.clan.ref(None),
                'country': 0,
                'score': 0,
                'ingame': True,
                'gameServer': shared.game_server.ref('0.0.0.0:8300')
            }
        ]
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
        'numPlayers': 2,
        'maxPlayers': 16,
        'numClients': 2,
        'maxClients': 2,

        'clients': [
            {
                'player': shared.player.ref('player1'),
                'clan': shared.clan.ref(None),
                'country': 0,
                'score': 0,
                'ingame': True,
                'gameServer': shared.game_server.ref('0.0.0.0:8300')
            }, {
                'player': shared.player.ref('player2'),
                'clan': shared.clan.ref(None),
                'country': 0,
                'score': 0,
                'ingame': True,
                'gameServer': shared.game_server.ref('0.0.0.0:8300')
            }
        ]
    }


def test_rank_player1_win(old, new):
    """
    Test rank() when player1 wins.
    """

    new['clients'][0]['score'] += 1

    assert rank(old, new)

    assert shared.player.get_elo(new['clients'][0]['player']['name'], None, None) == 1512.5
    assert shared.player.get_elo(new['clients'][1]['player']['name'], None, None) == 1487.5


def test_rank_draw(old, new):
    """
    Test rank() when both players have the same score delta.
    """

    new['clients'][0]['score'] += 1
    new['clients'][1]['score'] += 1

    assert rank(old, new)

    assert shared.player.get_elo(new['clients'][0]['player']['name'], None, None) == 1500
    assert shared.player.get_elo(new['clients'][1]['player']['name'], None, None) == 1500


def test_rank_player2_win(old, new):
    """
    Test rank() when player2 wins.
    """

    new['clients'][1]['score'] += 1

    assert rank(old, new)

    assert shared.player.get_elo(new['clients'][0]['player']['name'], None, None) == 1487.5
    assert shared.player.get_elo(new['clients'][1]['player']['name'], None, None) == 1512.5


def test_rank_no_old(new):
    """
    Make sure rank() fails when there is no old state to compare against.
    """

    assert not rank({}, new)


def test_rank_gametype_not_supported(old, new):
    """
    Make sure rank() fails when gametype is not supported.
    """

    new['gameType'] = old['gameType'] = 'BAD_GAMETYPE'
    assert not rank(old, new)


def test_rank_gametype_changed(old, new):
    """
    Make sure rank() fails when gametypes are different.
    """

    new['gameType'] = old['gameType'] + 'a'
    assert not rank(old, new)


def test_rank_map_changed(old, new):
    """
    Make sure rank() fails when maps are different.
    """

    new['map'] = old['map'] + 'a'
    assert not rank(old, new)


def test_rank_score_regressed(old, new):
    """
    Make sure rank() fails when score regressed.
    """

    old['clients'][0]['score'] = 1
    assert not rank(old, new)


def test_rank_not_enough_players(old, new):
    """
    Make sure rank() fails when there is only one player.
    """

    old['numClients'] = 1
    old['numPlayers'] = 1
    del old['clients'][1]

    new['numClients'] = 1
    new['numPlayers'] = 1
    del new['clients'][1]

    assert not rank(old, new)
