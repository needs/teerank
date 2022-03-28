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


def gametype_name(map_id: str) -> str:
    """
    Get gametype given the map ID.
    """

    return graphql("""
        query($map_id: ID!) {
            getMap(id: $map_id) {
                gameType {
                    name
                }
            }
        }
        """, {
            'map_id': map_id
        }
    )['getMap']['gameType']['name']


def id_all(map_id: str) -> str:
    """
    From the given map ID, get the map that has its name null for the given gametype.
    """

    gametype = graphql("""
        query($map_id: ID!) {
            getMap(id: $map_id) {
                gameType {
                    id
                    maps(filter: { name: { eq: null } }) {
                        id
                    }
                }
            }
        }
        """, {
            'map_id': map_id
        }
    )['getMap']['gameType']

    if gametype['maps']:
        return gametype['maps'][0]['id']

    return graphql("""
        mutation($map: AddMapInput!) {
            addMap(input: [$map]) {
                map {
                    id
                }
            }
        }
        """, {
            'map': {
                'name': None,
                'gameType': backend.database.gametype.ref(gametype['id'])
            },
        }
    )['addMap']['map'][0]['id']
