"""
Operations on PlayerInfo.
"""

from gql import gql
from backend.database import graphql

import backend.database.player
import backend.database.map


_GQL_GET_PLAYER_INFO = gql(
    """
    query get($map_id: ID!, $player_name: String!) {
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

    mutation create($player_info: AddPlayerInfoInput!) {
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
    """
)

def get(map_id: str, player_name: str) -> dict:
    """
    Get or create player info for the given player name on the given map ID.
    """

    player_info = dict(graphql().execute(
        _GQL_GET_PLAYER_INFO,
        operation_name = 'get',
        variable_values = {
            'map_id': map_id,
            'player_name': player_name,
        }
    ))['getMap']['playerInfos']

    if not player_info:
        player_info = dict(graphql().execute(
            _GQL_GET_PLAYER_INFO,
            operation_name = 'create',
            variable_values = {
                'player_info': {
                    'player': backend.database.player.ref(player_name),
                    'score': 0,
                    'map': backend.database.map.ref(map_id)
                }
            }
        ))['addPlayerInfo']['playerInfo']

    return player_info[0]


_GQL_UPDATE_PLAYER_INFO = gql(
    """
    mutation ($input: UpdatePlayerInfoInput!) {
        updatePlayerInfo(input: $input) {
            numUids
        }
    }
    """
)

def update(player_info: dict) -> None:
    """
    Save the given player_info.
    """

    copy = player_info.copy()
    copy.pop('id')

    graphql().execute(
        _GQL_UPDATE_PLAYER_INFO,
        variable_values = {
            'input': {
                'filter': {
                    'id': player_info['id']
                },
                'set': copy
            }
        }
    )
