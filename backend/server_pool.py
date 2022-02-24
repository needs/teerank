"""
Implement ServerPool class.
"""

from dataclasses import dataclass, field
from socket import socket as Socket, gethostname
from socket import AF_INET, SOCK_DGRAM
import heapq
import logging
import select
import time
import random
import os

from backend.server import Server
from backend.packet import Packet, PacketException

class ServerPool:
    """A list of servers to poll for data."""

    # Time between two poll for the same server.
    POLL_DELAY = 3 * 60

    # Maximum number of packet sent for each poll.
    MAX_PACKET_SENT_PER_POLL = 25

    # Number of poll failure after a server is removed.
    MAX_POLL_FAILURE = 3

    @dataclass(order=True)
    class _Entry:
        server: Server = field(compare=False)
        poll_time: float = field(init=False)
        poll_failure: int = 0

        def __post_init__(self):
            """
            Randomize poll delay so that servers are evenly spread for polling,
            and that should decrease packet loss even further.
            """
            self.poll_time = time.time() + random.randrange(ServerPool.POLL_DELAY)


    def __init__(self):
        """Initialize a server pool."""
        self._entries = []
        self._servers = {}
        self._batch = {}
        self._socket = Socket(family=AF_INET, type=SOCK_DGRAM)

        # Bind socket to a specific port to be able to tell docker which port to
        # expose in order to receive packets.

        host = gethostname()
        port = int(os.getenv('TEERANK_BACKEND_PORT', '8311'))

        self._socket.bind((host, port))
        logging.info('Socket bound to %s:%d', host, port)


    def __contains__(self, address: tuple[str, int]) -> bool:
        """Return whether or not the given address is in the server pool."""
        return address in self._servers


    def add(self, server: Server) -> None:
        """Add the given server to the pool."""

        logging.info('Adding server %s', server)

        heapq.heappush(self._entries, ServerPool._Entry(server))
        self._servers[server.address] = server


    def poll(self) -> None:
        """Poll servers."""

        current_time = time.time()

        # Process any incoming packets.

        while select.select([self._socket], [], [], 0)[0]:

            data, address = self._socket.recvfrom(4096)
            entry = self._batch.get(address, None)

            if entry is not None:
                try:
                    entry.server.process_packet(Packet(data))
                except PacketException as exception:
                    logging.info('Server %s: Dropping packet: %s', entry.server, exception)

        # Stop polling for the current batch and re-schedule servers.

        for entry in self._batch.values():
            if entry.server.stop_polling():
                entry.poll_time += ServerPool.POLL_DELAY
                entry.poll_failure = 0

            else:
                entry.poll_failure += 1

                if entry.poll_failure == ServerPool.MAX_POLL_FAILURE:
                    logging.info('Removing server %s', entry.server)
                    del self._servers[entry.server.address]
                    continue

            heapq.heappush(self._entries, entry)

        self._batch = {}

        # Poll any server in the queue that have their poll timer exhausted.
        # Limit the number of packet sent to limit the number of packet loss.

        packet_count = 0

        while self._entries:
            if self._entries[0].poll_time > current_time:
                break
            if packet_count >= ServerPool.MAX_PACKET_SENT_PER_POLL:
                break

            entry = heapq.heappop(self._entries)
            server = entry.server
            packets = server.start_polling()

            for packet in packets:
                self._socket.sendto(bytes(packet), server.address)

            self._batch[server.address] = entry
            packet_count += len(packets)


server_pool = ServerPool()
