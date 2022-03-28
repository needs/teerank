"""
Implement /clan.
"""

from flask import request, render_template, abort

from shared.database.graphql import graphql
import frontend.components.paginator
import frontend.components.top_tabs

def route_clan():
    """
    Show a single clan.
    """

    name = request.args.get('name', default=None, type=str)

    first, offset = frontend.components.paginator.info()

    # Couting players directly instead of relying upon clan->playersCount make
    # this query work even when clan->playersCount is not set.

    clan = graphql("""
        query($name: String!, $first: Int!, $offset: Int!) {
            getClan(name: $name) {
                name

                playersAggregate {
                    count
                }

                players(first: $first, offset: $offset) {
                    name
                }
            }
        }
        """, {
            'name': name,
            'first': first,
            'offset': offset
        }
    )['getClan']

    if not clan:
        abort(404)

    paginator = frontend.components.paginator.init(clan['playersAggregate']['count'])

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
