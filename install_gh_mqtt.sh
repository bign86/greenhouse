#!/bin/bash
set -x

echo ">>> Installing the nfpy package"

# Prepare the folders for the python environment
BASE_DIR="/opt"
ENV_DIR="mqttenv"

if [ ! -d "$ENV_DIR" ];
then
    echo -n "No ${ENV_DIR} python environment found, creating it in ${BASE_DIR}/${ENV_DIR}... "
    cd ${BASE_DIR}
    python3 -m venv ${ENV_DIR}
    echo "done"
else
    echo "Found an environment in ${BASE_DIR}/${ENV_DIR}"
fi


# Activate the environment
echo "Activate the environment..."
source ${BASE_DIR}/${ENV_DIR}/bin/activate
ACTIVE_PYTHON=$(python3 -c "import sys; print('python{}.{}'.format(sys.version_info.major, sys.version_info.minor))")
echo "Activated python: "${ACTIVE_PYTHON}

# Update pip and cython
echo "Update pip..."
pip3 install --upgrade pip

# Check if the nfpy is already installed
PAHO_INFO=$(pip3 list | grep -F paho)
if [ -z "$PAHO_INFO" ];
then
    echo "paho not installed"
    pip3 install paho-mqtt
else
    PAHO_VERSION=$(echo "$PAHO_INFO" | awk '{print $2}')
    echo "Found paho version: $PAHO_VERSION"
    pip3 install --upgrade paho-mqtt
fi


# All done
echo "<<< All done!"

set +x
