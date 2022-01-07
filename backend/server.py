"""
Implement Server class.
"""

import secrets

class Server:
    """
    Server class.
    """

    def __init__(self, ip: str, port: int):
        """
        Initialize server with the given IP and port.
        """

        self.ip = ip
        self.port = port
        self.key = f'{ip}:{port}'
        self.address = (ip, port)
        self.token = secrets.token_bytes(nbytes=4)
