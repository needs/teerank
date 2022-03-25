"""
Implement /clans.
"""

from gql import gql
from flask import request, render_template
from frontend.database import graphql

import frontend.components.paginator
import frontend.components.section_tabs
import frontend.components.top_tabs

_GQL_QUERY_CLANS = gql(
    """
    query($first: Int!, $offset: Int!) {
        queryClan(first: $first, offset: $offset, order: {desc: playersCount}) {
            name
            playersCount
        }
    }
    """
)

def route_clans():
    """
    List of maps for a given game type.
    """

    game_type = request.args.get('gametype', default=None, type = str)
    map_name = request.args.get('map', default=None, type = str)

    section_tabs = frontend.components.section_tabs.init('clans')
    paginator = frontend.components.paginator.init(section_tabs['clans']['count'])

    clans = dict(graphql.execute(
        _GQL_QUERY_CLANS,
        variable_values = {
            'offset': paginator['offset'],
            'first': paginator['first']
        }
    ))['queryClan']

    for i, clan in enumerate(clans):
        clan['rank'] = paginator['offset'] + i + 1

    return render_template(
        'clans.html',
        top_tabs = frontend.components.top_tabs.init({
            'type': 'gametype',
            'gametype': game_type,
            'map': map_name
        }),

        section_tabs = section_tabs,
        paginator = paginator,

        game_type=game_type,
        map_name=map_name,
        clans = clans,
    )
