"""
Implement GameServer, GameServerState and GameServerType classes.
"""

import json
from enum import IntEnum
from gql import gql

from shared.database import redis, key_to_address, graphql
from shared.server import Server

import shared.clan
import shared.player


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

        if self.state.clients:
            #
            # Query players current clan.
            #
            # Clan maintains a count of players to be able to sort clan by their
            # number of players.  So before changing players clan, we need to
            # know in which clan the player was in to update their player count.
            #

            players_clan = shared.player.get_clan(list(self.state.clients.keys()))

            #
            # Create new clans and update players clan.
            #
            # Since player clan have a @hasInverse() pointing to the clan player
            # list, updating players clan automatically updates clan players list
            # as well.
            #

            unique_clans = set()

            for client in self.state.clients.values():
                if client['clan']:
                    unique_clans.add(client['clan'])

            shared.clan.upsert([{'name': clan} for clan in unique_clans])

            players = []

            for name, client in self.state.clients.items():
                clan_ref = { 'name': client['clan'] } if client['clan'] else None

                players.append({
                    'name': name,
                    'clan': clan_ref
                })

            shared.player.upsert(players)

            #
            # Query clans playersCount.
            #
            # Complete unique_clans with the clan queried before updating
            # players clan so that clans that losed a player also have their
            # count updated.
            #

            for clan in players_clan.values():
                if clan:
                    unique_clans.add(clan['name'])

            #
            # Set clans playersCount.
            #

            for clan, count in shared.clan.get_player_count(list(unique_clans)).items():
                shared.clan.set_player_count(clan, count)


    @staticmethod
    def keys() -> list[str]:
        """
        Load all game servers keys from the database.
        """

        keys = redis.smembers('game-servers')
        return [key.decode() for key in keys]
