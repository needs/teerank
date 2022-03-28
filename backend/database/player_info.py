"""
Operations on PlayerInfo.
"""

from shared.database.graphql import graphql

import backend.database.player
import backend.database.map


def get(map_id: str, player_name: str) -> dict:
    """
    Get or create player info for the given player name on the given map ID.
    """

    player_info = graphql("""
        query($map_id: ID!, $player_name: String!) {
            getMap(id: $map_id) {
                playerInfos @cascade(fields: "player") {
                    id
                    player(filter: { name: { eq: $player_name } }) {
                        name
                    }
                    score
                    map {
                        id
                    }
                }
            }
        }
        """, {
            'map_id': map_id,
            'player_name': player_name,
        }
    )['getMap']['playerInfos']

    if not player_info:
        player_info = graphql("""
            mutation($player_info: AddPlayerInfoInput!) {
                addPlayerInfo(input: [$player_info]) {
                    playerInfo {
                        id
                        player {
                            name
                        }
                        score
                        map {
                            id
                        }
                    }
                }
            }
            """, {
                'player_info': {
                    'player': backend.database.player.ref(player_name),
                    'score': 0,
                    'map': backend.database.map.ref(map_id)
                }
            }
        )['addPlayerInfo']['playerInfo']

    return player_info[0]


def update(player_info: dict) -> None:
    """
    Save the given player_info.
    """

    copy = player_info.copy()
    copy.pop('id')

    graphql("""
        mutation ($input: UpdatePlayerInfoInput!) {
            updatePlayerInfo(input: $input) {
                numUids
            }
        }
        """, {
            'input': {
                'filter': {
                    'id': player_info['id']
                },
                'set': copy
            }
        }
    )
