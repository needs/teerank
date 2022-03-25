"""
Implement Player and PlayerELO classes.
"""

from gql import gql
from backend.database import graphql

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
