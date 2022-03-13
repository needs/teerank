"""
Test player.py
"""

import shared.player


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
