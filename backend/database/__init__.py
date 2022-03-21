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

_REDIS = None

def redis_init(host: str, port: str) -> None:
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


#
# Graphql
#

_GRAPHQL_HOST = None
_GRAPHQL_PORT = None
_GRAPHQL = None

def graphql():
    """
    Return graphql handle.
    """
    return _GRAPHQL


def graphql_init(host: str, port: int) -> None:
    """
    Connect to graphql and optionally set database schema.
    """
    global _GRAPHQL_HOST, _GRAPHQL_PORT, _GRAPHQL #pylint: disable=global-statement

    _GRAPHQL_HOST = host
    _GRAPHQL_PORT = port

    _GRAPHQL = gql_connect(
        transport=AIOHTTPTransport(url=f'http://{host}:{port}/graphql'),
    )

    # By default gql is very verbose, limit logs to only warnings and above.
    aiohttp_logger.setLevel(logging.WARNING)

    # Setting graphQL schema in dgraph requires a specially crafted HTTP request
    # to /admin/schema.  It can fail for many reasons, most of the time dgraph
    # is not ready or our IP is not whitelisted.

    with open('./backend/database/schema.graphql', 'r', encoding='utf-8') as file:
        schema = file.read()

    while True:
        response = requests.post(f'http://{host}:{port}/admin/schema', data=schema)

        if response.status_code == 200 and 'errors' not in response.json():
            break

        logging.warning(response.text)
        sleep(10)


def graphql_drop_all_data() -> None:
    """
    Drop all data stored in graphql.
    """

    requests.post(f'http://{_GRAPHQL_HOST}:{_GRAPHQL_PORT}/alter', data='{"drop_op": "DATA"}')
