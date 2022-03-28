"""
Helper functions to use our GraphQL database.
"""

import logging
from time import sleep
import json

import requests
from gql import gql, Client as gql_connect
from gql.transport.aiohttp import AIOHTTPTransport, log as aiohttp_logger


_GRAPHQL_HOST = None
_GRAPHQL_PORT = None
_GRAPHQL = None

def init(host: str, port: int, set_schema: bool = False) -> None:
    """
    Connect to graphql and optionally set database schema.
    """
    global _GRAPHQL_HOST, _GRAPHQL_PORT, _GRAPHQL #pylint: disable=global-statement

    _GRAPHQL_HOST = host
    _GRAPHQL_PORT = port

    _GRAPHQL = gql_connect(
        transport=AIOHTTPTransport(url=f'http://{host}:{port}/graphql')
    )

    # By default gql is very verbose, limit logs to only warnings and above.
    aiohttp_logger.setLevel(logging.WARNING)

    if set_schema:
        # Setting graphQL schema in dgraph requires a specially crafted HTTP request
        # to /admin/schema.  It can fail for many reasons, most of the time dgraph
        # is not ready or our IP is not whitelisted.

        with open('./shared/database/schema.graphql', 'r', encoding='utf-8') as file:
            schema = file.read()

        while True:
            response = requests.post(f'http://{host}:{port}/admin/schema', data=schema)

            if response.status_code == 200 and 'errors' not in response.json():
                break

            logging.warning(response.text)
            sleep(10)


def drop_all_data() -> None:
    """
    Drop all data stored in graphql.
    """

    requests.post(f'http://{_GRAPHQL_HOST}:{_GRAPHQL_PORT}/alter', data='{"drop_op": "DATA"}')


def drop_all() -> None:
    """
    Drop all data stored in graphql.
    """

    requests.post(f'http://{_GRAPHQL_HOST}:{_GRAPHQL_PORT}/alter', data='{"drop_op": true}')


class RawString(str):
    """
    This class is used by serialize_string() and unserialize_string().  When a
    string is an instance of this class, then it will not be
    serialized/unserialized.

    This is especially useful for sending regular expressions to dgraph.  For
    instance a regular expression that contains an escaped user string will not
    work as expected because the escaping that was done will be serialized a
    second time and that will change the escaped value.
    """


def _deep_copy(value, copy):
    """
    Recursively copy the given value and call copy() on its non-dict and
    non-list values.
    """

    if isinstance(value, dict):
        value = { key: _deep_copy(val, copy) for key, val in value.items() }

    elif isinstance(value, list):
        value = [ _deep_copy(val, copy) for val in value ]

    else:
        value = copy(value)

    return value


def serialize_string(value):
    """
    Encode the string using the JSON encoder because it removes all escape
    sequence that break DGraph lexer.  Also remove the surounding quotes to save
    a little more space.
    """

    if isinstance(value, str) and not isinstance(value, RawString):
        value = json.dumps(value)[1:-1]

    return value


def unserialize_string(value):
    """
    Cancel the effect of _serialize_string().
    """

    if isinstance(value, str) and not isinstance(value, RawString):
        value = json.loads(f'"{value}"')

    return value


def graphql(query, variables=None, operation=None):
    """
    Execute the given query and return the results.
    """

    # Cache GQL'ed queries for faster performances.

    if query not in graphql.queries:
        graphql.queries[query] = gql(query)

    query = graphql.queries[query]

    if variables is not None:
        variables = _deep_copy(variables, serialize_string)

    result = _GRAPHQL.execute(query, variable_values=variables, operation_name=operation)

    return _deep_copy(result, unserialize_string)

graphql.queries = {}
