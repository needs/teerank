"""
Implement /maps.
"""

from frontend.routes import blueprint

@blueprint.route('/maps', endpoint='maps')
def route_maps():
    """
    List of maps for a given game type.
    """
