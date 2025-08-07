
import datetime
import os
import sqlite3 as sql
import time

from paho.mqtt import client as mqtt_client


# Broker variables
BROKER = '<MQTT_BROKER>'
PORT = 1883
TOPIC = "gh"
CLIENT_ID = 'gh-mqtt-sub'
USERNAME = '<MQTT_USER>'
PASSWORD = '<MQTT_PWD>'

# Re-connection variables
RECONNECT_RATE = 2
MAX_ATTEMPTS = 12
RECONNECT_DELAY = 5

# Database variables
DB_FILE = 'greenhouse.db'
DB_TABLE_NAME = 'data'
DB_CREATE_FILE = 'greenhouse.sql'
DB_TABLE_EXISTS = f'''
    SELECT [name] FROM [sqlite_master]
    WHERE [type] = 'table' AND [name] = '{DB_TABLE_NAME}';
'''
DB_INSERT = f'''
    INSERT INTO {DB_TABLE_NAME} ([time], [temperature_1], [temperature_2],
    [humidity_1], [humidity_2], [fan_on], [heater_on])
    VALUES (?,?,?,?,?,?,?);
'''


# Initialize database
def db_init():
    # Connect to the DB file
    global conn
    conn = sql.connect(database=DB_FILE)

    # Check whether the table exists
    cursor = conn.cursor()
    res = cursor.execute(DB_TABLE_EXISTS).fetchall()

    # If the table does not exists create it
    if not res:
        if not os.path.exists(DB_CREATE_FILE):
            raise RuntimeError(f'{DB_CREATE_FILE} not found')
        with open(DB_CREATE_FILE, 'r') as sql_file:
            create = sql_file.read()
            cursor.executescript(create)
            conn.commit()


# On connect callback
def on_connect(client, userdata, flags, reason_code, properties):
    if reason_code == 0:
        print("MQTT client: connected to broker!")
        client.subscribe(TOPIC)
    else:
        print(f'MQTT client: failed to connect, return code {reason_code}')


# On message callback
def on_message(client, userdata, msg):
    timestamp = datetime.datetime \
                        .now() \
                        .strftime("%Y-%m-%d %H:%M")
    payload = msg.payload.decode()

    try:
        cursor = conn.cursor()
        data = [
            timestamp,
            *map(float, payload.split('|')),
            False, False
        ]

        # Insert message data into sqlite database
        cursor.execute(DB_INSERT, data)
        conn.commit()

    except sql.Error as e:
        print("MQTT client: failed to save message.", e)



# On disconnect callback
def on_disconnect(client, userdata, flags, reason_code, properties):
    print(f"MQTT client: disconnected with result code: {reason_code}")

    attemps = 0
    while attemps < MAX_ATTEMPTS:
        print(f"MQTT client: reconnecting in {RECONNECT_DELAY} seconds...")
        time.sleep(RECONNECT_DELAY)

        try:
            client.reconnect()
            print("MQTT client: reconnected successfully!")
        except Exception as err:
            print(f"MQTT client: {err}. Reconnect failed. Retrying...")
            attemps += 1

    # If reconnecting is impossible exit
    print(f"MQTT client: reconnect failed after {MAX_ATTEMPTS} attempts. Exiting...")
    client.loop_stop()
    conn.close()
    exit(1)


# Setup the MQTT client
def connect_mqtt() -> mqtt_client.Client:

    # Create client
    client = mqtt_client.Client(mqtt_client.CallbackAPIVersion.VERSION2, CLIENT_ID)
    client.username_pw_set(USERNAME, PASSWORD)

    # Callbacks
    client.on_connect = on_connect
    client.on_disconnect = on_disconnect
    client.on_message = on_message

    try:
        client.connect(BROKER, PORT)
    except Exception as e:
        print(f"MQTT client: connection attempt failed: {e}")

    return client


# Main run loop
def run():
    db_init()
    client = connect_mqtt()
    client.loop_forever()
    conn.close()


if __name__ == '__main__':
    run()

