"""
Section tabs component.
"""

from flask import url_for
from shared.database.graphql import graphql

def init(active: str, counts = None, urls = None) -> dict:
    """
    Build a section tabs with the proper values.
    """

    if counts is None:
        results = graphql("""
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

        counts = {
            'players': results['aggregatePlayer']['count'],
            'clans': results['aggregateClan']['count'],
            'servers': results['aggregateGameServer']['count']
        }

    if urls is None:
        urls = {
            'players': url_for('players'),
            'clans': url_for('clans'),
            'servers': url_for('servers')
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
