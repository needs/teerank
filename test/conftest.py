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


@pytest.fixture(autouse=True)
def fixture_reset_databases(_redis):
    """
    Empty databases before each test.
    """

    # Note: the redis database is automatically reseted thanks to the _redis
    # fixture.

    backend.database.graphql_drop_all_data()
