"""
Implement MasterServer.
"""

import socket

from backend.server import Server
from backend.packet import Packet
from backend.server_pool import server_pool
from backend.game_server import GameServer

import shared.master_server


class MasterServer(Server):
    """
    Teeworld master server.
    """

    def __init__(self, host: str, port: str):
        """
        Initialize master server with the given host and port.
        """

        super().__init__(host, port)
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
                    host = socket.inet_ntop(socket.AF_INET, data[12:16])
                else:
                    host = '[' + socket.inet_ntop(socket.AF_INET6, data[:16]) + ']'

                port = str(int.from_bytes(packet.unpack_bytes(2), byteorder='big'))

                if not (host == self.host and port == self.port):
                    if (host, port) not in server_pool:
                        server_pool.add(GameServer(host, port))

            self._packet_count += 1
