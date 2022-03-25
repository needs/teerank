"""
Implement /gametypes.
"""

from gql import gql
from flask import render_template

from frontend.database import graphql
import frontend.components.top_tabs

_GQL_QUERY_GAMETYPES = gql(
    """
    query($first: Int!, $offset: Int!) {
        queryGameType(first: $first, offset: $offset) {
            name

            maps(filter: { name: null }) {
                playerInfosAggregate {
                    count
                }
            }

            mapsAggregate(filter: { has: name }) {
                count
            }
        }

        aggregateGameType(filter: { has: name }) {
            count
        }
    }
    """
)

def route_gametypes():
    """
    List of all game types.
    """

    first, offset = frontend.components.paginator.info()

    query = dict(graphql.execute(
        _GQL_QUERY_GAMETYPES,
        variable_values = {
            'offset': offset,
            'first': first
        }
    ))

    gametypes = query['queryGameType']
    count = query['aggregateGameType']['count']

    paginator = frontend.components.paginator.init(count)

    for i, gametype in enumerate(gametypes):
        gametype['rank'] = paginator['offset'] + i + 1

        if not gametype['maps']:
            gametype['maps'] = [{'playerInfosAggregate': {'count': 0}}]

    return render_template(
        'gametypes.html',

        top_tabs = frontend.components.top_tabs.init({
            'type': '...'
        }),

        paginator = paginator,
        gametypes = gametypes,
    )
