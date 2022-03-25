"""
Test /gametypes.
"""

def test_route_gametypes(client, gametype):
    """
    Test /gametypes.
    """

    response = client.get('/gametypes')

    assert response.status_code == 200
    assert gametype['name'].encode() in response.data


def test_route_gametypes_no_gametypes(client):
    """
    Test /gametypes when there is no gametypes.
    """

    response = client.get('/gametypes')

    assert response.status_code == 200
    assert b'No gametypes found.' in response.data
