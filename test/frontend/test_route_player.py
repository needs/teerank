"""
Test /player.
"""

import backend.database.player

def test_route_player(client):
    """
    Test /player.
    """

    player = {
        'name': 'player-name'
    }

    backend.database.player.upsert([player])

    response = client.get(f'/player?name={player["name"]}')

    assert response.status_code == 200
    assert player['name'].encode() in response.data


def test_route_player_unexisting(client):
    """
    Test /player when the requested player does not exist.
    """

    assert client.get('/player?name=none').status_code == 404
