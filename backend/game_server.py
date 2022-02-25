"""
Implement GameServer, GameServerState and GameServerType classes.
"""

import secrets
from enum import IntEnum

from backend.packet import Packet, PacketException
from backend.rank import rank
from backend.server import Server

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


class GameServer(Server):
    """
    Teeworld game server.
    """

    def __init__(self, host: str, port: str):
        """
        Initialize game server with the given host and port.
        """

        super().__init__(host, port)

        self.state = shared.game_server.get(host, port)
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

        if self._state_new.get('num_clients', -1) != len(self._state_new.get('clients', {})):
            return False

        # Now that the server is fully updated, save its state.

        old_state = self.state
        self.state = self._state_new
        self._state_new = None

        shared.game_server.set(self.host, self.port, self.state)
        self.process_state(self.state)

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


    @staticmethod
    def _process_packet_vanilla(packet: Packet) -> dict:
        """
        Parse the default response of the vanilla client.
        """

        state = {
            'type': GameServerType.VANILLA,
            'version': packet.unpack(),
            'name': packet.unpack(),
            'map_name': packet.unpack(),
            'game_type': packet.unpack(),
            'flags': packet.unpack_int(),
            'num_players': packet.unpack_int(),
            'max_players': packet.unpack_int(),
            'num_clients': packet.unpack_int(),
            'max_clients': packet.unpack_int(),

            'clients': []
        }

        while packet.unpack_remaining() >= 5:
            state['clients'].append({
                'name': packet.unpack(),
                'clan': packet.unpack(),
                'country': packet.unpack_int(),
                'score': packet.unpack_int(),
                'ingame': bool(packet.unpack_int())
            })

        return state


    @staticmethod
    def _process_packet_legacy_64(packet: Packet) -> dict:
        """
        Parse legacy 64 packet.
        """

        state = {
            'type': GameServerType.LEGACY_64,
            'version': packet.unpack(),
            'name': packet.unpack(),
            'map_name': packet.unpack(),
            'game_type': packet.unpack(),
            'flags': packet.unpack_int(),
            'num_players': packet.unpack_int(),
            'max_players': packet.unpack_int(),
            'num_clients': packet.unpack_int(),
            'max_clients': packet.unpack_int(),

            'clients': []
        }

        # Even though the offset is advertised as an integer, in real condition
        # we receive a single byte.

        packet.unpack_bytes(1) # Offset

        while packet.unpack_remaining() >= 5:
            state['clients'].append({
                'name': packet.unpack(),
                'clan': packet.unpack(),
                'country': packet.unpack_int(),
                'score': packet.unpack_int(),
                'ingame': bool(packet.unpack_int())
            })

        return state


    @staticmethod
    def _process_packet_extended(packet: Packet) -> dict:
        """
        Parse the extended server info packet.
        """

        state = {
            'type': GameServerType.EXTENDED,
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
            'max_clients': packet.unpack_int(),

            'clients': []
        }

        packet.unpack() # Reserved

        while packet.unpack_remaining() >= 6:
            state['clients'].append({
                'name': packet.unpack(),
                'clan': packet.unpack(),
                'country': packet.unpack_int(-1),
                'score': packet.unpack_int(),
                'ingame': bool(packet.unpack_int())
            })
            packet.unpack() # Reserved

        return state


    @staticmethod
    def _process_packet_extended_more(packet: Packet) -> dict:
        """
        Parse the extended server info packet.
        """

        state = {
            'type': GameServerType.EXTENDED,
            'clients': []
        }

        packet.unpack_int() # Packet number
        packet.unpack() # Reserved

        while packet.unpack_remaining() >= 6:
            state['clients'].append({
                'name': packet.unpack(),
                'clan': packet.unpack(),
                'country': packet.unpack_int(-1),
                'score': packet.unpack_int(),
                'ingame': bool(packet.unpack_int())
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

            players_name = [client['name'] for client in state['clients']]
            players_clan = shared.player.get_clan(players_name)

            #
            # Create new clans and update players clan.
            #
            # Since player clan have a @hasInverse() pointing to the clan player
            # list, updating players clan automatically updates clan players list
            # as well.
            #

            unique_clans = set()

            for client in state['clients']:
                if client['clan']:
                    unique_clans.add(client['clan'])

            shared.clan.upsert([{'name': clan} for clan in unique_clans])

            players = []

            for client in state['clients']:
                clan_ref = { 'name': client['clan'] } if client['clan'] else None

                players.append({
                    'name': client['name'],
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
