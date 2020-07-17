#/bin/bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

docker run \
    -d \
    --name grafana \
    -p 9091:3000 \
    -e "GF_SECURITY_ADMIN_PASSWORD=secret" \
    grafana/grafana
