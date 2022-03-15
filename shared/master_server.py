"""
Helpers for master server.
"""

from gql import gql
from shared.database import graphql


_GQL_QUERY_MASTER_SERVERS = gql(
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

    servers = dict(graphql.execute(
        _GQL_QUERY_MASTER_SERVERS
    ))['queryMasterServer']

    servers = [server['address'] for server in servers]

    if not servers:
        servers = [
            'master1.teeworlds.com:8300',
            'master2.teeworlds.com:8300',
            'master3.teeworlds.com:8300',
            'master4.teeworlds.com:8300'
        ]

    return servers


def get_all() -> list[dict]:
    """
    Get the list of all master servers.
    """

    return dict(graphql.execute(
        _GQL_QUERY_MASTER_SERVERS
    ))['queryMasterServer']


_GQL_UPSERT_MASTER_SERVER = gql(
    """
    mutation ($masterServer: AddMasterServerInput!) {
        addMasterServer(input: [$masterServer], upsert: true) {
            numUids
        }
    }
    """
)

def upsert(master_server: dict) -> None:
    """
    Update the given master server.
    """

    graphql.execute(
        _GQL_UPSERT_MASTER_SERVER,
        variable_values = {
            'masterServer': master_server
        }
    )
