"""
Launch frontend application.
"""

import datetime
import re

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

_GQL_GET_PLAYER = gql(
    """
    query ($name: String!) {
        getPlayer(name: $name) {
            name
            clan {
                name
            }
        }
    }
    """
)

@app.route("/player")
def route_player():
    """
    Show a single player.
    """

    name = request.args.get('name', default=None, type=str)

    player = dict(graphql.execute(
        _GQL_GET_PLAYER,
        variable_values = {
            'name': name,
        }
    ))['getPlayer']

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

_GQL_GET_CLAN_PLAYERS_COUNT = gql(
    """
    query ($name: String!) {
        getClan(name: $name) {
            playersCount
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

    players_count = dict(graphql.execute(
        _GQL_GET_CLAN_PLAYERS_COUNT,
        variable_values = {
            'name': name
        }
    ))['getClan'].get('playersCount', 0)

    paginator = frontend.components.paginator.init(players_count)

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

    for i, player in enumerate(clan['players']):
        player['rank'] = paginator['offset'] + i + 1
        player['clan'] = { 'name': name }

    return render_template(
        'clan.html',
        top_tabs = frontend.components.top_tabs.init({
            'type': 'custom',
            'links': [{
                'name': 'Player'
            }, {
                'name': name
            }]
        }),
        paginator = paginator,
        clan = clan,
        players = clan['players'],
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

    section_tabs = frontend.components.section_tabs.init('servers')
    paginator = frontend.components.paginator.init(section_tabs['servers']['count'])

    servers = dict(graphql.execute(
        _GQL_QUERY_SERVERS,
        variable_values = {
            'offset': paginator['offset'],
            'first': paginator['first']
        }
    ))['queryGameServer']

    for i, server in enumerate(servers):
        server['rank'] = paginator['offset'] + i + 1

    return render_template(
        'servers.html',
        top_tabs = frontend.components.top_tabs.init({
            'type': 'gametype',
            'gametype': game_type,
            'map': map_name
        }),

        section_tabs = section_tabs,
        paginator = paginator,

        game_type = game_type,
        map_name = map_name,
        servers = servers,
    )


_GQL_GET_SERVER = gql(
    """
    query ($address: String!, $clientsOrder: ClientOrder!) {
        getGameServer(address: $address) {
            address

            name
            map
            gameType

            numPlayers
            maxPlayers
            numClients
            maxClients

            clients(order: $clientsOrder) {
                player {
                    name
                }
                clan {
                    name
                }

                score
                ingame
            }
        }
    }
    """
)

@app.route('/server')
def route_server():
    """
    Show a single server.
    """

    address = request.args.get('address', default="", type=str)

    server = dict(graphql.execute(
        _GQL_GET_SERVER,
        variable_values = {
            'address': address,
            'clientsOrder': {'desc': 'score'}
        }
    ))['getGameServer']

    if not server:
        abort(404)

    return render_template(
        'server.html',
        top_tabs = frontend.components.top_tabs.init({
            'type': 'custom',
            'links': [{
                'name': 'Server'
            }, {
                'name': address
            }]
        }),
        server = server,
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
        top_tabs = frontend.components.top_tabs.init({
            'type': '...'
        }),
        game_type = None,
        map_name = None,
        game_types = ['CTF'],
    )


@app.route('/about')
def route_about():
    """
    Render the about page.
    """

    return render_template(
        'about.html',
        top_tabs = frontend.components.top_tabs.init({
            'type': 'about'
        })
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
        top_tabs = frontend.components.top_tabs.init({
            'type': 'custom',
            'links': [{
                'name': 'Status'
            }]
        }),
        master_servers = master_servers
    )


MAX_SEARCH_RESULTS = 100

_GQL_SEARCH = gql(
    """
    query counts($regexp: String!) {
        aggregatePlayer(filter: { name: { regexp: $regexp }}) {
            count
        }
        aggregateClan(filter: { name: { regexp: $regexp }}) {
            count
        }
        aggregateGameServer(filter: { name: { regexp: $regexp }}) {
            count
        }
    }

    query searchPlayers(
        $regexp: String!,
        $first: Int!
    ) {
        queryPlayer(
            filter: { name: { regexp: $regexp }},
            first: $first
        ) {
            name
            clan {
                name
            }
        }
    }

    query searchClans(
        $regexp: String!, $
        first: Int!
    ) {
        queryClan(
            filter: { name: { regexp: $regexp }},
            first: $first,
            order: { desc: playersCount }
        ) {
            name
            playersCount
        }
    }

    query searchServers(
        $regexp: String!,
        $first: Int!
    ) {
        queryGameServer(
            filter: { name: { regexp: $regexp }},
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

def search(template_name, section_tabs_active, operation_name, query_name):
    """
    Search for the given category.
    """

    query = request.args.get('q', default='', type = str)

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

        query_escaped = re.escape(query)

        counts_results = dict(graphql.execute(
            _GQL_SEARCH,
            operation_name = 'counts',
            variable_values = {
                'regexp': '/.*' + query_escaped + '.*/i'
            }
        ))

        counts = {
            'players': counts_results['aggregatePlayer']['count'],
            'clans': counts_results['aggregateClan']['count'],
            'servers': counts_results['aggregateGameServer']['count']
        }

        # Build the various regexp that are going to be given to dgraph to perform
        # the actual search.

        regexps = (
            '/^' + query_escaped + '$/i',
            '/^' + query_escaped + '.+/i',
            '/.+' + query_escaped + '$/i',
            '/.+' + query_escaped + '.+/i'
        )

        results = []

        for regexp in regexps:
            if len(results) < MAX_SEARCH_RESULTS:
                results.extend(dict(graphql.execute(
                    _GQL_SEARCH,
                    operation_name = operation_name,
                    variable_values = {
                        'regexp': regexp,
                        'first': MAX_SEARCH_RESULTS - len(results)
                    }
                ))[query_name])

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

    section_tabs = frontend.components.section_tabs.init(
        section_tabs_active, counts, urls
    )

    return render_template(
        template_name,

        top_tabs = frontend.components.top_tabs.init({
            'type': 'custom',
            'links': [{
                'name': 'Search'
            }, {
                'name': query
            }]
        }),

        section_tabs = section_tabs,
        query = query,
        results = results,
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
