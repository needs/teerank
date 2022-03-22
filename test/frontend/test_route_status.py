"""
Test /status.
"""

import backend.database.master_server

def test_route_status(client):
    """
    Test /status.
    """
    address = 'foo:8080'
    backend.database.master_server.create(address)
    response = client.get('/status')

    assert response.status_code == 200
    assert address.encode() in response.data
