"""
Launch backend server.
"""

from time import sleep
import logging

from shared.master_server import MasterServer as DatabaseMasterServer
from shared.game_server import GameServer as DatabaseGameServer
from shared.database import graphql_set_schema

from backend.server_pool import server_pool
from backend.master_server import MasterServer
from backend.game_server import GameServer

if __name__ == '__main__':
    logging.basicConfig()
    logging.getLogger().setLevel(logging.INFO)

    graphql_set_schema()

    for key in DatabaseMasterServer.keys():
        server_pool.add(MasterServer(key))

    for key in DatabaseGameServer.keys():
        server_pool.add(GameServer(key))

    while True:
        sleep(1)
        server_pool.poll()
