# Greenhouse
Greenhouse automation software

The package contains:

- Esp32 code: sketch for the Esp32 board
- MQTT service: systemd service reading from the Mosquitto broker
- Python script to save data into a Sqlite DB
- Micro website to monitor the greenhouse
  - Website API (Flask)
  - Website main page (html)
  - Website service

## Prepare the microcontroller

Flash the microcontroller with the sketch `sketch_greenhouse/sketch_greenhouse.ino`.
Make sure to insert the required passwords and IPs for the wifi and the mqtt host.

## Receive the data from the greenhouse

### Create the python environment

In `/opt` create the folder `mqttenv` and install Paho:

```bash
cd /opt
python3 -m venv mqttenv
source /opt/mqttenv/bin/activate
pip3 install paho-mqtt
```

Alternatively, use the `install_gh_mqtt.sh` script to generate the environment automatically.

### Prepare the required files

Create a folder somewhere in the system, the script absolute paths are hardcoded with `/opt/greenhouse`. Inside the new folder copy the `gh_mttq_sub.py` and the SQL file, then create a new database.

```bash
mkdir -p /opt/greenhouse
cd /opt/greenhouse
mv ~/gh_mttq_sub.py ~/greenhouse.sql .
sqlite3 greenhouse.db
```

Make sure both the `greenhouse.db` file and the `greenhouse` folder are owned or have permissions for the user that will run the service.
To create the table in the database use the query in `greenhouse.sql`.

### Create and start the service

Copy the `mqtt_greenhouse.service` file in `/etc/systemd/system`.

```bash
mkdir -p /opt/greenhouse
cd /opt/greenhouse
mv ~/gh_mttq_sub.py ~/greenhouse.sql .
sqlite3 greenhouse.db
```

In the file search for `<USER>` and substitute with the user that will run the service.
Enable and start the service

```bash
sudo systemctl enable mqtt_greenhouse
sudo systemctl start mqtt_greenhouse
```

Mosquitto must be installed and running.

## Create the website to monitor the greenhouse

### Prepare the files

Create a folder that will contain the website

```bash
mkdir -p /opt/greenhouse
cd /opt/greenhouse
mkdir web_interface
```

Copy into `/opt/greenhouse/web_interface` the file `app.py` for the APIs and the `static` folder containing the html.

### Configure and start the website

Go to the nginx folder containing the websites configurations

```bash
cd /etc/nginx/sites-available
```

and copy the file `gh_web_interface`. Enable the website and restart nginx

```bash
sudo ln -s /etc/nginx/sites-available/gh_web_interface /etc/nginx/sites-enabled/
sudo nginx -t
sudo systemctl reload nginx
```

The website is available at the 7777 port.

### Configure and start the APIs

Copy the `gh_web_interface.service` file into `/etc/systemd/system`. Enable and start the service

```bash
sudo systemctl enable gh_web_interface
sudo systemctl start gh_web_interface
```

The APIs are available at the 4411 port.


