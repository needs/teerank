"""
Implement Player and PlayerELO classes.
"""

import json

from backend.database import redis

class Player:
    """
    Teerank player.
    """

    def __init__(self, key: str):
        """
        Load player with the given key.
        """
        self.key = key

        data = redis.get(f'players:{key}')

        if data:
            self.data = json.loads(data)
        else:
            self.data = None


    def save(self) -> None:
        """
        Save player data.
        """
        redis.set(f'players:{self.key}', json.dumps(self.data))


    @staticmethod
    def _elo_key(game_type: str, map_name: str) -> str:
        """
        Return redis sorted set key for the given game type and map.
        """

        if game_type is None and map_name is None:
            return 'rank'
        if game_type is not None and map_name is None:
            return f'rank-gametype:{game_type}'
        if game_type is None and map_name is not None:
            return f'rank-map:{map_name}'

        return f'rank-gametype-map:{game_type}:{map_name}'


    @staticmethod
    def get_elo(player_key: str, game_type: str, map_name: str) -> int:
        """
        Load player ELO for the given game type.
        """

        elo = redis.zscore(Player._elo_key(game_type, map_name), player_key)
        return elo if elo else 1500


    @staticmethod
    def set_elo(player_key: str, game_type: str, map_name: str, elo: int) -> None:
        """
        Save player ELO for the given game type.
        """

        redis.zadd(Player._elo_key(game_type, map_name), {player_key: elo})
