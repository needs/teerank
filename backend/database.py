"""
Connect to the databases used by teerank, and also implement some helpers.
"""

import os
from base64 import b64encode, b64decode

from redis import Redis as redis_connect

#
# Redis
#

_redis_host = os.getenv('TEERANK_REDIS_HOST', 'database')
_redis_port = os.getenv('TEERANK_API_REDIS_PORT', '6379')

redis = redis_connect(host=_redis_host, port=_redis_port)

#
# Helpers
#

def key_to_address(key: str) -> tuple[str, int]:
    """
    Return the host and port encoded by the given key.
    """
    host, port = key.rsplit(':', 1)
    return (host, int(port))


def key_from_address(host: str, port: int) -> str:
    """
    Return an unique key from the given host and port.
    """
    return f'{host}:{port}'


def key_to_string(key: str) -> str:
    """
    Return the string encoded by the key.
    """
    return b64decode(key).decode()


def key_from_string(string: str) -> str:
    """
    Return an unique key from the given string.
    """
    return b64encode(string.encode()).decode()
