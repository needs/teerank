"""
Implement /server.
"""

from gql import gql
from flask import request, render_template, abort
from frontend.database import graphql

import frontend.components.top_tabs
from frontend.routes import blueprint

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

@blueprint.route('/server', endpoint='server')
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
