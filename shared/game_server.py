"""
Implement GameServer, GameServerState and GameServerType classes.
"""

import json
from enum import IntEnum
from gql import gql

from shared.database import redis, key_to_address, graphql
from shared.server import Server


class GameServerType(IntEnum):
    """
    Game server type.  Higher is better.
    """

    UNKNOWN = -1
    VANILLA = 0
    LEGACY_64 = 1
    EXTENDED = 2


class GameServerState:
    """
    Internal game server state.
    """

    def __init__(self, server_type: GameServerType):
        """
        Initialize a server state with the given server type.
        """

        self.server_type = server_type
        self.info = {}
        self.clients = {}


    def merge(self, state: 'GameServerState') -> None:
        """
        Merge two states into a single one.

        The server state with the highest server type overide the other.  If
        server types are equal, do a classic merge.
        """

        if self.server_type <= state.server_type:
            self.server_type = state.server_type
            if state.info:
                self.info = state.info

            if state.server_type == self.server_type:
                self.clients |= state.clients
            else:
                self.clients = state.clients


    def is_complete(self) -> bool:
        """
        Return true if server state is valid and have all its data.
        """

        return self.info and len(self.clients) == self.info['num_clients']


    @staticmethod
    def to_bytes(state: 'GameServerState') -> bytes:
        """
        Convert the given server state into JSON and then into bytes.
        """

        data = state.info
        data['server_type'] = state.server_type
        data['clients'] = state.clients

        return json.dumps(data).encode()

    @staticmethod
    def from_bytes(data: bytes) -> 'GameServerState':
        """
        Create a new server state from its byte data decoded as JSON.
        """

        if not data:
            return GameServerState(GameServerType.UNKNOWN)

        data = json.loads(data.decode())

        state = GameServerState(data.pop('server_type'))
        state.clients = data.pop('clients')
        state.info = data

        return state


_GQL_UPDATE_PLAYERS = gql(
    """
    mutation ($input: [AddPlayerInput!]!) {
        addPlayer(input: $input, upsert: true) {
            player {
                name
            }
        }
    }
    """
)

class GameServer(Server):
    """
    Teeworld game server.
    """

    def __init__(self, key: str):
        """
        Initialize game server with the given key.
        """

        # GameServer host is already an IP.

        super().__init__(*key_to_address(key))

        self.key = key
        self.state = GameServerState.from_bytes(redis.get(f'game-servers:{key}'))


    def save(self) -> None:
        """
        Save game server data.
        """

        # Save players

        if self.state.clients:
            variables = {
                'input': []
            }

            for name in self.state.clients.keys():
                variables['input'].append({
                    'name': name
                })

            graphql.execute(_GQL_UPDATE_PLAYERS, variable_values = variables)


    @staticmethod
    def keys() -> list[str]:
        """
        Load all game servers keys from the database.
        """

        keys = redis.smembers('game-servers')
        return [key.decode() for key in keys]
