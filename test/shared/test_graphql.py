"""
Test shared.graphql.
"""

from shared.database.graphql import graphql


def test_graphql():
    """
    Test that our module fix a bug in DGraph lexer where some escape sequences
    are not handled and make the query crash.
    """

    strings = [
        '\x00',
        '\U000e0021',
        '\U000f0009',
        '\v',
        '\n',
        'null'
    ]

    for string in strings:
        player = {
            'name': string,
        }

        stored_player = graphql("""
            mutation ($player: AddPlayerInput!) {
                addPlayer(input: [$player]) {
                    player {
                        name
                    }
                }
            }
            """, {
                'player': player
            }
        )['addPlayer']['player'][0]

        print(player)
        print(stored_player)
        print(repr(player['name']))
        print(repr(stored_player['name']))

        assert stored_player['name'] == player['name']
