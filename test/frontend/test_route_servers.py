"""
Test /servers.
"""

import backend.database.game_server
import backend.database.map

def test_route_servers(client, map_, gametype):
    """
    Test /servers.
    """

    game_server = {
        'address': 'foo:8000',
        'name': 'test-server',

        'map': backend.database.map.ref(map_['id']),

        'numPlayers': 1,
        'maxPlayers': 16,
        'numClients': 1,
        'maxClients': 16,
    }

    backend.database.game_server.upsert(game_server)

    response = client.get('/servers')

    assert response.status_code == 200
    assert game_server['name'].encode() in response.data
    assert map_['name'].encode() in response.data
    assert gametype['name'].encode() in response.data


def test_route_servers_no_servers(client):
    """
    Test /servers when there is no servers.
    """

    response = client.get('/servers')

    assert response.status_code == 200
    assert b'No servers found.' in response.data
