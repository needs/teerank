"""
Implement clan related functions.
"""

from gql import gql
from shared.database import graphql


def ref(name):
    """Create a clan reference from the given clan name."""
    return { 'name': name } if name else None


_GQL_UPDATE_CLANS = gql(
    """
    mutation ($clans: [AddClanInput!]!) {
        addClan(input: $clans, upsert: true) {
            clan {
                name
            }
        }
    }
    """
)

def upsert(clans):
    """
    Add or update the given clans.
    """

    graphql.execute(
        _GQL_UPDATE_CLANS,
        variable_values = { 'clans': clans }
    )


_GQL_COMPUTE_CLANS_PLAYERS_COUNT = gql(
    """
    query ($clans: [String]!) {
        queryClan(filter: {name: {in: $clans}}) {
            name

            playersAggregate {
                count
            }
        }
    }
    """
)

def compute_player_count(clans):
    """
    Return a dictionary where for each given clans is associated its player
    count.
    """

    if not clans:
        return {}

    clans = graphql.execute(
        _GQL_COMPUTE_CLANS_PLAYERS_COUNT,
        variable_values = {
            'clans': clans
        }
    )['queryClan']

    return { clan['name']: clan['playersAggregate']['count'] for clan in clans }


_GQL_GET_CLAN_PLAYERS_COUNT = gql(
    """
    query ($name: String!) {
        getClan(name: $name) {
            playersCount
        }
    }
    """
)

def get_player_count(name: str) -> str:
    """
    Return clan player count or None if any.
    """

    clan = graphql.execute(
        _GQL_GET_CLAN_PLAYERS_COUNT,
        variable_values = {
            'name': name
        }
    )['getClan']

    if not clan:
        return None

    return clan['playersCount']


_GQL_UPDATE_CLAN_PLAYERS_COUNT = gql(
    """
    mutation ($clan: String!, $count: Int) {
        updateClan(input: {filter: {name: {eq: $clan}}, set: {playersCount: $count}}) {
            clan {
                name
            }
        }
    }
    """
)

def set_player_count(clan, count):
    """
    Set player count for the given clan.
    """

    graphql.execute(
        _GQL_UPDATE_CLAN_PLAYERS_COUNT,
        variable_values = { 'clan': clan, 'count': count }
    )

