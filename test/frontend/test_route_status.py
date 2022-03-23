"""
Test /status.
"""

import pytest
import backend.database.master_server


@pytest.fixture(name='master_server')
def fixture_master_server():
    """
    Create a new master server and return its address.
    """

    address = 'foo:8080'
    backend.database.master_server.create(address)
    return address


def test_route_status_never_up(client, master_server):
    """
    Test /status for a master server that was never up.
    """

    response = client.get('/status')

    assert response.status_code == 200
    assert master_server.encode() in response.data
    assert b'Down' in response.data


def test_route_status_down(client, master_server):
    """
    Test /status for a master server that is down after being up.
    """

    backend.database.master_server.up(master_server, { 'bar:8080' })
    backend.database.master_server.down(master_server)

    response = client.get('/status')

    assert response.status_code == 200
    assert master_server.encode() in response.data
    assert b'Down' in response.data


def test_route_status_up(client, master_server):
    """
    Test /status for a master server that is up.
    """

    backend.database.master_server.up(master_server, { 'bar:8080' })

    response = client.get('/status')

    assert response.status_code == 200
    assert master_server.encode() in response.data
    assert b'OK' in response.data
    assert b'1 servers' in response.data
