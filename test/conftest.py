"""
Common fixtures for tests.
"""

import pytest_redis
import pytest

import backend.database
import frontend.app

redis_instance = pytest_redis.factories.redis_noproc(
    host='redis'
)

_redis = pytest_redis.factories.redisdb(
    'redis_instance'
)

backend.database.graphql_init('dgraph-alpha', '8080')
backend.database.redis_init('redis', '6379')


@pytest.fixture(autouse=True)
def fixture_reset_databases(_redis):
    """
    Empty databases before each test.
    """

    # Note: the redis database is automatically reseted thanks to the _redis
    # fixture.

    backend.database.graphql_drop_all_data()


@pytest.fixture(name='app', scope='session')
def fixture_app():
    """
    Flask fixture for our application.

    Since tests will use the test client rather than this fixture directly,
    bounding it to the session scope works and will improve performances.
    """

    app = frontend.app.create_app()
    app.testing = True

    return app


@pytest.fixture(name='client')
def fixture_client(app):
    """
    Create a test client for our application.
    """

    return app.test_client()
