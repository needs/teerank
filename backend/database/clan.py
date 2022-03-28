"""
Implement clan related functions.
"""

from shared.database.graphql import graphql


def ref(name):
    """Create a clan reference from the given clan name."""
    return { 'name': name } if name else None


def upsert(clans):
    """
    Add or update the given clans.
    """

    graphql("""
        mutation ($clans: [AddClanInput!]!) {
            addClan(input: $clans, upsert: true) {
                clan {
                    name
                }
            }
        }
        """, {
            'clans': clans
        }
    )


def compute_player_count(clans):
    """
    Return a dictionary where for each given clans is associated its player
    count.
    """

    if not clans:
        return {}

    clans = graphql("""
        query ($clans: [String]!) {
            queryClan(filter: {name: {in: $clans}}) {
                name

                playersAggregate {
                    count
                }
            }
        }
        """, {
            'clans': clans
        }
    )['queryClan']

    return { clan['name']: clan['playersAggregate']['count'] for clan in clans }


def get_player_count(name: str) -> str:
    """
    Return clan player count or None if any.
    """

    clan = graphql("""
        query ($name: String!) {
            getClan(name: $name) {
                playersCount
            }
        }
        """, {
            'name': name
        }
    )['getClan']

    if not clan:
        return None

    return clan['playersCount']


def set_player_count(clan, count):
    """
    Set player count for the given clan.
    """

    graphql("""
        mutation ($clan: String!, $count: Int) {
            updateClan(input: {filter: {name: {eq: $clan}}, set: {playersCount: $count}}) {
                clan {
                    name
                }
            }
        }
        """, {
            'clan': clan, 'count': count
        }
    )
