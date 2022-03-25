"""
Operations on type GameType.
"""

from gql import gql
from backend.database import graphql


def ref(gametype_id):
    """Create a gametype reference from the given gametype ID."""
    return { 'id': gametype_id }


_GQL_GET = gql(
    """
    query get($name: String) {
        queryGameType(filter: { name: { eq: $name } }) {
            id
            name
        }
    }

    mutation create($gameType: AddGameTypeInput!) {
        addGameType(input: [$gameType]) {
            gameType {
                id
                name
            }
        }
    }
    """
)

def get(name: str) -> dict:
    """
    Get GameType with the given name, and create if it does not exist.
    """

    gametype = dict(graphql().execute(
        _GQL_GET,
        operation_name = 'get',
        variable_values = {
            'name': name,
        }
    ))['queryGameType']

    if not gametype:
        gametype = dict(graphql().execute(
            _GQL_GET,
            operation_name = 'create',
            variable_values = {
                'gameType': {
                    'name': name
                },
            }
        ))['addGameType']['gameType']

    return gametype[0]
