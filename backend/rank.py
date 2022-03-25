"""
Implement rank() function.
"""

import logging
from itertools import combinations
from dataclasses import dataclass, field
from collections import defaultdict

import backend.database.player_info


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

    delta = min(400.0, max(-400.0, elo1 - elo2))
    expected = 1.0 / (1.0 + pow(10.0, -delta / 400.0))

    return k_factor * (result - expected)


@dataclass
class _Player:
    name: str = field(init=False)
    old: dict
    new: dict
    score: int = field(init=False)
    delta: float = field(init=False)
    info: dict = field(init=False)

    def __post_init__(self):
        self.name = self.old['player']['name']
        self.score = self.new['score'] - self.old['score']


def rank(old: dict, new: dict) -> bool:
    """
    Rank players given the old and new server state.
    """

    # Both old and new must be valid to be able to compare them.

    if not old:
        return False

    # If game type is not rankable, or game type changed, or map changed, then
    # it makes no sens to rank players.

    if new['gameType'] not in ('CTF', 'DM', 'TDM') or \
        old['gameType'] != new['gameType'] or \
        old['map'] != new['map']:
        return False

    # Create a list of all players that are ingame in both old and new state.

    clients = defaultdict(dict)

    for client in old['clients']:
        clients[client['player']['name']]['old'] = client
    for client in new['clients']:
        clients[client['player']['name']]['new'] = client

    players = []

    for client in clients.values():
        if (
            'old' in client and
            client['old']['ingame'] and
            'new' in client and
            client['new']['ingame']
        ):
            players.append(_Player(client['old'], client['new']))

    if len(players) <= 1:
        return False

    # Player score is the difference between its old score and new score.
    #
    # If the sum of all players scores in old state is bigger than the sum of
    # all players scores in new state, then we don't rank the game because there
    # is a high chance that a new game started.

    if sum([player.score for player in players]) <= 0:
        return False

    # Now that all sanity checks have been done, compute players new ELO.
    #
    # New ELO is computed by matching all players against each other.  The
    # winner is the one with the highest score difference.  The average ELO
    # delta of all matches of a player is added to the player original ELO.
    #
    # We do this for each combination of game type and map name.

    gametype_id = backend.database.gametype.get(new['gameType'])['id']

    for map_name in (new['map'], None):

        map_id = backend.database.map.get(gametype_id, map_name)['id']

        for player in players:
            player.info = backend.database.player_info.get(map_id, player.name)
            player.delta = 0

        for player1, player2 in combinations(players, 2):
            delta = _elo_delta(
                player1.score,
                player1.info['score'],
                player2.score,
                player2.info['score']
            )
            player1.delta += delta
            player2.delta -= delta

        for player in players:
            player.info['score'] += player.delta / (len(players) - 1)
            backend.database.player_info.update(player.info)

            logging.info(
                'Gametype: %s: map %s: player %s: new_score: %d',
                new['gameType'], new['map'], player.name, player.info['score']
            )

    return True
