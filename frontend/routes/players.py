"""
Implement /players.
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

_GQL_QUERY_PLAYERS = gql(
    """
    query($first: Int!, $offset: Int!) {
        queryPlayer(first: $first, offset: $offset) {
            name
            clan {
                name
            }
        }
    }
    """
)

@blueprint.route('/players', endpoint='players')
def route_players():
    """
    List players for a specific game type and map.
    """

    game_type = request.args.get('gametype', default='CTF', type = str)
    map_name = request.args.get('map', default=None, type = str)

    section_tabs = frontend.components.section_tabs.init('players')
    paginator = frontend.components.paginator.init(section_tabs['players']['count'])

    players = dict(graphql.execute(
        _GQL_QUERY_PLAYERS,
        variable_values = {
            'offset': paginator['offset'],
            'first': paginator['first']
        }
    ))['queryPlayer']

    for i, player in enumerate(players):
        player['rank'] = paginator['offset'] + i + 1

    return render_template(
        'players.html',
        tab = {
            'type': 'gametype',
            'gametype': game_type,
            'map': map_name
        },

        section_tabs = section_tabs,
        paginator = paginator,

        game_type=game_type,
        map_name=map_name,
        players = players,
        main_game_types=main_game_types
    )
