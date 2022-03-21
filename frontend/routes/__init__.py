"""
Routes module.
"""

from flask import Blueprint

blueprint = Blueprint('routes', __name__)

#
# For convenience we want to import all routes here, however since all routes
# will use the blueprint variable, we need to declare blueprint before importing
# the routes.
#
# pylint: disable=wrong-import-position

from .players import *
