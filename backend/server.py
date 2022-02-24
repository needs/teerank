"""
Implement Server class.
"""

import socket
from dataclasses import dataclass, field

@dataclass
class Server:
    """
    Server class.
    """

    host: str
    port: str
    address: tuple[str, int] = field(init=False)

    def __post_init__(self):
        self.address = (socket.gethostbyname(self.host), int(self.port))

    def __str__(self):
        return self.host + ':' + self.port
