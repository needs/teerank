"""
Launch backend server.
"""

from time import sleep
import logging

from shared.database import graphql_set_schema

from backend.server_pool import server_pool
from backend.master_server import MasterServer
from backend.game_server import GameServer

import shared.master_server
import shared.game_server


if __name__ == '__main__':
    logging.basicConfig()
    logging.getLogger().setLevel(logging.INFO)

    graphql_set_schema()

    for address in shared.master_server.all():
        server_pool.add(MasterServer(address[0], address[1]))

    for address in shared.game_server.all():
        server_pool.add(GameServer(address[0], address[1]))

    while True:
        sleep(1)
        server_pool.poll()
