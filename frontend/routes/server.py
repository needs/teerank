"""
Implement /server.
"""

from flask import request, render_template, abort
from shared.database.graphql import graphql

import frontend.components.top_tabs

def route_server():
    """
    Show a single server.
    """

    address = request.args.get('address', default="", type=str)

    server = graphql("""
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
        """, {
            'address': address,
            'clientsOrder': {'desc': 'score'}
        }
    )['getGameServer']

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
