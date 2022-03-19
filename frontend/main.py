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

def _section_tab(active: str, counts = None, urls = None) -> dict:
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

    if urls is None:
        urls = {
            'players': url_for('route_players'),
            'clans': url_for('route_clans'),
            'servers': url_for('route_servers')
        }

    return {
        'active': active,

        'players': {
            'url': urls['players'],
            'count': counts['players']
        },
        'clans': {
            'url': urls['clans'],
            'count': counts['clans']
        },
        'servers': {
            'url': urls['servers'],
            'count': counts['servers']
        }
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
    paginator = _paginator(section_tab['players']['count'])

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
    paginator = _paginator(section_tab['clans']['count'])

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
    paginator = _paginator(section_tab['servers']['count'])

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

_GQL_SEARCH = gql(
    """
    query counts($query: String!) {
        aggregatePlayer(filter: { name: { regexp: $query }}) {
            count
        }
        aggregateClan(filter: { name: { regexp: $query }}) {
            count
        }
        aggregateGameServer(filter: { name: { regexp: $query }}) {
            count
        }
    }

    query searchPlayers($query: String!, $first: Int!) {
        queryPlayer(filter: { name: { regexp: $query }}, first: $first) {
            name
            clan {
                name
            }
        }
    }

    query searchClans($query: String!, $first: Int!) {
        queryClan(
            filter: { name: { regexp: $query }},
            first: $first,
            order: { desc: playersCount }
        ) {
            name
            playersCount
        }
    }

    query searchServers($query: String!, $first: Int!) {
        queryGameServer(
            filter: { name: { regexp: $query }},
            first: $first,
            order: { desc: numClients }
        ) {
            address
            name
            gameType
            map
            numClients
            maxClients
        }
    }
    """
)

def search(template_name, section_tab_active, operation_name, query_name):
    """
    Search for the given category.
    """

    query = request.args.get('q', default='', type = str)
    regexp = '/.*' + re.escape(query) + '.*/i'

    if query == '':
        # If nothing is done, an empty query will juste return all entries.
        # There is no real use case for such query so instead just return
        # nothing to avoid overcharging dgraph.

        counts = {
            'players': 0,
            'clans': 0,
            'servers': 0
        }

        results = []
    else:
        # Count the number of results for players, clans and servers.

        counts_results = dict(graphql.execute(
            _GQL_SEARCH,
            operation_name = 'counts',
            variable_values = {
                'query': regexp
            }
        ))

        counts = {
            'players': counts_results['aggregatePlayer']['count'],
            'clans': counts_results['aggregateClan']['count'],
            'servers': counts_results['aggregateGameServer']['count']
        }

        # Do the real search query.

        results = dict(graphql.execute(
            _GQL_SEARCH,
            operation_name = operation_name,
            variable_values = {
                'query': regexp,
                'first': MAX_SEARCH_RESULTS
            }
        ))[query_name]

    # Build the page.

    urls = {
        'players': url_for('route_players_search', q = query),
        'clans': url_for('route_clans_search', q = query),
        'servers': url_for('route_servers_search', q = query)
    }

    # If the number of results exceed the maximum number of results, add a small
    # '+' to indicate that too many results were found.

    for key, count in counts.items():
        if count > MAX_SEARCH_RESULTS:
            counts[key] = f'{MAX_SEARCH_RESULTS}+'

    section_tab = _section_tab(
        section_tab_active, counts, urls
    )

    return render_template(
        template_name,

        tab = {
            'type': 'custom'
        },

        section_tab = section_tab,
        query = query,
        results = results,

        main_game_types = main_game_types
    )


@app.route('/players/search')
def route_players_search():
    """
    List of players matching the search query.
    """

    return search('search_players.html', 'players', 'searchPlayers', 'queryPlayer')


@app.route('/clans/search')
def route_clans_search():
    """
    List of clans matching the search query.
    """

    return search('search_clans.html', 'clans', 'searchClans', 'queryClan')


@app.route('/servers/search')
def route_servers_search():
    """
    List of servers matching the search query.
    """

    return search('search_servers.html', 'servers', 'searchServers', 'queryGameServer')
