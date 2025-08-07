# greenhouse
Greenhouse automation software

The package contains:

- Esp32 code: sketch for the Esp32 board
- MQTT service: systemd service reading from the Mosquitto broker
- Python script to save data into a Sqlite DB

## Create the python environment

In `/opt` create the folder `mqttenv` and install Paho:

```bash
cd /opt
python3 -m venv mqttenv
source /opt/mqttenv/bin/activate
pip3 install paho-mqtt
```

Alternatively, use the `install_gh_mqtt.sh` script to generate the environment automatically.

## Prepare the required files

Create a folder somewhere in the system. Inside the new folder copy the `gh_mttq_sub.py` file and create a new database.

```bash
mkdir -p /opt/greenhouse
cd /opt/greenhouse
mv ~/gh_mttq_sub.py .
sqlite3 greenhouse.db
```

To create the table in the database use the query in `greenhouse.sql`.

## Create and start the service

Copy the `mqtt_greenhouse.service` file in `/etc/systemd/system`. Enable and start the service

```bash
sudo systemctl enable mqtt_greenhouse
sudo systemctl start mqtt_greenhouse
```

Mosquitto Must be installed and running.

