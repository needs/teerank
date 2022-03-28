"""
Implement Player and PlayerELO classes.
"""

from shared.database.graphql import graphql

def ref(name):
    """Create a player reference from the given player name."""
    return { 'name': name } if name else None


def upsert(players):
    """
    Add or update the given players.
    """

    graphql("""
        mutation ($players: [AddPlayerInput!]!) {
            addPlayer(input: $players, upsert: true) {
                player {
                    name
                }
            }
        }
        """, {
            'players': players
        }
    )


def get_clan(players):
    """
    Get clan for each of the given players.
    """

    players = graphql("""
        query ($players: [String]!) {
            queryPlayer(filter: {name: {in: $players}}) {
                name
                clan {
                    name
                }
            }
        }
        """, {
            'players': players
        }
    )['queryPlayer']

    return { player['name']: player['clan'] for player in players }


def get(name: str) -> dict:
    """
    Get player with the given name.
    """

    player = graphql("""
        query ($name: String!) {
            getPlayer(name: $name) {
                name

                clan {
                    name
                }
            }
        }
        """, {
            'name': name,
        }
    )['getPlayer']

    return player if player else {}
