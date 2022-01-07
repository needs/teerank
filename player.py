class Player:
    def __init__(self, key: str = None, data: dict = {}):
        """Initialize player with the given key and optionally with the given data."""
        if key is None:
            key = data['name'].encode('utf-8').hex()

        self.key = key
        self.redis_key = f'players:{key}'


    def load(self) -> None:
        """Loads player data."""
        # self.data = json.loads(redis.get(self.redis_key))


    def save(self) -> None:
        """Save player data."""
        # redis.set(self.redis_key, json.dumps(self.data))


class PlayerELO:
    """
    Provides helpers to load and save player ELO.
    """

    @staticmethod
    def _key(game_type: str, map_name: str) -> str:
        """
        Return redis sorted set key for the given game type and map.
        """

        if game_type is None and map_name is None:
            return f'rank'
        elif game_type is not None and map_name is None:
            return f'rank-gametype:{game_type}'
        elif game_type is None and map_name is not None:
            return f'rank-map:{map_name}'
        else:
            return f'rank-gametype-map:{game_type}:{map_name}'


    @staticmethod
    def load(name: str, game_type: str, map_name: str) -> int:
        """
        Load player ELO for the given game type.
        """
        # redis.zscore(PlayerELO._key(game_type, map_name), name)
        return 1500


    @staticmethod
    def save(name: str, game_type: str, map_name: str, elo: int) -> None:
        """
        Save player ELO for the given game type.
        """

        # redis.zadd(PlayerELO._key(game_type, map_name), (elo, name))
        logging.info(f'Gametype: {game_type}: map {map_name}: player {name}: new_elo: {elo}')
        pass
