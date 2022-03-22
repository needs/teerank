"""
Top tabs component.
"""

from flask import url_for

def init(tab: dict) -> dict:
    """
    Build a section tabs with the proper values.
    """

    gametypes = ['CTF', 'DM', 'TDM']

    # Main gametypes tabs

    left = []
    right = []

    for gametype in gametypes:
        if tab['type'] == 'gametype' and tab['gametype'] == gametype:
            left.append({
                'active': True,
                'links': [{
                    'name': gametype,
                }, {
                    'name': tab['map'] if tab['map'] else 'All maps',
                    'url': url_for('route_maps', gametype=gametype)
                }]
            })
        else:
            left.append({
                'active': False,
                'links': [{
                    'name': gametype,
                    'url': url_for('routes.players', gametype=gametype)
                }]
            })

    # Non-main gametype tab.

    if tab['type'] == 'gametype' and tab['gametype'] not in gametypes:
        left.append({
            'active': True,
            'links': [{
                'name': tab['gametype'],
            }, {
                'name': tab['map'],
                'url': url_for('route_maps', gametype=gametype)
            }]
        })

    # "..." tab.

    left.append({
        'active': tab['type'] == '...',
        'links': [{
            'name': '...',
            'url': None if tab['type'] == '...' else url_for('routes.gametypes')
        }]
    })

    # Any custom tab.

    if tab['type'] == 'custom':
        right.append({
            'active': True,
            'links': tab['links']
        })

    # "About" tab.

    right.append({
        'active': tab['type'] == 'about',
        'links': [{
            'name': 'About',
            'url': None if tab['type'] == 'about' else url_for('routes.about')
        }]
    })

    return [left, right]
