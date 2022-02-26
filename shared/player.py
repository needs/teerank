"""
Implement Player and PlayerELO classes.
"""

import json
from gql import gql
from shared.database import redis, graphql


class Player:
    """
    Teerank player.
    """

    def __init__(self, key: str):
        """
        Load player with the given key.
        """
        self.key = key

        data = redis.get(f'players:{key}')

        if data:
            self.data = json.loads(data)
        else:
            self.data = None


    def save(self) -> None:
        """
        Save player data.
        """
        redis.set(f'players:{self.key}', json.dumps(self.data))


    @staticmethod
    def get_elo(player_key: str, game_type: str, map_name: str) -> int:
        """
        Load player ELO for the given game type.
        """

        elo = redis.zscore(_key(game_type, map_name), player_key)
        return elo if elo else 1500


    @staticmethod
    def set_elo(player_key: str, game_type: str, map_name: str, elo: int) -> None:
        """
        Save player ELO for the given game type.
        """

        redis.zadd(_key(game_type, map_name), {player_key: elo})


def _key(game_type: str, map_name: str) -> str:
    """
    Return redis sorted set key for the given game type and map.
    """

    if game_type is None and map_name is None:
        return 'rank'
    if game_type is not None and map_name is None:
        return f'rank-gametype:{game_type}'
    if game_type is None and map_name is not None:
        return f'rank-map:{map_name}'

    return f'rank-gametype-map:{game_type}:{map_name}'


def players_by_rank(game_type: str, map_name: str, rank: int, count: int) -> list[str]:
    """
    Return the number of players requested from the starting rank for the given
    game type and map.
    """

    return redis.zrange(_key(game_type, map_name), rank, rank + count - 1, withscores=True)


def players_count(game_type: str, map_name: str) -> int:
    """
    Return the number of players for the given game type and map.
    """

    return redis.zcard(_key(game_type, map_name))


def ref(name):
    """Create a player reference from the given player name."""
    return { 'name': name } if name else None


_GQL_UPDATE_PLAYERS = gql(
    """
    mutation ($players: [AddPlayerInput!]!) {
        addPlayer(input: $players, upsert: true) {
            player {
                name
            }
        }
    }
    """
)

def upsert(players):
    """
    Add or update the given players.
    """

    graphql.execute(
        _GQL_UPDATE_PLAYERS,
        variable_values = { 'players': players }
    )


_GQL_QUERY_PLAYERS_CLAN = gql(
    """
    query ($players: [String]!) {
        queryPlayer(filter: {name: {in: $players}}) {
            name
            clan {
                name
            }
        }
    }
    """
)

def get_clan(players):
    """
    Get clan for each of the given players.
    """

    players = graphql.execute(
        _GQL_QUERY_PLAYERS_CLAN,
        variable_values = { 'players': players }
    )['queryPlayer']

    return { player['name']: player['clan'] for player in players }
