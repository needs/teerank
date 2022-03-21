"""
Implement Player and PlayerELO classes.
"""

from base64 import b64encode
from gql import gql
from backend.database import redis, graphql

def _key(game_type: str, map_name: str) -> str:
    """
    Return redis sorted set key for the given game type and map.
    """

    game_type = b64encode(game_type.encode()) if game_type else None
    map_name = b64encode(map_name.encode()) if map_name else None

    if not game_type:
        return f'rank-map:{map_name}' if map_name else 'rank'

    return f'rank-gametype-map:{game_type}:{map_name}' if map_name else f'rank-gametype:{game_type}'


def get_elo(name: str, game_type: str, map_name: str) -> int:
    """
    Load player ELO for the given game type and map.
    """

    elo = redis().zscore(_key(game_type, map_name), name)
    return elo if elo else 1500


def set_elo(name: str, game_type: str, map_name: str, elo: int) -> None:
    """
    Save player ELO for the given game type.
    """

    redis().zadd(_key(game_type, map_name), {name: elo})


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

    graphql().execute(
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

    players = dict(graphql().execute(
        _GQL_QUERY_PLAYERS_CLAN,
        variable_values = { 'players': players }
    ))['queryPlayer']

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

    player = dict(graphql().execute(
        _GQL_GET_PLAYER,
        variable_values = {
            'name': name,
        }
    ))['getPlayer']

    return player if player else {}
