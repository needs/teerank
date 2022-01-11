"""
Test rank.py.
"""

from backend.game_server import GameServerState, GameServerType
from backend.rank import rank
from backend.player import Player

def test_rank():
    """
    Test rank with a valid game state.
    """

    Player.set_elo('player1', None, None, 1500)
    Player.set_elo('player2', None, None, 1500)

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

    new = GameServerState(GameServerType.VANILLA)

    new.info = {
        'game_type': 'CTF',
        'map_name': 'ctf1',
        'num_clients': 2
    }

    new.clients = {
        'player1': {
            'score': 1,
            'ingame': True
        },

        'player2': {
            'score': 0,
            'ingame': True
        }
    }

    rank(old, new)

    elo1 = Player.get_elo('player1', None, None)
    elo2 = Player.get_elo('player2', None, None)

    assert elo1 - 1500 == 12.5
    assert elo2 - 1500 == -12.5
