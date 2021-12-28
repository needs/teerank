class Player:
    def __init__(self, key: str = None, data: dict = {}):
        """Initialize player with the given key and optionally with the given data."""
        if key is None:
            key = data['name'].encode('utf-8').hex()

        self.key = key
        self.redis_key = 'players:' + key


    def load(self) -> None:
        """Loads player data."""
        # self.data = json.loads(redis.get(self.redis_key))


    def save(self) -> None:
        """Save player data."""
        # redis.set(self.redis_key, json.dumps(self.data))
