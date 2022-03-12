"""
Implement Player and PlayerELO classes.
"""

import json
from gql import gql
from shared.database import redis, graphql


def _key(game_type: str, map: str) -> str:
    """
    Return redis sorted set key for the given game type and map.
    """

    game_type_key = b64encode(game_type)
    map_key = b64encode(map)

    if not game_type:
        if not map:
            return 'rank'
        else:
            return f'rank-map:{map_key}'
    else:
        if not map:
            return f'rank-gametype:{game_type_key}'
        else:
            return f'rank-gametype-map:{game_type_key}:{map_key}'


def get_elo(name: str, game_type: str, map: str) -> int:
    """
    Load player ELO for the given game type and map.
    """

    elo = redis.zscore(_key(game_type, map), name)
    return elo if elo else 1500


def set_elo(name: str, game_type: str, map: str, elo: int) -> None:
    """
    Save player ELO for the given game type.
    """

    redis.zadd(_key(game_type, map), {name: elo})


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


_GQL_GET_PLAYER = gql(
    """
    query ($name: String!) {
        getPlayer(name: $name) {
            name

            clan {
                name
            }
        }
    }
    """
)

def get(name: str) -> dict:
    """
    Get player with the given name.
    """

    try:
        player = graphql.execute(
            _GQL_GET_PLAYER,
            variable_values = {
                'name': name,
            }
        )['getPlayer']
    except:
        return {}

    return player if player else {}
