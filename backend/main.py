"""
Launch backend server.
"""

from time import sleep
import logging

import backend.database

from backend.server_pool import server_pool
from backend.master_server import MasterServer
from backend.game_server import GameServer

import backend.database.master_server
import backend.database.game_server


if __name__ == '__main__':
    logging.basicConfig()
    logging.getLogger().setLevel(logging.INFO)

    backend.database.graphql_init('dgraph-alpha', '8080')
    backend.database.redis_init('redis', '8080')

    for address in backend.database.master_server.all_addresses():
        server_pool.add(MasterServer(address))

    for address in backend.database.game_server.all_addresses():
        server_pool.add(GameServer(address))

    while True:
        sleep(1)
        server_pool.poll()
