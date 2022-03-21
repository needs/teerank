"""
Implement GameServer, GameServerState and GameServerType classes.
"""

import secrets
from enum import IntEnum

from backend.packet import Packet, PacketException
from backend.rank import rank
from backend.server import Server

import backend.database.clan
import backend.database.player


class GameServerType(IntEnum):
    """
    Game server type.  Higher is better.
    """

    UNKNOWN = -1
    VANILLA = 0
    LEGACY_64 = 1
    EXTENDED = 2


class GameServer(Server):
    """
    Teeworld game server.
    """

    def __init__(self, address: str):
        """
        Initialize game server with the given address.
        """

        super().__init__(address)

        self.state = backend.database.game_server.get(address)
        self._state_new = {}
        self._request_token = None


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

        server_type = self.state.get('type', GameServerType.UNKNOWN)

        if server_type == GameServerType.EXTENDED:
            self._request_token = secrets.token_bytes(3)
        else:
            self._request_token = secrets.token_bytes(1) + bytes(2)

        # Pick a packet according to server type or send both packets if server
        # type is unknown.
        #
        # The packet is the same for both vanilla and extended servers, thanks
        # to a clever trick with the token field.

        packets = []

        if server_type in (GameServerType.UNKNOWN, \
                           GameServerType.VANILLA, \
                           GameServerType.EXTENDED):
            packets.append(self._request_info_packet(b'gie3'))
        if server_type in (GameServerType.UNKNOWN, \
                           GameServerType.LEGACY_64):
            packets.append(self._request_info_packet(b'fstd'))

        # Reset _state_new.

        self._state_new = {}

        return packets


    def stop_polling(self) -> bool:
        """
        Check any received data and process them.
        """

        # Check that data is complete by comparing the number of clients on the
        # server to the number of clients received.

        if self._state_new.get('numClients', -1) != len(self._state_new.get('clients', {})):
            return False

        # Now that the server is fully updated, save its state.

        old_state = self.state
        self.state = self._state_new
        self._state_new = None

        self.process_state(self.state)
        backend.database.game_server.upsert(self.state)

        # Rank players after saving server so that player already exist in the
        # database.

        rank(old_state, self.state)

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

        # Merge received state into the new state.  This step is required
        # because new state can be received in many smaller parts.

        if state["type"] == self._state_new.get("type", GameServerType.UNKNOWN):
            state['clients'] += self._state_new.get('clients', [])
        if state["type"] >= self._state_new.get("type", GameServerType.UNKNOWN):
            self._state_new |= state


    def _process_packet_vanilla(self, packet: Packet) -> dict:
        """
        Parse the default response of the vanilla client.
        """

        state = {}

        state['type'] = GameServerType.VANILLA
        state['address'] = self.address
        state['version'] = packet.unpack()
        state['name'] = packet.unpack()
        state['map'] = packet.unpack()
        state['gameType'] = packet.unpack()
        packet.unpack_int() # Flags
        state['numPlayers'] = packet.unpack_int()
        state['maxPlayers'] = packet.unpack_int()
        state['numClients'] = packet.unpack_int()
        state['maxClients'] = packet.unpack_int()

        state['clients'] = []

        while packet.unpack_remaining() >= 5:
            state['clients'].append({
                'player': backend.database.player.ref(packet.unpack()),
                'clan': backend.database.clan.ref(packet.unpack()),
                'country': packet.unpack_int(),
                'score': packet.unpack_int(),
                'ingame': bool(packet.unpack_int()),
                'gameServer': backend.database.game_server.ref(self.address)
            })

        return state


    def _process_packet_legacy_64(self, packet: Packet) -> dict:
        """
        Parse legacy 64 packet.
        """

        state = {}

        state['type'] = GameServerType.LEGACY_64
        state['address'] = self.address
        state['version'] = packet.unpack()
        state['name'] = packet.unpack()
        state['map'] = packet.unpack()
        state['gameType'] = packet.unpack()
        packet.unpack_int() # Flags
        state['numPlayers'] = packet.unpack_int()
        state['maxPlayers'] = packet.unpack_int()
        state['numClients'] = packet.unpack_int()
        state['maxClients'] = packet.unpack_int()

        state['clients'] = []

        # Even though the offset is advertised as an integer, in real condition
        # we receive a single byte.

        packet.unpack_bytes(1) # Offset

        while packet.unpack_remaining() >= 5:
            state['clients'].append({
                'player': backend.database.player.ref(packet.unpack()),
                'clan': backend.database.clan.ref(packet.unpack()),
                'country': packet.unpack_int(),
                'score': packet.unpack_int(),
                'ingame': bool(packet.unpack_int()),
                'gameServer': backend.database.game_server.ref(self.address)
            })

        return state


    def _process_packet_extended(self, packet: Packet) -> dict:
        """
        Parse the extended server info packet.
        """

        state = {}

        state['type'] = GameServerType.EXTENDED
        state['address'] = self.address
        state['version'] = packet.unpack()
        state['name'] = packet.unpack()
        state['map'] = packet.unpack()
        packet.unpack_int() # Map CRC
        packet.unpack_int() # Map size
        state['gameType'] = packet.unpack()
        packet.unpack_int() # Flags
        state['numPlayers'] = packet.unpack_int()
        state['maxPlayers'] = packet.unpack_int()
        state['numClients'] = packet.unpack_int()
        state['maxClients'] = packet.unpack_int()

        state['clients'] = []

        packet.unpack() # Reserved

        while packet.unpack_remaining() >= 6:
            state['clients'].append({
                'player': backend.database.player.ref(packet.unpack()),
                'clan': backend.database.clan.ref(packet.unpack()),
                'country': packet.unpack_int(-1),
                'score': packet.unpack_int(),
                'ingame': bool(packet.unpack_int()),
                'gameServer': backend.database.game_server.ref(self.address)
            })
            packet.unpack() # Reserved

        return state


    def _process_packet_extended_more(self, packet: Packet) -> dict:
        """
        Parse the extended server info packet.
        """

        state = {
            'type': GameServerType.EXTENDED,
            'address': self.address,
            'clients': []
        }

        packet.unpack_int() # Packet number
        packet.unpack() # Reserved

        while packet.unpack_remaining() >= 6:
            state['clients'].append({
                'player': backend.database.player.ref(packet.unpack()),
                'clan': backend.database.clan.ref(packet.unpack()),
                'country': packet.unpack_int(-1),
                'score': packet.unpack_int(),
                'ingame': bool(packet.unpack_int()),
                'gameServer': backend.database.game_server.ref(self.address)
            })
            packet.unpack() # Reserved

        return state


    @staticmethod
    def process_state(state: dict) -> None:
        """
        Update players and clans depending on the given state.
        """

        if state['clients']:
            #
            # Query players current clan.
            #
            # Clan maintains a count of players to be able to sort clan by their
            # number of players.  So before changing players clan, we need to
            # know in which clan the player was in to update their player count.
            #

            players_name = [client['player']['name'] for client in state['clients']]
            players_clan = backend.database.player.get_clan(players_name)

            #
            # Create new clans.
            #
            # Do this before updating players so that player can reference new
            # clans.
            #

            unique_clans = set()

            for client in state['clients']:
                if client['clan']:
                    unique_clans.add(client['clan']['name'])

            backend.database.clan.upsert([{'name': clan} for clan in unique_clans])

            #
            # Update player one by one.  It looks tempting to update all players
            # with a single upsert, but it doesn't work without some more
            # preprocessing on our side because such upsert will fail if there
            # are players with the same name.
            #
            # When two players tries to connect to a server, they are both
            # reported as (connecting) by the server, and that's how we can end
            # up having two players with the same name.
            #

            for client in state['clients']:
                backend.database.player.upsert({
                    'name': client['player']['name'],
                    'clan': client['clan']
                })

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

            for clan, count in backend.database.clan.compute_player_count(list(unique_clans)).items():
                backend.database.clan.set_player_count(clan, count)
