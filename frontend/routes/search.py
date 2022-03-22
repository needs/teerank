"""
Implement /player.
"""

import re

from gql import gql
from flask import request, render_template, url_for
from frontend.database import graphql

import frontend.components.paginator
import frontend.components.section_tabs
import frontend.components.top_tabs
from frontend.routes import blueprint

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
        'players': url_for('routes.players-search', q = query),
        'clans': url_for('routes.clans-search', q = query),
        'servers': url_for('routes.servers-search', q = query)
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


@blueprint.route('/players/search', endpoint='players-search')
def route_players_search():
    """
    List of players matching the search query.
    """

    return search('search_players.html', 'players', 'searchPlayers', 'queryPlayer')


@blueprint.route('/clans/search', endpoint='clans-search')
def route_clans_search():
    """
    List of clans matching the search query.
    """

    return search('search_clans.html', 'clans', 'searchClans', 'queryClan')


@blueprint.route('/servers/search', endpoint='servers-search')
def route_servers_search():
    """
    List of servers matching the search query.
    """

    return search('search_servers.html', 'servers', 'searchServers', 'queryGameServer')
