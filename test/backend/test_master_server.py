"""
Test master_server.py
"""

import pytest
from gql import gql
from backend.database import graphql

from backend.database.master_server import create, all_addresses, up, down, DEFAULT_ADDRESSES

_GQL_GET = gql(
    """
    query($address: String!) {
        getMasterServer(address: $address) {
            address,
            downSince,
            gameServers {
                address
            }
        }
    }
    """
)

def get(address: str) -> dict:
    """
    Get the master server (if any) with the specified address.
    """

    return dict(graphql().execute(
        _GQL_GET,
        variable_values = {
            'address': address
        }
    ))['getMasterServer']


@pytest.fixture(name='address')
def fixture_address(_graphql):
    """
    Create a dummy master server and return its address.
    """
    address = 'foo:8300'
    create(address)
    return address


def test_create(address):
    """
    Test master server creation.
    """

    assert get(address) is not None


def test_all_addresses_empty(_graphql):
    """
    Test behavior of all_addresses with an empty database.
    """

    for address in DEFAULT_ADDRESSES:
        assert get(address) is None

    all_addresses()

    for address in DEFAULT_ADDRESSES:
        assert get(address) is not None


def test_all_addresses_not_empty(address):
    """
    Test behavior of all_addresses when some master servers are already in the
    database.
    """

    for default_address in DEFAULT_ADDRESSES:
        assert get(default_address) is None

    all_addresses()

    assert get(address) is not None

    for default_address in DEFAULT_ADDRESSES:
        assert get(default_address) is None


def test_up(address):
    """
    Test the behavior of up().
    """

    up(address, {'bar:8300'})

    master_server = get(address)

    assert master_server['downSince'] is None
    assert master_server['gameServers'] == [{'address': 'bar:8300'}]


def test_down(address):
    """
    Test the behavior of down().
    """

    down(address)

    master_server = get(address)

    assert master_server['downSince'] is not None
    assert master_server['gameServers'] == []


def test_up_down(address):
    """
    Test the behavior of up() followed by down().
    """

    up(address, {'bar:8300'})
    down(address)

    master_server = get(address)

    assert master_server['downSince'] is not None
    assert master_server['gameServers'] == []


def test_down_up(address):
    """
    Test the behavior of down() followed by up().
    """

    down(address)
    up(address, {'bar:8300'})

    master_server = get(address)

    assert master_server['downSince'] is None
    assert master_server['gameServers'] == [{'address': 'bar:8300'}]


def test_down_down(address):
    """
    Test the behavior of down() followed by down().
    """

    down(address)
    down(address)

    master_server = get(address)

    assert master_server['downSince'] is not None
    assert master_server['gameServers'] == []


def test_up_up(address):
    """
    Test the behavior of up() followed by up().
    """

    up(address, {'bar:8300'})
    up(address, {'qux:8300'})

    master_server = get(address)

    assert master_server['downSince'] is None
    assert master_server['gameServers'] == [{'address': 'qux:8300'}]
