"""
Operations on type Map.
"""

from shared.database.graphql import graphql

import backend.database.gametype


def ref(map_id):
    """Create a map reference from the given map ID."""
    return { 'id': map_id }


def get(gametype_id: str, name: str) -> dict:
    """
    Get Map with the given name for the given gametype ID, and create if it does not exist.
    """

    value = graphql("""
        query($gametype_id: ID!, $map_name: String) {
            getGameType(id: $gametype_id) {
                maps(filter: { name: {eq: $map_name }}) {
                    id
                    name
                    gameType {
                        id
                    }
                }
            }
        }
        """, {
            'gametype_id': gametype_id,
            'map_name': name,
        }
    )['getGameType']['maps']

    if not value:
        value = graphql("""
            mutation($map: AddMapInput!) {
                addMap(input: [$map]) {
                    map {
                        id
                        name
                        gameType {
                            id
                        }
                    }
                }
            }
            """, {
                'map': {
                    'name': name,
                    'gameType': backend.database.gametype.ref(gametype_id)
                },
            }
        )['addMap']['map']

    return value[0]
