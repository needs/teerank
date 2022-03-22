"""
Implement /gametypes.
"""

from flask import render_template

import frontend.components.top_tabs
from frontend.routes import blueprint

@blueprint.route('/gametypes', endpoint='gametypes')
def route_gametypes():
    """
    List of all game types.
    """

    return render_template(
        'gametypes.html',
        top_tabs = frontend.components.top_tabs.init({
            'type': '...'
        }),
        game_type = None,
        map_name = None,
        game_types = ['CTF'],
    )
