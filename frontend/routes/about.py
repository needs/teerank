"""
Implement /about.
"""

from flask import render_template

import frontend.components.top_tabs
from frontend.routes import blueprint

@blueprint.route('/about', endpoint='about')
def route_about():
    """
    Render the about page.
    """

    return render_template(
        'about.html',
        top_tabs = frontend.components.top_tabs.init({
            'type': 'about'
        })
    )
