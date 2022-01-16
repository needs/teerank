"""
Common fixtures for tests.
"""

import pytest_redis
from shared.database import redis

redis_instance = pytest_redis.factories.redis_noproc(
    host='database-test'
)

_redis = pytest_redis.factories.redisdb(
    'redis_instance'
)

def pytest_sessionfinish(session, exitstatus): # pylint: disable=unused-argument
    """
    Shutdown redis server once all tests have been run.
    """
    redis.shutdown()
