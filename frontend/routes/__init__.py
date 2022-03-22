"""
Routes module.
"""

from flask import Blueprint

blueprint = Blueprint('routes', __name__)

#
# For convenience we want to import all routes here, however since all routes
# will use the blueprint variable, we need to declare blueprint before importing
# the routes.
#
# pylint: disable=wrong-import-position

from .players import route_players
from .clans import route_clans
from .servers import route_servers
from .player import route_player
from .clan import route_clan
from .server import route_server
from .about import route_about
from .status import route_status
from .search import route_players_search, route_clans_search, route_servers_search
from .gametypes import route_gametypes
from .maps import route_maps
