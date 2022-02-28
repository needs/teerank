"""
Launch frontend application.
"""

from flask import Flask, render_template
from flask import request, abort
from gql import gql

from shared.database import graphql
import shared.game_server
import shared.player
import shared.clan

app = Flask(__name__)

main_game_types = (
    'CTF',
    'DM',
    'TDM'
)

_GQL_QUERY_PLAYERS = gql(
    """
    query {
        queryPlayer {
            name
            clan {
                name
            }
        }
    }
    """
)

@app.route("/players")
def players():
    """
    List players for a specific game type and map.
    """

    game_type = request.args.get('gametype', default='CTF', type = str)
    map_name = request.args.get('map', default=None, type = str)

    result = graphql.execute(_GQL_QUERY_PLAYERS)

    return render_template(
        'players.html',
        tab = {
            'type': 'gametype',
            'gametype': game_type,
            'map': map_name
        },
        game_type=game_type,
        map_name=map_name,
        players=result['queryPlayer'],
        players_count=len(result['queryPlayer']),
        main_game_types=main_game_types
    )


@app.route("/player")
def player():
    """
    Show a single player.
    """

    name = request.args.get('name', default=None, type=str)
    player = shared.player.get(name)

    if not player:
        abort(404)

    return render_template(
        'player.html',
        tab = {
            'type': 'custom'
        },
        player=player,
        main_game_types=main_game_types
    )


_GQL_QUERY_CLANS = gql(
    """
    query {
        queryClan {
            name
            playersCount
        }
    }
    """
)

@app.route('/clans')
def clans():
    """
    List of maps for a given game type.
    """

    game_type = request.args.get('gametype', default='CTF', type = str)
    map_name = request.args.get('map', default=None, type = str)

    result = graphql.execute(_GQL_QUERY_CLANS)

    return render_template(
        'clans.html',
        tab = {
            'type': 'gametype',
            'gametype': game_type,
            'map': map_name
        },
        game_type=game_type,
        map_name=map_name,
        clans=result['queryClan'],
        clans_count=len(result['queryClan']),
        main_game_types=main_game_types
    )


_GQL_GET_CLAN = gql(
    """
    query($name: String!) {
        getClan(name: $name) {
            name

            players {
                name
            }
        }
    }
    """
)

@app.route("/clan")
def clan():
    """
    Show a single clan.
    """

    name = request.args.get('name', default=None, type=str)

    clan = graphql.execute(
        _GQL_GET_CLAN,
        variable_values = {
            'name': name
        }
    )['getClan']

    if not clan:
        abort(404)

    return render_template(
        'clan.html',
        tab = {
            'type': 'custom'
        },
        clan=clan,
        main_game_types=main_game_types
    )


_GQL_QUERY_SERVERS = gql(
    """
    query {
        queryGameServer(order: {desc: numClients}) {
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

@app.route('/servers')
def servers():
    """
    List of maps for a given game type.
    """

    game_type = request.args.get('gametype', default='CTF', type = str)
    map_name = request.args.get('map', default=None, type = str)

    result = graphql.execute(_GQL_QUERY_SERVERS)

    return render_template(
        'servers.html',
        tab = {
            'type': 'gametype',
            'gametype': game_type,
            'map': map_name
        },
        game_type=game_type,
        map_name=map_name,
        servers=result['queryGameServer'],
        servers_count=len(result['queryGameServer']),
        main_game_types=main_game_types
    )


@app.route('/server')
def server():
    """
    Show a single server.
    """

    address = request.args.get('address', default="", type=str)
    server = shared.game_server.get(address)

    if not server:
        abort(404)

    return render_template(
        'server.html',
        tab = {
            'type': 'custom'
        },
        server=server,
        main_game_types=main_game_types
    )


@app.route('/maps')
def maps():
    """
    List of maps for a given game type.
    """


@app.route('/gametypes')
def gametypes():
    """
    List of all game types.
    """

    return render_template(
        'gametypes.html',
        tab = {
            'type': 'gametypes_list'
        },
        game_type=None,
        map_name=None,
        game_types=['CTF'],
        main_game_types=main_game_types
    )
