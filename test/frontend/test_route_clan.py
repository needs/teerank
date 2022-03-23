"""
Test /clan.
"""

import backend.database.player
import backend.database.clan

def test_route_clan(client):
    """
    Test /clan.
    """

    clan = {
        'name': 'test-clan'
    }

    player = {
        'name': 'player-name',
        'clan': backend.database.clan.ref(clan['name'])
    }

    backend.database.player.upsert([player])
    backend.database.clan.upsert([clan])

    response = client.get(f'/clan?name={clan["name"]}')

    assert response.status_code == 200
    assert clan['name'].encode() in response.data
    assert player['name'].encode() in response.data


def test_route_clan_unexisting(client):
    """
    Test /clan when the requested clan does not exist.
    """

    assert client.get('/clan?name=none').status_code == 404
