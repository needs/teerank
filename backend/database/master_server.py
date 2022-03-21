"""
Helpers for master server.
"""

import datetime

from gql import gql
from backend.database import graphql
import backend.database.game_server


DEFAULT_ADDRESSES = [
    'master1.teeworlds.com:8300',
    'master2.teeworlds.com:8300',
    'master3.teeworlds.com:8300',
    'master4.teeworlds.com:8300'
]


_GQL_CREATE = gql(
    """
    mutation($masterServer: AddMasterServerInput!) {
        addMasterServer(input: [$masterServer]) {
            numUids
        }
    }
    """
)

def create(address: str) -> None:
    """
    Create a new master server with the given address.
    """

    graphql().execute(
        _GQL_CREATE,
        variable_values = {
            'masterServer': {
                'address': address,
                'downSince': datetime.datetime.min.isoformat()
            }
        }
    )


_GQL_ALL_ADDRESSES = gql(
    """
    query {
        queryMasterServer {
            address
        }
    }
    """
)

def all_addresses() -> list[str]:
    """
    Get the list of all master servers address.

    If no servers are found, return a default list of servers.
    """

    servers = dict(graphql().execute(
        _GQL_ALL_ADDRESSES
    ))['queryMasterServer']

    addresses = [server['address'] for server in servers]

    if not addresses:
        addresses = DEFAULT_ADDRESSES

        for address in addresses:
            create(address)

    return addresses


_GQL_UP = gql(
    """
    query get($address: String!) {
        getMasterServer(address: $address) {
            gameServers {
                address
            }
        }
    }

    mutation update($address: String!, $toRemove: [GameServerRef], $toAdd: [GameServerRef]) {
        updateMasterServer(input: {
            filter: {address: {eq: $address}}
            remove: {
                gameServers: $toRemove
                downSince: null
            }
            set: {
                gameServers: $toAdd
            }
        }) {
            numUids
        }
    }
    """
)

def up(address: str, game_servers_addresses: set[str]) -> None:
    """
    Update the given master server, replace all its linked game servers by the
    new ones.
    """

    master_server = dict(graphql().execute(
        _GQL_UP,
        operation_name = 'get',
        variable_values = {
            'address': address
        }
    ))['getMasterServer']

    # Only send the server to add and to remove to save on bandwidth (and also
    # because dgraph may not insert servers that are in by both the old and new
    # listings).

    old = { game_server['address'] for game_server in master_server['gameServers'] }
    new = game_servers_addresses

    to_remove = [ backend.database.game_server.ref(address) for address in old.difference(new) ]
    to_add = [ backend.database.game_server.ref(address) for address in new.difference(old) ]

    graphql().execute(
        _GQL_UP,
        operation_name = 'update',
        variable_values = {
            'address': address,
            'toAdd': to_add,
            'toRemove': to_remove
        }
    )

_GQL_DOWN = gql(
    """
    query get($address: String!) {
        getMasterServer(address: $address) {
            downSince
            gameServers {
                address
            }
        }
    }

    mutation update($address: String!, $toRemove: [GameServerRef], $downSince: DateTime!) {
        updateMasterServer(input: {
            filter: {address: {eq: $address}}
            remove: {gameServers: $toRemove}
            set: {downSince: $downSince}
        }) {
            numUids
        }
    }
    """
)

def down(address: str) -> None:
    """
    Mark the given game server as down.
    """

    master_server = dict(graphql().execute(
        _GQL_DOWN,
        operation_name = 'get',
        variable_values = {
            'address': address
        }
    ))['getMasterServer']

    to_remove = master_server['gameServers']
    down_since = master_server['downSince']

    if not down_since:
        down_since = datetime.datetime.now().isoformat()

    graphql().execute(
        _GQL_DOWN,
        operation_name = 'update',
        variable_values = {
            'address': address,
            'toRemove': to_remove,
            'downSince': down_since
        }
    )
