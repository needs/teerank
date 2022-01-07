"""
Implement rank() function.
"""

from itertools import combinations, product
from player import PlayerELO

def _elo_delta(score1: int, elo1: int, score2: int, elo2: int) -> int:
    """
    Classic ELO formula for two players, return the ELO delta for both players.
    """

    k_factor = 25

    if score1 < score2:
        result = 0.0
    elif score1 == score2:
        result = 0.5
    else:
        result = 1.0

    delta = max(400.0, min(-400.0, elo1 - elo2))
    expected = 1.0 / (1.0 + pow(10.0, -delta / 400.0))

    return k_factor * (result - expected)


def rank(old: 'GameServerState', new: 'GameServerState') -> None:
    """
    Rank players given the old and new server state.
    """

    # Both old and new must be valid to be able to compare them.

    if not old.is_complete() or not new.is_complete():
        return

    # If game type or map changed, then it makes no sens to rank players.

    if old.info['game_type'] != new.info['game_type']:
        return
    if old.info['map_name'] != new.info['map_name']:
        return

    if new.info['game_type'] not in ('CTF', 'DM', 'TDM'):
        return

    # Create a list of all players that are ingame in both old and new state.

    old_names = { name for name, client in old.clients.items() if client['ingame'] }
    new_names = { name for name, client in new.clients.items() if client['ingame'] }
    names = list(old_names.intersection(new_names))

    # Player score is the difference between its old score and new score.
    #
    # If the sum of all players scores in old state is bigger than the sum of
    # all players scores in new state, then we don't rank the game because there
    # is a high chance that a new game started.

    scores = [ new.clients[name]['score'] - old.clients[name]['score'] for name in names ]

    if sum(scores) <= 0:
        return

    # Now that all sanity checks have been done, compute players new ELO.
    #
    # New ELO is computed by matching all players against each other.  The
    # winner is the one with the highest score difference.  The average ELO
    # delta of all matches of a player is added to the player original ELO.
    #
    # We do this for each combination of game type and map name.

    for game_type, map_name in product((new.info['game_type'], None), (new.info['map_name'], None)):

        elos = [ PlayerELO.load(name, game_type, map_name) for name in names ]
        deltas = [0] * len(names)

        for i, j in combinations(range(len(names)), 2):
            delta = _elo_delta(scores[i], elos[i], scores[j], elos[j])
            deltas[i] += delta
            deltas[j] -= delta

        for i, name in enumerate(names):
            new_elo = elos[i] + deltas[i] / (len(names) - 1)
            PlayerELO.save(name, game_type, map_name, new_elo)
