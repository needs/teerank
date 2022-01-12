"""
Test player.py
"""

from backend.player import Player


def test_player(_redis):
    """
    Test load and save from the database.
    """

    player = Player('player1')
    assert not player.data

    player.data = {
        'test': 1
    }

    player.save()
    assert player.data == Player(player.key).data


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
        assert Player.get_elo('player1', combination[0], combination[1]) == 1500
        Player.set_elo('player1', combination[0], combination[1], 900)
        assert Player.get_elo('player1', combination[0], combination[1]) == 900
