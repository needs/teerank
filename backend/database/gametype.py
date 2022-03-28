"""
Operations on type GameType.
"""

from shared.database.graphql import graphql


def ref(gametype_id):
    """Create a gametype reference from the given gametype ID."""
    return { 'id': gametype_id }


def get(name: str) -> dict:
    """
    Get GameType with the given name, and create if it does not exist.
    """

    gametype = graphql("""
        query($name: String) {
            queryGameType(filter: { name: { eq: $name } }) {
                id
                name
            }
        }
        """, {
            'name': name,
        }
    )['queryGameType']

    if not gametype:
        gametype = graphql("""
            mutation($gameType: AddGameTypeInput!) {
                addGameType(input: [$gameType]) {
                    gameType {
                        id
                        name
                    }
                }
            }
            """, {
                'gameType': {
                    'name': name
                },
            }
        )['addGameType']['gameType']

    return gametype[0]
