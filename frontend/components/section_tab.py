"""
Section tab component.
"""

from gql import gql
from flask import url_for
from frontend.database import graphql

_GQL_QUERY_COUNTS = gql(
    """
    query {
        aggregatePlayer {
            count
        }
        aggregateClan {
            count
        }
        aggregateGameServer {
            count
        }
    }
    """
)

def init(active: str, counts = None, urls = None) -> dict:
    """
    Build a section tabs with the proper values.
    """

    if counts is None:
        results = dict(graphql.execute(_GQL_QUERY_COUNTS))

        counts = {
            'players': results['aggregatePlayer']['count'],
            'clans': results['aggregateClan']['count'],
            'servers': results['aggregateGameServer']['count']
        }

    if urls is None:
        urls = {
            'players': url_for('route_players'),
            'clans': url_for('route_clans'),
            'servers': url_for('route_servers')
        }

    return {
        'active': active,

        'players': {
            'url': urls['players'],
            'count': counts['players']
        },
        'clans': {
            'url': urls['clans'],
            'count': counts['clans']
        },
        'servers': {
            'url': urls['servers'],
            'count': counts['servers']
        }
    }


