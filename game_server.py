import logging

from server import Server
from packet import Packet
from player import Player

class GameServer(Server):
    def __init__(self, ip: str, port: int):
        super().__init__(ip, port)
        self.redis_key = f'game-servers:{self.key}'
        self.data = None


    def load(self) -> None:
        # self.data = json.loads(redis.get(self.redis_key))
        self.data = {}


    def save(self) -> None:
        # redis.set(self.redis_key, json.dumps(self.data))
        pass


    def start_polling(self) -> list[Packet]:
        self.data_new = {}

        # Pick a packet according to server type or send both packets if server
        # type is unknown.

        server_type = self.data.get('server_type', None)
        packets = []

        if server_type is None or server_type == 'vanilla':
            packets.append(Packet(packet_type=b'\xff\xff\xff\xffgie3\x00'))
        if server_type is None or server_type != 'vanilla':
            packets.append(Packet(packet_type=b'\xff\xff\xff\xfffstd\x00'))

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
        packet.unpack() # Server token

        if packet.type == b'\xff\xff\xff\xffinf3':
            return self._process_packet_vanilla(packet)
        if packet.type == b'\xff\xff\xff\xffdtsf':
            return self._process_packet_legacy_64(packet)
        if packet.type == b'\xff\xff\xff\xffiext':
            return self._process_packet_extended(packet)
        if packet.type == b'\xff\xff\xff\xffiex+':
            return self._process_packet_extended_more(packet)

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
