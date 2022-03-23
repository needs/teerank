"""
Test /$path/search.
"""

import pytest
import backend.database.clan

def create_player():
    """
    Create and return a new player.
    """

    player = {
        'name': 'test-player'
    }

    backend.database.player.upsert([player])
    return player['name']


def create_clan():
    """
    Create and return a new clan.
    """

    clan = {
        'name': 'test-clan'
    }

    backend.database.clan.upsert([clan])
    return clan['name']


def create_server():
    """
    Create and return a new server.
    """

    game_server = {
        'address': 'foo:8000',
        'name': 'test-server',

        'gameType': 'CTF',
        'map': 'ctf1',

        'numPlayers': 1,
        'maxPlayers': 16,
        'numClients': 1,
        'maxClients': 16,
    }

    backend.database.game_server.upsert(game_server)
    return game_server['name']


# Simple way to perform the same test for players, clans and servers search.
pytestmark = pytest.mark.parametrize(
    "path, create", [
        ('players', create_player),
        ('clans', create_clan),
        ('servers', create_server)
    ]
)

def test_route_search_exact(client, path, create):
    """
    Test /$path/search when the query is an exact match.
    """

    string = create()
    response = client.get(f'/{path}/search?q={string}')

    assert response.status_code == 200
    assert string.encode() in response.data


def test_route_search_prefix(client, path, create):
    """
    Test /$path/search when the query is a prefix.
    """

    string = create()
    response = client.get(f'/{path}/search?q={string[:4]}')

    assert response.status_code == 200
    assert string.encode() in response.data


def test_route_search_suffix(client, path, create):
    """
    Test /$path/search when the query is a suffix.
    """

    string = create()
    response = client.get(f'/{path}/search?q={string[4:]}')

    assert response.status_code == 200
    assert string.encode() in response.data


def test_route_search_any(client, path, create):
    """
    Test /$path/search when the query match somewher in the middle of clan name.
    """

    string = create()
    response = client.get(f'/{path}/search?q={string[2:6]}')

    assert response.status_code == 200
    assert string.encode() in response.data


def test_route_search_no_create(client, path, create):
    """
    Test /$path/search when there is no entries to search on.
    """

    # pylint: disable=unused-argument

    response = client.get(f'/{path}/search?q=foo')

    assert response.status_code == 200
    assert f'No {path} found.'.encode() in response.data


def test_route_search_no_match(client, path, create):
    """
    Test /$path/search when there is a clan but the query does not match it.
    """

    query = 'foo'
    string = create()
    response = client.get(f'/{path}/search?q={query}')

    assert query.lower() not in string.lower()
    assert response.status_code == 200
    assert f'No {path} found.'.encode() in response.data


def test_route_search_case_insensitive(client, path, create):
    """
    Test /$path/search when the query match but the case is different.
    """

    string = create()
    response = client.get(f'/{path}/search?q={string.swapcase()}')

    assert response.status_code == 200
    assert string.encode() in response.data
