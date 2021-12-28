import logging
import secrets

from server import Server
from packet import Packet
from player import Player

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
        self.data_new = {}
        server_type = self.data.get('server_type', None)

        # Generate a new request token.

        if server_type == 'iext':
            self._request_token = secrets.token_bytes(3)
        else:
            self._request_token = secrets.token_bytes(1) + bytes(2)

        # Pick a packet according to server type or send both packets if server
        # type is unknown.

        packets = []

        if server_type is None or server_type == 'vanilla':
            packets.append(self._request_info_packet(b'gie3'))
        if server_type is None or server_type != 'vanilla':
            packets.append(self._request_info_packet(b'fstd'))

        return packets


    def stop_polling(self) -> bool:
        if self.data_new:
            self.data = self.data_new

            logging.info(f'Updated server {self.address}: {self.data["game_type"]} ({self.data["num_clients"]}/{self.data["max_clients"]})')
            self.save()
            return True
        else:
            return False


    def process_packet(self, packet: Packet) -> None:
        """
        Process packet header and route the packet to its final process
        function.
        """

        packet.unpack_bytes(10) # Padding
        packet_type = packet.unpack_bytes(4)

        token = int(packet.unpack()).to_bytes(3, byteorder='big')
        token = bytes([token[2], token[0], token[1]])

        if token != self._request_token:
            logging.debug(f'Dropping packet: wrong request token. ({packet_type}) {token} vs {self._request_token}')

        elif packet_type == b'inf3':
            self._process_packet_vanilla(packet)
        elif packet_type == b'dtsf':
            self._process_packet_legacy_64(packet)
        elif packet_type == b'iext':
            self._process_packet_extended(packet)
        elif packet_type == b'iex+':
            self._process_packet_extended_more(packet)

        else:
            logging.debug('Dropping packet: packet type not supported.')


    def _process_packet_vanilla(self, packet: Packet):
        """Parse the default response of the vanilla client."""

        # If a non-vanilla packet was already processed, drop this packet.
        if self.data_new:
            return

        self.data_new = {
            'server_type': 'vanilla',
            'version': packet.unpack(),
            'name': packet.unpack(),
            'map_name': packet.unpack(),
            'game_type': packet.unpack(),
            'flags': int(packet.unpack()),
            'num_players': int(packet.unpack()),
            'max_players': int(packet.unpack()),
            'num_clients': int(packet.unpack()),
            'max_clients': int(packet.unpack()),
            'players': []
        }

        while packet.unpack_remaining() >= 5:
            self.data_new['players'].append({
                'name': packet.unpack(),
                'clan': packet.unpack(),
                'country': int(packet.unpack()),
                'score': int(packet.unpack()),
                'ingame': bool(int(packet.unpack()))
            })


    def _process_packet_legacy_64(self, packet: Packet) -> None:
        """Parse legacy 64 packet."""

        self.data_new = {
            'server_type': 'legacy_64',
            'version': packet.unpack(),
            'name': packet.unpack(),
            'map_name': packet.unpack(),
            'game_type': packet.unpack(),
            'flags': int(packet.unpack()),
            'num_players': int(packet.unpack()),
            'max_players': int(packet.unpack()),
            'num_clients': int(packet.unpack()),
            'max_clients': int(packet.unpack()),
            'players': []
        }

        packet.unpack() # Offset

        while packet.unpack_remaining() >= 5:
            self.data_new['players'].append({
                'name': packet.unpack(),
                'clan': packet.unpack(),
                'country': int(packet.unpack()),
                'score': int(packet.unpack()),
                'ingame': bool(int(packet.unpack()))
            })


    def _process_packet_extended(self, packet: Packet) -> None:
        """Parse the extended server info packet."""

        logging.debug('Dropping packet: packet type "extended" not supported.')
        return

        self.server_type = 'ext'
        self.version = packet.unpack()
        self.name = packet.unpack()
        self.map_name = packet.unpack()
        self.map_crc = int(packet.unpack())
        self.map_size = int(packet.unpack())
        self.game_type = packet.unpack()
        self.flags = int(packet.unpack())
        self.num_players = int(packet.unpack())
        self.max_players = int(packet.unpack())
        self.num_clients = int(packet.unpack())
        self.max_clients = int(packet.unpack())

        while packet.unpack_remaining() >= 6:
            packet.unpack() # Reserved
            self.append_player(Player(
                name=packet.unpack(),
                clan=packet.unpack(),
                country=int(packet.unpack()),
                score=int(packet.unpack()),
                ingame=bool(int(packet.unpack()))
            ))


    def _process_packet_extended_more(self, packet: Packet) -> None:
        """Parse the extended server info packet."""

        logging.debug('Dropping packet: packet type "extended more" not supported.')
        return

        packet.unpack()

        self.server_type = 'ext+'

        while packet.unpack_remaining() >= 6:
            packet.unpack() # Reserved
            self.append_player(Player(
                name=packet.unpack(),
                clan=packet.unpack(),
                country=int(packet.unpack()),
                score=int(packet.unpack()),
                ingame=bool(int(packet.unpack()))
            ))


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
