"""
Helpers for master server.
"""

def all_addresses() -> list[str]:
    """
    Get the list of all master servers address.
    """

    return [
        'master1.teeworlds.com:8300',
        'master2.teeworlds.com:8300',
        'master3.teeworlds.com:8300',
        'master4.teeworlds.com:8300'
    ]
