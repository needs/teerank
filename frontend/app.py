"""
Main flask application.
"""

from flask import Flask

from frontend.routes.players import route_players
from frontend.routes.clans import route_clans
from frontend.routes.servers import route_servers
from frontend.routes.gametypes import route_gametypes
from frontend.routes.maps import route_maps
from frontend.routes.player import route_player
from frontend.routes.clan import route_clan
from frontend.routes.server import route_server
from frontend.routes.about import route_about
from frontend.routes.status import route_status
from frontend.routes.search import route_players_search, route_clans_search, route_servers_search

def create_app():
    """
    Creates Flask application.
    """

    app = Flask(__name__)

    app.route('/players', endpoint='players')(route_players)
    app.route('/clans', endpoint='clans')(route_clans)
    app.route('/servers', endpoint='servers')(route_servers)
    app.route('/gametypes', endpoint='gametypes')(route_gametypes)
    app.route('/maps', endpoint='maps')(route_maps)

    app.route('/player', endpoint='player')(route_player)
    app.route('/clan', endpoint='clan')(route_clan)
    app.route('/server', endpoint='server')(route_server)

    app.route('/about', endpoint='about')(route_about)
    app.route('/status', endpoint='status')(route_status)

    app.route('/players/search', endpoint='players-search')(route_players_search)
    app.route('/clans/search', endpoint='clans-search')(route_clans_search)
    app.route('/servers/search', endpoint='servers-search')(route_servers_search)

    return app
