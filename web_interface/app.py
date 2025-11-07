
from flask import Flask, request, jsonify
import sqlite3
from datetime import datetime, timedelta

app = Flask(__name__)

DB_PATH = "/opt/greenhouse/greenhouse.db"


def heater_changes(rows):
    is_on = None
    current_on = None
    all_changes = []

    # Check the initial state
    row = rows[0]
    is_on = bool(row[5])
    current_on = row[0] if is_on else None

    # Loop on all the other rows
    for i in range(1, len(rows)):
        row = rows[i]
        prev_state = is_on
        is_on = bool(row[5])

        if not prev_state and is_on:  # Turned on
            current_on = row[0]
        elif prev_state and not is_on and current_on:  # Turned off
            all_changes.append((current_on, row[0]))
            current_on = None

    # Handle case where heater is still on at end of data
    if current_on:
        all_changes.append((current_on, rows[-1][0]))

    return all_changes

def query_data(days):
    conn = sqlite3.connect(DB_PATH)
    cursor = conn.cursor()
    since = datetime.now() - timedelta(days=days)
    cursor.execute("""
        SELECT [time], [temperature_1], [temperature_2], [humidity_1],
            [humidity_2], [heater_on]
        FROM [data]
        WHERE [time] >= ?
        ORDER BY [time] ASC
    """, (since.strftime("%Y-%m-%d %H:%M"),))
    rows = cursor.fetchall()
    conn.close()

    heater_data = heater_changes(rows)

    return [{
        "data": [
            {
                "date": row[0],
                "temp1": row[1],
                "temp2": row[2],
                "hum1": row[3],
                "hum2": row[4],
            }
            for row in rows
        ],
        "heater": heater_data
    }]

@app.route("/api/data")
def get_data():
    days = int(request.args.get("days", 3))
    data = query_data(days)
    return jsonify(data)

if __name__ == "__main__":
    app.run(host="127.0.0.1", port=4411)
