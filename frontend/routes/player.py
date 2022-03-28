"""
Implement /player.
"""

from flask import request, render_template, abort

from shared.database.graphql import graphql
import frontend.components.top_tabs


def route_player():
    """
    Show a single player.
    """

    name = request.args.get('name', default=None, type=str)

    player = graphql("""
        query ($name: String!) {
            getPlayer(name: $name) {
                name
                clan {
                    name
                }
            }
        }
        """, {
            'name': name,
        }
    )['getPlayer']

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
