"""
Common fixtures for tests.
"""

import pytest_redis
import pytest

import shared.database

redis_instance = pytest_redis.factories.redis_noproc(
    host='redis-test'
)

_redis = pytest_redis.factories.redisdb(
    'redis_instance'
)

@pytest.fixture(scope='session')
def fixture_graphql_start():
    """
    Prepare Dgraph to accept queries.
    """
    shared.database.graphql_set_schema()


@pytest.fixture(name='_graphql')
def fixture_graphql():
    """
    Empty graphql before each test.
    """
    shared.database.graphql_drop_all_data()
    yield
