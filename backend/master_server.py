"""
Implement MasterServer.
"""

import socket

from shared.database import key_from_address
from shared.master_server import MasterServer as DatabaseMasterServer

from backend.packet import Packet
from backend.server_pool import server_pool
from backend.game_server import GameServer


class MasterServer(DatabaseMasterServer):
    """
    Teeworld master server.
    """

    def __init__(self, key: str):
        """
        Initialize master server with the given key.
        """

        super().__init__(key)
        self._packet_count = None


    def start_polling(self) -> list[Packet]:
        """
        Poll master server.
        """

        self._packet_count = 0

        # 10 bytes of padding and the 'req2' packet type.
        return [Packet(b'\xff\xff\xff\xff\xff\xff\xff\xff\xff\xffreq2')]


    def stop_polling(self) -> bool:
        """
        Stop master server polling.
        """

        # There is no reliable way to know when all packets have been received.
        # Therefor when at least one packet have been received, we considere
        # that the polling was a success and we move on.

        return self._packet_count > 0


    def process_packet(self, packet: Packet) -> None:
        """
        Process master server packet.

        Add any game server not yet in the server pool.
        """

        packet.unpack_bytes(10) # Padding
        packet_type = packet.unpack_bytes(4)

        if packet_type == b'lis2':

            while len(packet) >= 18:
                data = packet.unpack_bytes(16)

                if data[:12] == b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xff\xff':
                    ip = socket.inet_ntop(socket.AF_INET, data[12:16])
                else:
                    ip = '[' + socket.inet_ntop(socket.AF_INET6, data[:16]) + ']'

                port = int.from_bytes(packet.unpack_bytes(2), byteorder='big')

                if not (ip == self.ip and port == self.port):
                    if (ip, port) not in server_pool:
                        server_pool.add(GameServer(key_from_address(ip, port)))

            self._packet_count += 1
