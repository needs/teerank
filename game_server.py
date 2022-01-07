import logging
import secrets

from server import Server
from packet import Packet, PacketException
from rank import rank

class GameServer(Server):
    def __init__(self, ip: str, port: int):
        super().__init__(ip, port)
        self.redis_key = f'game-servers:{self.key}'
        self.data = {}


    def load(self) -> None:
        # self.data = json.loads(redis.get(self.redis_key))
        self.data = {}


    def save(self) -> None:
        # redis.set(self.redis_key, json.dumps(self.data))
        pass


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

        server_type = self.data.get('server_type', None)

        # Generate a new request token.

        if server_type == 'extended':
            self._request_token = secrets.token_bytes(3)
        else:
            self._request_token = secrets.token_bytes(1) + bytes(2)

        # Pick a packet according to server type or send both packets if server
        # type is unknown.
        #
        # The packet is the same for both vanilla and extended servers, thanks
        # to a clever trick with the token field.

        packets = []

        if server_type is None or server_type == 'vanilla' or server_type == 'extended':
            packets.append(self._request_info_packet(b'gie3'))
        if server_type is None or server_type == 'legacy_64':
            packets.append(self._request_info_packet(b'fstd'))

        # Reset containers for the data to be received.

        self._data_new_vanilla = {}
        self._data_new_legacy_64 = {}
        self._data_new_legacy_64_clients = {}
        self._data_new_extended = {}
        self._data_new_extended_clients = {}

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

        if self._data_new_extended and self._data_new_extended['num_clients'] == len(self._data_new_extended_clients):
            data_new = self._data_new_extended
            data_new['clients'] = self._data_new_extended_clients
        elif self._data_new_legacy_64 and self._data_new_legacy_64['num_clients'] == len(self._data_new_legacy_64_clients):
            data_new = self._data_new_legacy_64
            data_new['clients'] = self._data_new_legacy_64_clients
        elif self._data_new_vanilla:
            data_new = self._data_new_vanilla
        else:
            return False

        # Delete received data because only self.data will be used now.

        del self._data_new_vanilla
        del self._data_new_legacy_64
        del self._data_new_legacy_64_clients
        del self._data_new_extended
        del self._data_new_extended_clients

        # Rank players before erasing server data.

        rank(self.data, data_new)

        # Now that the server is fully updated, save its state.

        self.data = data_new
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

        elif packet_type == b'inf3':
            self._process_packet_vanilla(packet)
        elif packet_type == b'dtsf':
            self._process_packet_legacy_64(packet)
        elif packet_type == b'iext':
            self._process_packet_extended(packet)
        elif packet_type == b'iex+':
            self._process_packet_extended_more(packet)

        else:
            raise PacketException('Packet type not supported.')


    def _process_packet_vanilla(self, packet: Packet):
        """Parse the default response of the vanilla client."""

        data = {
            'server_type': 'vanilla',
            'version': packet.unpack(),
            'name': packet.unpack(),
            'map_name': packet.unpack(),
            'game_type': packet.unpack(),
            'flags': packet.unpack_int(),
            'num_players': packet.unpack_int(),
            'max_players': packet.unpack_int(),
            'num_clients': packet.unpack_int(),
            'max_clients': packet.unpack_int(),
            'clients': {}
        }

        while packet.unpack_remaining() >= 5:
            name = packet.unpack()
            data['clients'][name] = {
                'clan': packet.unpack(),
                'country': packet.unpack_int(),
                'score': packet.unpack_int(),
                'ingame': bool(packet.unpack_int())
            }

        self._data_new_vanilla = data


    def _process_packet_legacy_64(self, packet: Packet) -> None:
        """Parse legacy 64 packet."""

        data = {
            'server_type': 'legacy_64',
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

        clients = {}

        while packet.unpack_remaining() >= 5:
            name = packet.unpack()
            clients[name] = {
                'clan': packet.unpack(),
                'country': packet.unpack_int(),
                'score': packet.unpack_int(),
                'ingame': bool(packet.unpack_int())
            }

        self._data_new_legacy_64_clients |= clients
        self._data_new_legacy_64 = data


    def _process_packet_extended(self, packet: Packet) -> None:
        """Parse the extended server info packet."""

        data = {
            'server_type': 'extended',
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

        clients = {}

        while packet.unpack_remaining() >= 6:
            name = packet.unpack()
            clients[name] = {
                'clan': packet.unpack(),
                'country': packet.unpack_int(-1),
                'score': packet.unpack_int(),
                'ingame': bool(packet.unpack_int())
            }
            packet.unpack() # Reserved

        self._data_new_extended_clients |= clients
        self._data_new_extended = data


    def _process_packet_extended_more(self, packet: Packet) -> None:
        """Parse the extended server info packet."""

        packet.unpack_int() # Packet number
        packet.unpack() # Reserved

        clients = {}

        while packet.unpack_remaining() >= 6:
            name = packet.unpack()
            clients[name] = {
                'clan': packet.unpack(),
                'country': packet.unpack_int(-1),
                'score': packet.unpack_int(),
                'ingame': bool(packet.unpack_int())
            }
            packet.unpack() # Reserved

        self._data_new_extended_clients |= clients


def load_game_servers() -> list[GameServer]:
    """Load all game servers stored in the database."""

    # addresses = redis.smembers('game-servers')
    addresses = []
    game_servers = []

    for address in addresses:
        game_server = GameServer(address)
        game_server.load()
        game_servers.append(game_server)

    return game_servers
