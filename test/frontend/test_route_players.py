"""
Test /players.
"""

import backend.database.player

def test_route_players(client):
    """
    Test /players.
    """

    player = {
        'name': 'player-name'
    }

    backend.database.player.upsert([player])

    response = client.get('/players')

    assert response.status_code == 200
    assert player['name'].encode() in response.data


def test_route_players_no_players(client):
    """
    Test /players when there is no players.
    """

    response = client.get('/players')

    assert response.status_code == 200
    assert b'No players found.' in response.data
