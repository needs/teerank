"""
Launch frontend application.
"""

from flask import Flask, render_template, url_for
from flask import request, abort
from gql import gql

import frontend.components.paginator
import frontend.components.section_tabs
import frontend.components.top_tabs
from frontend.database import graphql
import frontend.routes

#
# Flask
#

app = Flask(__name__)
app.register_blueprint(frontend.routes.blueprint)

@app.route('/maps')
def route_maps():
    """
    List of maps for a given game type.
    """

@app.route('/gametypes')
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
