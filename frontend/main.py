"""
Launch frontend application.
"""

import datetime
import re

from flask import Flask, render_template, url_for
from flask import request, abort
from gql import gql

from shared.database import graphql
import shared.game_server
import shared.master_server
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

def _section_tab(tab_type: str, counts = None) -> dict:
    """
    Build a section tabs with the proper values.
    """

    if counts is None:
        results = dict(graphql.execute(_GQL_QUERY_COUNTS))

        counts = {
            'players': results['aggregatePlayer']['count'],
            'clans': results['aggregateClan']['count'],
            'servers': results['aggregateGameServer']['count']
        }

    return {
        'type': tab_type,
        'players_count': counts['players'],
        'clans_count': counts['clans'],
        'servers_count': counts['servers']
    }


def clamp(minval, maxval, val):
    """Clamp val between minval and maxval."""
    return max(minval, min(maxval, val))

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
    except KeyError:
        page = 1
    except ValueError:
        page = 1

    last_page = int((count - 1) / PAGE_SIZE) + 1
    page = clamp(1, last_page, page)

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

    return {
        'first': PAGE_SIZE,
        'offset': (page - 1) * PAGE_SIZE,
        'links': links
    }


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

@app.route("/players")
def route_players():
    """
    List players for a specific game type and map.
    """

    game_type = request.args.get('gametype', default='CTF', type = str)
    map_name = request.args.get('map', default=None, type = str)

    section_tab = _section_tab('players')
    paginator = _paginator(section_tab['players_count'])

    result = dict(graphql.execute(
        _GQL_QUERY_PLAYERS,
        variable_values = {
            'offset': paginator['offset'],
            'first': paginator['first']
        }
    ))

    return render_template(
        'players.html',
        tab = {
            'type': 'gametype',
            'gametype': game_type,
            'map': map_name
        },

        section_tab = section_tab,
        paginator = paginator,

        game_type=game_type,
        map_name=map_name,
        players=result['queryPlayer'],
        main_game_types=main_game_types
    )


@app.route("/player")
def route_player():
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
    query($first: Int!, $offset: Int!) {
        queryClan(first: $first, offset: $offset, order: {desc: playersCount}) {
            name
            playersCount
        }
    }
    """
)

@app.route('/clans')
def route_clans():
    """
    List of maps for a given game type.
    """

    game_type = request.args.get('gametype', default='CTF', type = str)
    map_name = request.args.get('map', default=None, type = str)

    section_tab = _section_tab('clans')
    paginator = _paginator(section_tab['clans_count'])

    result = dict(graphql.execute(
        _GQL_QUERY_CLANS,
        variable_values = {
            'offset': paginator['offset'],
            'first': paginator['first']
        }
    ))

    return render_template(
        'clans.html',
        tab = {
            'type': 'gametype',
            'gametype': game_type,
            'map': map_name
        },

        section_tab = section_tab,
        paginator = paginator,

        game_type=game_type,
        map_name=map_name,
        clans=result['queryClan'],
        main_game_types=main_game_types
    )


_GQL_GET_CLAN = gql(
    """
    query($name: String!, $first: Int!, $offset: Int!) {
        getClan(name: $name) {
            name

            players(first: $first, offset: $offset) {
                name
            }
        }
    }
    """
)

@app.route("/clan")
def route_clan():
    """
    Show a single clan.
    """

    name = request.args.get('name', default=None, type=str)
    paginator = _paginator(shared.clan.get_player_count(name))

    clan = dict(graphql.execute(
        _GQL_GET_CLAN,
        variable_values = {
            'name': name,
            'first': paginator['first'],
            'offset': paginator['offset']
        }
    ))['getClan']

    if not clan:
        abort(404)

    return render_template(
        'clan.html',
        tab = {
            'type': 'custom'
        },
        paginator = paginator,
        clan=clan,
        main_game_types=main_game_types
    )


_GQL_QUERY_SERVERS = gql(
    """
    query($first: Int!, $offset: Int!) {
        queryGameServer(first: $first, offset: $offset, order: {desc: numClients}) {
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
def route_servers():
    """
    List of maps for a given game type.
    """

    game_type = request.args.get('gametype', default='CTF', type = str)
    map_name = request.args.get('map', default=None, type = str)

    section_tab = _section_tab('servers')
    paginator = _paginator(section_tab['servers_count'])

    result = dict(graphql.execute(
        _GQL_QUERY_SERVERS,
        variable_values = {
            'offset': paginator['offset'],
            'first': paginator['first']
        }
    ))

    return render_template(
        'servers.html',
        tab = {
            'type': 'gametype',
            'gametype': game_type,
            'map': map_name
        },

        section_tab = section_tab,
        paginator = paginator,

        game_type=game_type,
        map_name=map_name,
        servers=result['queryGameServer'],
        main_game_types=main_game_types
    )


@app.route('/server')
def route_server():
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
        tab = {
            'type': 'gametypes_list'
        },
        game_type=None,
        map_name=None,
        game_types=['CTF'],
        main_game_types=main_game_types
    )


@app.route('/about')
def route_about():
    """
    Render the about page.
    """

    return render_template(
        'about.html',
        tab = {
            'type': 'about'
        },
        main_game_types=main_game_types
    )


_GQL_QUERY_MASTER_SERVERS = gql(
    """
    query {
        queryMasterServer {
            address
            downSince
            gameServersAggregate {
                count
            }
        }
    }
    """
)

@app.route('/status')
def route_status():
    """
    Render the status page.
    """

    master_servers = dict(graphql.execute(
        _GQL_QUERY_MASTER_SERVERS
    ))['queryMasterServer']

    for master_server in master_servers:
        if master_server['downSince']:
            master_server['status'] = 'Down'
            master_server['status_class'] = 'down'
            if master_server['downSince'] != datetime.datetime.min.isoformat() + 'Z':
                master_server['comment'] = master_server['downSince']
        else:
            master_server['status'] = 'OK'
            master_server['status_class'] = 'ok'
            master_server['comment'] = f'{master_server["gameServersAggregate"]["count"]} servers'

    return render_template(
        'status.html',
        tab = {
            'type': 'custom'
        },
        master_servers = master_servers,
        main_game_types=main_game_types
    )


MAX_SEARCH_RESULTS = 100

_GQL_SEARCH_PLAYERS = gql(
    """
    query($query: String!, $first: Int!) {
        queryPlayer(filter: { name: { regexp: $query }}, first: $first) {
            name,
            clan {
                name
            }
        }
    }
    """
)

@app.route('/search')
def route_search():
    """
    List of players matching the search request.
    """

    query = request.args.get('q', default='', type = str)

    result = dict(graphql.execute(
        _GQL_SEARCH_PLAYERS,
        variable_values = {
            'query': "/.*" + re.escape(query) + ".*/i",
            'first': MAX_SEARCH_RESULTS + 1
        }
    ))

    counts = {
        'players': len(result['queryPlayer']),
        'clans': 0,
        'servers': 0
    }

    # Search results are not paginated in order to limit the number of searches.
    # So when the number of search results exceed the maximum, we just add a
    # '+'' next to the number of results.

    for key, count in counts.items():
        if count > MAX_SEARCH_RESULTS:
            counts[key] = f'{MAX_SEARCH_RESULTS}+'

    section_tab = _section_tab(
        'players', counts
    )

    return render_template(
        'search.html',

        tab = {
            'type': 'custom'
        },

        section_tab = section_tab,
        query = query,
        players = result['queryPlayer'],

        main_game_types = main_game_types
    )
