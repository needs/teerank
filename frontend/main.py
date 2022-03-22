"""
Launch frontend application.
"""

from flask import Flask, render_template, url_for
from flask import request, abort
from gql import gql

import frontend.components.paginator
import frontend.components.section_tabs
import frontend.components.top_tabs
from frontend.database import graphql
import frontend.routes

#
# Flask
#

app = Flask(__name__)
app.register_blueprint(frontend.routes.blueprint)
