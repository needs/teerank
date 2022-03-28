"""
Implement /maps.
"""

from flask import render_template, request, abort

from shared.database.graphql import graphql
import frontend.components.top_tabs

def route_maps():
    """
    List of maps for a given game type.
    """

    gametype = request.args.get('gametype', default=None, type = str)

    first, offset = frontend.components.paginator.info()

    query = graphql("""
        query($gametype: String, $first: Int!, $offset: Int!) {
            queryGameType(filter: { name: { eq: $gametype }}) {
                maps(first: $first, offset: $offset, filter: { has: name }) {
                    name

                    playerInfosAggregate {
                        count
                    }
                }

                mapsAggregate(filter: { has: name }) {
                    count
                }
            }
        }
        """, {
            'gametype': gametype,
            'offset': offset,
            'first': first
        }
    )

    if not query['queryGameType']:
        abort(404)

    maps = query['queryGameType'][0]['maps']
    count = query['queryGameType'][0]['mapsAggregate']['count']

    paginator = frontend.components.paginator.init(count)

    for i, map_ in enumerate(maps):
        map_['rank'] = paginator['offset'] + i + 1

    return render_template(
        'maps.html',

        gametype = gametype,

        top_tabs = frontend.components.top_tabs.init({
            'type': 'gametype',
            'gametype': gametype,
            'map': None
        }),

        paginator = paginator,
        maps = maps,
    )
