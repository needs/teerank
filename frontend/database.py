"""
Shared database handlers.
"""

import logging
from gql import Client as gql_connect
from gql.transport.aiohttp import AIOHTTPTransport, log as aiohttp_logger

graphql = gql_connect(
    transport=AIOHTTPTransport(url='http://dgraph-alpha:8080/graphql'),
)

# By default gql is very verbose, limit logs to only warnings and above.
aiohttp_logger.setLevel(logging.WARNING)
