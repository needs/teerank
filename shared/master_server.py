"""
Implement MasterServer.
"""

import socket
import json

from shared.database import redis, key_from_address, key_to_address
from shared.server import Server

class MasterServer(Server):
    """
    Teeworld master server.
    """

    def __init__(self, key: str):
        """
        Initialize master server with the given key.
        """

        self.key = key

        data = redis.get(f'master-servers:{key}')

        if data:
            self.data = json.loads(data.decode())
        else:
            self.data = None

        # Master servers host needs to be resolved to an IP.
        host, port = key_to_address(key)
        ip = socket.gethostbyname(host)

        super().__init__(ip, port)


    def save(self) -> None:
        """
        Save master server.
        """
        redis.sadd('master-servers', self.key)
        redis.set(f'master-servers:{self.key}', json.dumps(self.data))


    @staticmethod
    def keys() -> list[str]:
        """
        Load all master servers keys from the database.
        """

        keys = redis.smembers('master-servers')

        if keys:
            return [key.decode() for key in keys]

        return (
            key_from_address('master1.teeworlds.com', 8300),
            key_from_address('master2.teeworlds.com', 8300),
            key_from_address('master3.teeworlds.com', 8300),
            key_from_address('master4.teeworlds.com', 8300)
        )
