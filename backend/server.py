"""
Implement Server class.
"""

import socket
from dataclasses import dataclass, field
from urllib.parse import urlparse

@dataclass
class Server:
    """
    Server class.
    """

    address: str
    socket_address: tuple[str, int] = field(init=False)

    def __post_init__(self):
        url = urlparse('//' + self.address)

        self.socket_address = (
            socket.gethostbyname(url.hostname),
            url.port if url.port else 8300
        )

    def __str__(self):
        return self.address
