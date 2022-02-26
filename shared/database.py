"""
Connect to the databases used by teerank, and also implement some helpers.
"""

import os
import logging
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


def graphql_drop_all_data() -> None:
    """
    Drop all data stored in graphql.
    """

    requests.post(f'http://{_graphql_url}/alter', data='{"drop_op": "DATA"}')
