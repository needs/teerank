"""
Implement /clan.
"""

from gql import gql
from flask import request, render_template, abort
from frontend.database import graphql

import frontend.components.paginator
import frontend.components.top_tabs
from frontend.routes import blueprint

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

@blueprint.route('/clan', endpoint='clan')
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
                'name': 'Clan'
            }, {
                'name': name
            }]
        }),
        paginator = paginator,
        clan = clan,
        players = clan['players'],
    )
