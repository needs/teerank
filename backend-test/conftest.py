"""
Common fixtures for backend tests.
"""

import pytest_redis

redis_instance = pytest_redis.factories.redis_noproc(
    host='database-test'
)

_redis = pytest_redis.factories.redisdb(
    'redis_instance'
)
