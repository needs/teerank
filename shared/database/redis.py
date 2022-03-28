"""
Helper functions to use our Redis database.
"""

from redis import Redis as redis_connect


_REDIS = None

def init(host: str, port: str) -> None:
    """
    Connect to redis.
    """

    global _REDIS #pylint: disable=global-statement
    _REDIS = redis_connect(host=host, port=port)


def redis():
    """
    Return redis handle.
    """
    return _REDIS
