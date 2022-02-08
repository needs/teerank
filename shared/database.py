"""
Connect to the databases used by teerank, and also implement some helpers.
"""

import os
import logging
from base64 import b64encode, b64decode
from time import sleep

import requests
from redis import Redis as redis_connect
from gql import Client as gql_connect
from gql.transport.aiohttp import AIOHTTPTransport, log as aiohttp_logger

#
# Redis
#

_redis_host = os.getenv('TEERANK_REDIS_HOST', 'redis')
_redis_port = os.getenv('TEERANK_REDIS_PORT', '6379')

redis = redis_connect(host=_redis_host, port=_redis_port)

#
# Graphql
#

_graphql_host = os.getenv('TEERANK_GRAPHQL_HOST', 'dgraph-alpha')
_graphql_port = os.getenv('TEERANK_GRAPHQL_PORT', '8080')
_graphql_url = f'{_graphql_host}:{_graphql_port}'

graphql = gql_connect(
    transport=AIOHTTPTransport(url=f'http://{_graphql_url}/graphql'),
)

# By default gql is very verbose, limit logs to only warnings and above.
aiohttp_logger.setLevel(logging.WARNING)

#
# Helpers
#

def graphql_set_schema() -> None:
    """
    Set graphql schema.
    """

    # Setting graphQL schema in dgraph requires a specially crafted HTTP request
    # to /admin/schema.  It can fail for many reasons, most of the time dgraph
    # is not ready or our IP is not whitelisted.

    with open('./shared/schema.graphql', 'r', encoding='utf-8') as file:
        schema = file.read()

    while True:
        response = requests.post(f'http://{_graphql_url}/admin/schema', data=schema)

        if response.status_code == 200 and 'errors' not in response.json():
            break

        logging.warning(response.text)
        sleep(10)


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
    if string is None:
        return None
    return b64encode(string.encode()).decode()
