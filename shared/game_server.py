"""
Helpers for game server.
"""

from gql import gql
from shared.database import graphql


_GQL_QUERY_SERVERS = gql(
    """
    query {
        queryGameServer {
            address
        }
    }
    """
)

def all_addresses() -> list[str]:
    """
    Get the list of all game servers addresses.
    """

    servers = graphql.execute(
        _GQL_QUERY_SERVERS
    )['queryGameServer']

    return [server['address'] for server in servers]


_GQL_GET_SERVER = gql(
    """
    query ($address: String!, $clientsOrder: ClientOrder!) {
        getGameServer(address: $address) {
            address

            type
            version
            name
            map
            gameType

            numPlayers
            maxPlayers
            numClients
            maxClients

            clients(order: $clientsOrder) {
                id
                player {
                    name
                }
                clan {
                    name
                }

                country
                score
                ingame
            }
        }
    }
    """
)

def get(address: str, clients_order={'desc': 'score'}) -> dict:
    """
    Get the game server with the given address.
    """

    try:
        server = graphql.execute(
            _GQL_GET_SERVER,
            variable_values = {
                'address': address,
                'clientsOrder': clients_order
            }
        )['getGameServer']
    except:
        return {}

    return server if server else {}


_GQL_SET_SERVER = gql(
    """
    query queryClients($address: String!) {
        getGameServer(address: $address) {
            clients {
                id
            }
        }
    }

    mutation replaceClients($clientsToAdd: [AddClientInput!]!, $clientsToRemove: [ID!]) {
        addClient(input: $clientsToAdd) {
            client {
                id
            }
        }
        deleteClient(filter: {id: $clientsToRemove}) {
            numUids
        }
    }

    mutation upsertGameServer($server: AddGameServerInput!) {
        addGameServer(input: [$server], upsert: true) {
            numUids
        }
    }
    """
)

def set(server: dict) -> None:
    """
    Update the given server.  Replace its clients list.
    """

    # An upsert mutation adds clients to the existing clients list.  Therefor we
    # need to remove old clients, and for that we need to get their IDs.

    clients_to_remove = graphql.execute(
        _GQL_SET_SERVER,
        operation_name = 'queryClients',
        variable_values = {
            'address': server['address']
        }
    )['getGameServer']

    if not clients_to_remove:
        clients_to_remove = []
    else:
        clients_to_remove = [client['id'] for client in clients_to_remove['clients']]

    server = server.copy()

    server['clients'] = graphql.execute(
        _GQL_SET_SERVER,
        operation_name = 'replaceClients',
        variable_values = {
            'clientsToRemove': clients_to_remove,
            'clientsToAdd': server['clients']
        }
    )['addClient']['client']

    graphql.execute(
        _GQL_SET_SERVER,
        operation_name = 'upsertGameServer',
        variable_values = {
            'server': server
        }
    )
