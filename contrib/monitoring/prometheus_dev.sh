#/bin/bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

docker run \
    -d \
    --name prometheus \
    -p 9090:9090 \
    -v ${SCRIPT_DIR}/prometheus.yml:/etc/prometheus/prometheus.yml \
    prom/prometheus
