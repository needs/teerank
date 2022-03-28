"""
Implement /gametypes.
"""

from flask import render_template

from shared.database.graphql import graphql
import frontend.components.top_tabs

def route_gametypes():
    """
    List of all game types.
    """

    first, offset = frontend.components.paginator.info()

    query = graphql("""
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
        """, {
            'offset': offset,
            'first': first
        }
    )

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
