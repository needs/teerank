"""
Launch frontend application.
"""

from flask import Flask, render_template

from shared.player import players_by_rank
from shared.database import key_to_string

app = Flask(__name__)

@app.route("/")
def index():
    """
    Teeranks entry point.
    """
    game_type = 'CTF'

    players_key_elo = players_by_rank(game_type, None, 0, 100)
    players = []

    for rank, key_elo in enumerate(players_key_elo):
        key = key_elo[0].decode()
        elo = key_elo[1]

        players.append({
            'rank': rank,
            'name': key_to_string(key),
            'elo': elo
        })

    return render_template('base.html', game_type=game_type, players=players)
