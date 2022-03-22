"""
Implement /player.
"""

from gql import gql
from flask import request, render_template, abort
from frontend.database import graphql

import frontend.components.top_tabs
from frontend.routes import blueprint

_GQL_GET_PLAYER = gql(
    """
    query ($name: String!) {
        getPlayer(name: $name) {
            name
            clan {
                name
            }
        }
    }
    """
)

@blueprint.route('/player', endpoint='player')
def route_player():
    """
    Show a single player.
    """

    name = request.args.get('name', default=None, type=str)

    player = dict(graphql.execute(
        _GQL_GET_PLAYER,
        variable_values = {
            'name': name,
        }
    ))['getPlayer']

    if not player:
        abort(404)

    return render_template(
        'player.html',
        top_tabs = frontend.components.top_tabs.init({
            'type': 'custom',
            'links': [{
                'name': 'Player'
            }, {
                'name': name
            }]
        }),
        player = player,
    )
