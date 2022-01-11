"""
Implement Server class.
"""

from dataclasses import dataclass, field

@dataclass
class Server:
    """
    Server class.
    """

    ip: str
    port: int
    address: tuple[str, int] = field(init=False)

    def __post_init__(self):
        self.address = (self.ip, self.port)
