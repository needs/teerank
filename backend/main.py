"""
Launch backend server.
"""

from time import sleep
import logging

from backend.server_pool import server_pool
from backend.master_server import load_master_servers
from backend.game_server import load_game_servers

if __name__ == '__main__':
    logging.basicConfig()
    logging.getLogger().setLevel(logging.INFO)

    for master_server in load_master_servers():
        server_pool.add(master_server)

    for game_server in load_game_servers():
        server_pool.add(game_server)

    while True:
        sleep(1)
        server_pool.poll()
