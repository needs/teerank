"""
Launch frontend application.
"""

from flask import Flask, render_template
from flask import request
from gql import gql

from shared.database import graphql

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
        game_type=game_type,
        map_name=map_name,
        players=result['queryPlayer'],
        players_count=len(result['queryPlayer']),
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
        game_type=game_type,
        map_name=map_name,
        clans=result['queryClan'],
        clans_count=len(result['queryClan']),
        main_game_types=main_game_types
    )


@app.route('/servers')
def servers():
    """
    List of maps for a given game type.
    """


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
        game_type=None,
        map_name=None,
        game_types=['CTF'],
        main_game_types=main_game_types
    )
