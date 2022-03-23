"""
Test /clans.
"""

import backend.database.clan

def test_route_clans(client):
    """
    Test /clans.
    """

    clan = {
        'name': 'test-clan'
    }

    backend.database.clan.upsert([clan])

    response = client.get('/clans')

    assert response.status_code == 200
    assert clan['name'].encode() in response.data


def test_route_clans_no_clans(client):
    """
    Test /clans when there is no clans.
    """

    response = client.get('/clans')

    assert response.status_code == 200
    assert b'No clans found.' in response.data
