"""
Common fixtures for tests.
"""

import pytest_redis
import pytest

import backend.database

redis_instance = pytest_redis.factories.redis_noproc(
    host='redis-test'
)

_redis = pytest_redis.factories.redisdb(
    'redis_instance'
)

backend.database.graphql_init('dgraph-alpha-test', '8080')
backend.database.redis_init('redis-test', '6379')


@pytest.fixture(name='_graphql')
def fixture_graphql():
    """
    Empty graphql before each test.
    """
    backend.database.graphql_drop_all_data()
