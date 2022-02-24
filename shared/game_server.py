"""
Helpers for game server.
"""

from enum import IntEnum


class GameServerType(IntEnum):
    """
    Game server type.  Higher is better.
    """

    UNKNOWN = -1
    VANILLA = 0
    LEGACY_64 = 1
    EXTENDED = 2


class GameServerState:
    """
    Internal game server state.
    """

    def __init__(self, server_type: GameServerType):
        """
        Initialize a server state with the given server type.
        """

        self.server_type = server_type
        self.info = {}
        self.clients = {}


    def merge(self, state: 'GameServerState') -> None:
        """
        Merge two states into a single one.

        The server state with the highest server type overide the other.  If
        server types are equal, do a classic merge.
        """

        if self.server_type <= state.server_type:
            self.server_type = state.server_type
            if state.info:
                self.info = state.info

            if state.server_type == self.server_type:
                self.clients |= state.clients
            else:
                self.clients = state.clients


    def is_complete(self) -> bool:
        """
        Return true if server state is valid and have all its data.
        """

        return self.info and len(self.clients) == self.info['num_clients']


def all() -> list[tuple[str, str]]:
    """
    Get the list of all game servers host and port.
    """
    return []


def get(host: str, port: str) -> GameServerState:
    """
    Get the game server with the given host and port.
    """
    return GameServerState(GameServerType.UNKNOWN)


def set(host: str, port: str, state: GameServerState) -> None:
    """
    Set the game server with the given host and port.
    """
    pass
