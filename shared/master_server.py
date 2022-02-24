"""
Helpers for master server.
"""

def all() -> list[tuple[str, str]]:
    """
    Get the list of all master servers host and port.
    """

    return [
        ('master1.teeworlds.com', "8300"),
        ('master2.teeworlds.com', "8300"),
        ('master3.teeworlds.com', "8300"),
        ('master4.teeworlds.com', "8300")
    ]
