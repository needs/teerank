"""
Test /server.
"""

import backend.database.player
import backend.database.game_server
import backend.database.map

def test_route_server(client, map_):
    """
    Test /server.
    """

    player = {
        'name': 'player-name'
    }

    game_server = {
        'address': 'foo:8000',
        'name': 'test-server',

        'map': backend.database.map.ref(map_['id']),

        'numPlayers': 1,
        'maxPlayers': 16,
        'numClients': 1,
        'maxClients': 16,

        'clients': [{
            'player': backend.database.player.ref(player['name']),
            'score': 0,
            'ingame': True,
            'gameServer': backend.database.game_server.ref('foo:8000')
        }]
    }

    backend.database.player.upsert([player])
    backend.database.game_server.upsert(game_server)

    response = client.get(f'/server?address={game_server["address"]}')

    assert response.status_code == 200

    assert game_server['address'].encode() in response.data
    assert game_server['name'].encode() in response.data
    assert player['name'].encode() in response.data


def test_route_server_unexisting(client):
    """
    Test /server when the requested server does not exist.
    """

    assert client.get('/server?address=none').status_code == 404
