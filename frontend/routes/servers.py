"""
Implement /servers.
"""

from gql import gql
from flask import request, render_template
from frontend.database import graphql

import frontend.components.paginator
import frontend.components.section_tabs
import frontend.components.top_tabs

_GQL_QUERY_SERVERS = gql(
    """
    query($first: Int!, $offset: Int!) {
        queryGameServer(first: $first, offset: $offset, order: {desc: numClients}) {
            address
            name
            map
            gameType
            numClients
            maxClients
        }
    }
    """
)

def route_servers():
    """
    List of maps for a given game type.
    """

    game_type = request.args.get('gametype', default='CTF', type = str)
    map_name = request.args.get('map', default=None, type = str)

    section_tabs = frontend.components.section_tabs.init('servers')
    paginator = frontend.components.paginator.init(section_tabs['servers']['count'])

    servers = dict(graphql.execute(
        _GQL_QUERY_SERVERS,
        variable_values = {
            'offset': paginator['offset'],
            'first': paginator['first']
        }
    ))['queryGameServer']

    for i, server in enumerate(servers):
        server['rank'] = paginator['offset'] + i + 1

    return render_template(
        'servers.html',
        top_tabs = frontend.components.top_tabs.init({
            'type': 'gametype',
            'gametype': game_type,
            'map': map_name
        }),

        section_tabs = section_tabs,
        paginator = paginator,

        game_type = game_type,
        map_name = map_name,
        servers = servers,
    )
