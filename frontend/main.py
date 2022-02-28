"""
Launch frontend application.
"""

from flask import Flask, render_template, url_for
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

_GQL_QUERY_COUNTS = gql(
    """
    query {
        aggregatePlayer {
            count
        }
        aggregateClan {
            count
        }
        aggregateGameServer {
            count
        }
    }
    """
)

def _section_tab(type: str) -> dict:
    """
    Build a section tabs with the proper values.
    """

    counts = graphql.execute(_GQL_QUERY_COUNTS)

    return {
        'type': type,
        'players_count': counts['aggregatePlayer']['count'],
        'clans_count': counts['aggregateClan']['count'],
        'servers_count': counts['aggregateGameServer']['count']
    }


PAGE_SIZE = 100
PAGE_SPREAD = 2

def _paginator(count: int) -> dict:
    """
    Build paginator links.
    """

    links = []

    # URL settings for the page links.

    endpoint = request.endpoint
    args = request.args.copy()

    try:
        page = int(args.pop('page'))
    except:
        page = 1

    last_page = int((count - 1) / PAGE_SIZE)

    # Add the "Previous" button.

    links.append({
        'type': 'button',
        'name': 'Previous',
        'class': 'previous',
        'href': url_for(endpoint, page=page-1, **args) if page > 1 else None
    })

    # Add the first page.

    if page > PAGE_SPREAD + 1:
        links.append({
            'type': 'page',
            'page': 1,
            'href': url_for(endpoint, page=1, **args)
        })

    # Notice any missing pages between the fist page and the start of the page spread.

    if page > PAGE_SPREAD + 2:
        links.append({
            'type': '...'
        })

    # Start page spread.

    for i in range(page-PAGE_SPREAD, page):
        if i > 0:
            links.append({
                'type': 'page',
                'page': i,
                'href': url_for(endpoint, page=i, **args)
            })

    # Add the current page.

    links.append({
        'type': 'current',
        'page': page
    })

    # End page spread.

    for i in range(page + 1, page + PAGE_SPREAD + 1):
        if i <= last_page:
            links.append({
                'type': 'page',
                'page': i,
                'href': url_for(endpoint, page=i, **args)
            })

    # Notice any missing pages between the end of the page spread and the last page.

    if page < last_page - PAGE_SPREAD - 1:
        links.append({
            'type': '...'
        })

    # Add the last page.

    if page < last_page - PAGE_SPREAD:
        links.append({
            'type': 'page',
            'page': last_page,
            'href': url_for(endpoint, page=last_page, **args)
        })

    # Add the "Next" button.

    links.append({
        'type': 'button',
        'name': 'Next',
        'class': 'next',
        'href': url_for(endpoint, page=page+1, **args) if page < last_page else None
    })

    return links


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

    section_tab = _section_tab('players')

    result = graphql.execute(_GQL_QUERY_PLAYERS)

    return render_template(
        'players.html',
        tab = {
            'type': 'gametype',
            'gametype': game_type,
            'map': map_name
        },

        section_tab = section_tab,
        paginator = _paginator(section_tab['players_count']),

        game_type=game_type,
        map_name=map_name,
        players=result['queryPlayer'],
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

    section_tab = _section_tab('clans')

    result = graphql.execute(_GQL_QUERY_CLANS)

    return render_template(
        'clans.html',
        tab = {
            'type': 'gametype',
            'gametype': game_type,
            'map': map_name
        },

        section_tab = section_tab,
        paginator = _paginator(section_tab['clans_count']),

        game_type=game_type,
        map_name=map_name,
        clans=result['queryClan'],
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

    section_tab = _section_tab('servers')

    result = graphql.execute(_GQL_QUERY_SERVERS)

    return render_template(
        'servers.html',
        tab = {
            'type': 'gametype',
            'gametype': game_type,
            'map': map_name
        },

        section_tab = section_tab,
        paginator = _paginator(section_tab['servers_count']),

        game_type=game_type,
        map_name=map_name,
        servers=result['queryGameServer'],
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
