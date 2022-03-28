"""
Implement /status.
"""

import datetime

from flask import render_template
from shared.database.graphql import graphql

import frontend.components.top_tabs

def route_status():
    """
    Render the status page.
    """

    master_servers = graphql("""
        query {
            queryMasterServer {
                address
                downSince
                gameServersAggregate {
                    count
                }
            }
        }
        """
    )['queryMasterServer']

    for master_server in master_servers:
        if master_server['downSince']:
            master_server['status'] = 'Down'
            master_server['status_class'] = 'down'
            if master_server['downSince'] != datetime.datetime.min.isoformat() + 'Z':
                master_server['comment'] = master_server['downSince']
        else:
            master_server['status'] = 'OK'
            master_server['status_class'] = 'ok'
            master_server['comment'] = f'{master_server["gameServersAggregate"]["count"]} servers'

    return render_template(
        'status.html',
        top_tabs = frontend.components.top_tabs.init({
            'type': 'custom',
            'links': [{
                'name': 'Status'
            }]
        }),
        master_servers = master_servers
    )
