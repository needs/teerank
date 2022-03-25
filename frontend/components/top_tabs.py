"""
Top tabs component.
"""

from flask import url_for

def init(active_tab: dict) -> dict:
    """
    Build a section tabs with the proper values.
    """

    gametypes = [None, 'CTF', 'DM', 'TDM']

    # Main gametypes tabs

    left = []
    right = []

    def gametype_tab(gametype: str):
        """Return a new gametype tab."""
        tab = {
            'class': '',
            'links': [{
                'name': gametype if gametype is not None else 'Global',
                'url': url_for('players', gametype=gametype)
            }]
        }

        if active_tab['type'] == 'gametype' and active_tab['gametype'] == gametype:
            tab['class'] += 'active '
            tab['links'].append({
                'name': active_tab['map'] if active_tab['map'] is not None else 'All maps',
                'url': url_for('maps', gametype=gametype)
            })

        if gametype is None:
            tab['class'] += 'global '

        return tab


    for gametype in gametypes:
        left.append(gametype_tab(gametype))

    # Non-main gametype tab.


    if active_tab['type'] == 'gametype' and active_tab['gametype'] not in gametypes:
        left.append(gametype_tab(active_tab['gametype']))

    # "..." tab.

    left.append({
        'class': 'active' if active_tab['type'] == '...' else '',
        'links': [{
            'name': '...',
            'url': url_for('gametypes')
        }]
    })

    # Any custom tab.

    if active_tab['type'] == 'custom':
        right.append({
            'class': 'active',
            'links': active_tab['links']
        })

    # "About" tab.

    right.append({
        'class': 'active' if active_tab['type'] == 'about' else '',
        'links': [{
            'name': 'About',
            'url': url_for('about')
        }]
    })

    return [left, right]
