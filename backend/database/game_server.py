"""
Helpers for game server.
"""

from shared.database.graphql import graphql


def ref(address):
    """Create a game server reference from the given address."""
    return { 'address': address } if address else None


def all_addresses() -> list[str]:
    """
    Get the list of all game servers addresses.
    """

    servers = graphql("""
        query {
            queryGameServer {
                address
            }
        }
        """
    )['queryGameServer']

    return [server['address'] for server in servers]


def get(address: str, clients_order=None) -> dict:
    """
    Get the game server with the given address.
    """

    if clients_order is None:
        clients_order = {'desc': 'score'}

    server = graphql("""
        query ($address: String!, $clientsOrder: ClientOrder!) {
            getGameServer(address: $address) {
                address

                type
                version
                name

                map {
                    name
                    gameType {
                        name
                    }
                }

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
        """, {
            'address': address,
            'clientsOrder': clients_order
        }
    )['getGameServer']

    return server if server else {}


def upsert(server: dict) -> None:
    """
    Update the given server.  Replace its clients list.
    """

    # An upsert mutation adds clients to the existing clients list.  Therefor we
    # need to remove old clients, and for that we need to get their IDs.

    clients_to_remove = graphql("""
        query($address: String!) {
            getGameServer(address: $address) {
                clients {
                    id
                }
            }
        }
        """, {
            'address': server['address']
        }
    )['getGameServer']

    if not clients_to_remove:
        clients_to_remove = []
    else:
        clients_to_remove = [client['id'] for client in clients_to_remove['clients']]

    graphql("""
        mutation($server: AddGameServerInput!, $clientsToRemove: [ID!]) {
            addGameServer(input: [$server], upsert: true) {
                numUids
            }
            deleteClient(filter: {id: $clientsToRemove}) {
                numUids
            }
        }
        """, {
            'server': server,
            'clientsToRemove': clients_to_remove
        }
    )
