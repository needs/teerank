"""
Common fixtures for tests.
"""

import pytest_redis

redis_instance = pytest_redis.factories.redis_noproc(
    host='redis-test'
)

_redis = pytest_redis.factories.redisdb(
    'redis_instance'
)
