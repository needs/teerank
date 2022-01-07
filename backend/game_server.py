"""
Implement GameServer, GameServerState and GameServerType classes.
"""

import secrets
import json
from enum import IntEnum

from server import Server
from packet import Packet, PacketException
from rank import rank

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
    def to_json(state: 'GameServerState') -> str:
        """
        Convert the givne server state into JSON.
        """

        data = state.info
        data['server_type'] = state.server_type
        data['clients'] = state.clients
        return json.dumps(state)

    @staticmethod
    def from_json(data: str) -> 'GameServerState':
        """
        Create a new server state from its JSON.
        """

        data = json.loads(data)

        state = GameServerState(data.pop('server_type'))
        state.clients = data.pop('clients')
        state.info = data

        return state


class GameServer(Server):
    """
    Teeworld game server.
    """

    def __init__(self, ip: str, port: int):
        """
        Initialize game server with the given IP and port.
        """

        super().__init__(ip, port)
        self.redis_key = f'game-servers:{self.key}'
        self.state = GameServerState(GameServerType.UNKNOWN)
        self._state_new = None
        self._request_token = None


    def load(self) -> None:
        """
        Load game server data.
        """
        # self.state = GameServerState.from_json(redis.get(self.redis_key))
        self.state = GameServerState(GameServerType.UNKNOWN)


    def save(self) -> None:
        """
        Save game server data.
        """
        # redis.set(self.redis_key, GameServerState.to_json(self.state))


    def _request_info_packet(self, request: bytes):
        """
        Create a 'request info' packet with the given request.
        """

        token = self._request_token[0:1]
        extra_token = self._request_token[1:3]

        packet = Packet()

        packet.pack_bytes(b'xe') # Magic header (2 bytes)
        packet.pack_bytes(extra_token) # Extra token (2 bytes)
        packet.pack_bytes(b'\x00\x00') # Reserved (2 bytes)
        packet.pack_bytes(b'\xff\xff\xff\xff') # Padding (4 bytes)
        packet.pack_bytes(request[0:4]) # Vanilla request (4 bytes)
        packet.pack_bytes(token) # Token (1 byte)

        return packet


    def start_polling(self) -> list[Packet]:
        """
        Setup internal state so that incoming data can be received.
        """

        # Generate a new request token.

        if self.state.server_type == GameServerType.EXTENDED:
            self._request_token = secrets.token_bytes(3)
        else:
            self._request_token = secrets.token_bytes(1) + bytes(2)

        # Pick a packet according to server type or send both packets if server
        # type is unknown.
        #
        # The packet is the same for both vanilla and extended servers, thanks
        # to a clever trick with the token field.

        packets = []

        if self.state.server_type in (GameServerType.UNKNOWN, \
                                      GameServerType.VANILLA, \
                                      GameServerType.EXTENDED):
            packets.append(self._request_info_packet(b'gie3'))
        if self.state.server_type in (GameServerType.UNKNOWN, \
                                      GameServerType.LEGACY_64):
            packets.append(self._request_info_packet(b'fstd'))

        # Reset _state_new.

        self._state_new = GameServerState(GameServerType.UNKNOWN)

        return packets


    def stop_polling(self) -> bool:
        """
        Check any received data and process them.
        """

        # Try to use extended data first because they are considered to be the
        # most superior format. Then use legacy 64 data because they provides
        # more information than the vanilla format.
        #
        # Check that data is complete by comparing the number of clients on the
        # server to the number of clients received.

        if not self._state_new.is_complete():
            return False

        # Rank players before overing server state.

        rank(self.state, self._state_new)

        # Now that the server is fully updated, save its state.

        self.state = self._state_new
        self._state_new = None
        self.save()

        return True


    def process_packet(self, packet: Packet) -> None:
        """
        Process packet header and route the packet to its final process
        function.
        """

        packet.unpack_bytes(10) # Padding
        packet_type = packet.unpack_bytes(4)

        # Servers send token back as an integer that is a combination of token
        # and extra_token fields of the packet we sent.  However the received
        # token has some its byte mixed because of endianess and we need to swap
        # some bytes to get the full token back.

        token = packet.unpack_int().to_bytes(3, byteorder='big')
        token = bytes([token[2], token[0], token[1]])

        if token != self._request_token:
            raise PacketException('Wrong request token.')

        if packet_type == b'inf3':
            state = self._process_packet_vanilla(packet)
        elif packet_type == b'dtsf':
            state = self._process_packet_legacy_64(packet)
        elif packet_type == b'iext':
            state = self._process_packet_extended(packet)
        elif packet_type == b'iex+':
            state = self._process_packet_extended_more(packet)

        else:
            raise PacketException('Packet type not supported.')

        self._state_new.merge(state)


    @staticmethod
    def _process_packet_vanilla(packet: Packet) -> GameServerState:
        """
        Parse the default response of the vanilla client.
        """

        state = GameServerState(GameServerType.VANILLA)

        state.info = {
            'version': packet.unpack(),
            'name': packet.unpack(),
            'map_name': packet.unpack(),
            'game_type': packet.unpack(),
            'flags': packet.unpack_int(),
            'num_players': packet.unpack_int(),
            'max_players': packet.unpack_int(),
            'num_clients': packet.unpack_int(),
            'max_clients': packet.unpack_int()
        }

        while packet.unpack_remaining() >= 5:
            name = packet.unpack()
            state.clients[name] = {
                'clan': packet.unpack(),
                'country': packet.unpack_int(),
                'score': packet.unpack_int(),
                'ingame': bool(packet.unpack_int())
            }

        return state


    @staticmethod
    def _process_packet_legacy_64(packet: Packet) -> GameServerState:
        """
        Parse legacy 64 packet.
        """

        state = GameServerState(GameServerType.LEGACY_64)

        state.info = {
            'version': packet.unpack(),
            'name': packet.unpack(),
            'map_name': packet.unpack(),
            'game_type': packet.unpack(),
            'flags': packet.unpack_int(),
            'num_players': packet.unpack_int(),
            'max_players': packet.unpack_int(),
            'num_clients': packet.unpack_int(),
            'max_clients': packet.unpack_int()
        }

        # Even though the offset is advertised as an integer, in real condition
        # we receive a single byte.

        packet.unpack_bytes(1) # Offset

        while packet.unpack_remaining() >= 5:
            name = packet.unpack()
            state.clients[name] = {
                'clan': packet.unpack(),
                'country': packet.unpack_int(),
                'score': packet.unpack_int(),
                'ingame': bool(packet.unpack_int())
            }

        return state


    @staticmethod
    def _process_packet_extended(packet: Packet) -> GameServerState:
        """
        Parse the extended server info packet.
        """

        state = GameServerState(GameServerType.EXTENDED)

        state.info = {
            'version': packet.unpack(),
            'name': packet.unpack(),
            'map_name': packet.unpack(),
            'map_crc': packet.unpack_int(),
            'map_size': packet.unpack_int(),
            'game_type': packet.unpack(),
            'flags': packet.unpack_int(),
            'num_players': packet.unpack_int(),
            'max_players': packet.unpack_int(),
            'num_clients': packet.unpack_int(),
            'max_clients': packet.unpack_int()
        }

        packet.unpack() # Reserved

        while packet.unpack_remaining() >= 6:
            name = packet.unpack()
            state.clients[name] = {
                'clan': packet.unpack(),
                'country': packet.unpack_int(-1),
                'score': packet.unpack_int(),
                'ingame': bool(packet.unpack_int())
            }
            packet.unpack() # Reserved

        return state


    @staticmethod
    def _process_packet_extended_more(packet: Packet) -> GameServerState:
        """
        Parse the extended server info packet.
        """

        state = GameServerState(GameServerType.EXTENDED)

        packet.unpack_int() # Packet number
        packet.unpack() # Reserved

        while packet.unpack_remaining() >= 6:
            name = packet.unpack()
            state.clients[name] = {
                'clan': packet.unpack(),
                'country': packet.unpack_int(-1),
                'score': packet.unpack_int(),
                'ingame': bool(packet.unpack_int())
            }
            packet.unpack() # Reserved

        return state


def load_game_servers() -> list[GameServer]:
    """
    Load all game servers stored in the database.
    """

    # addresses = redis.smembers('game-servers')
    addresses = []
    game_servers = []

    for address in addresses:
        game_server = GameServer(address)
        game_server.load()
        game_servers.append(game_server)

    return game_servers
