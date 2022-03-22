"""
Test /status.
"""

def test_status(client):
    """
    Test /status.
    """
    assert client.get('/status').status_code == 200
