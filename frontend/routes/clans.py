"""
Implement /clans.
"""

from gql import gql
from flask import request, render_template
from frontend.database import graphql

import frontend.components.paginator
import frontend.components.section_tabs
from frontend.routes import blueprint

main_game_types = (
    'CTF',
    'DM',
    'TDM'
)

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

@blueprint.route('/clans', endpoint='clans')
def route_clans():
    """
    List of maps for a given game type.
    """

    game_type = request.args.get('gametype', default='CTF', type = str)
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
        tab = {
            'type': 'gametype',
            'gametype': game_type,
            'map': map_name
        },

        section_tabs = section_tabs,
        paginator = paginator,

        game_type=game_type,
        map_name=map_name,
        clans = clans,
        main_game_types=main_game_types
    )
