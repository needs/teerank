"""
Test /maps.
"""

def test_route_maps(client, gametype, map_):
    """
    Test /maps.
    """

    response = client.get(f'/maps?gametype={gametype["name"]}')

    assert response.status_code == 200
    assert map_['name'].encode() in response.data


def test_route_maps_no_maps(client, gametype):
    """
    Test /maps when there is no maps for the given gametype.
    """

    response = client.get(f'/maps?gametype={gametype["name"]}')

    assert response.status_code == 200
    assert b'No maps found.' in response.data


def test_route_maps_404(client):
    """
    Test /maps when gametype does not exist.
    """

    response = client.get('/maps?gametype=foo')

    assert response.status_code == 404
