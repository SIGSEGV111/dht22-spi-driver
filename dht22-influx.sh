#!/bin/bash

set -e
set -u
set -o pipefail

exec 0</dev/null
P="$(dirname "$(readlink -f "$0")")"

set -x
source "/etc/location.conf"
exec 3>>"$CSV"
flock --exclusive --nonblock 3
set +x

echo "Country: $COUNTRY"
echo "City: $CITY"
echo "Building: $BUILDING"
echo "Floor: $FLOOR"
echo "Wing: $WING"
echo "Room: $ROOM"

exec 1>/var/log/dht22-influx.log 2>&1
set +e
while true; do
	data="environment,country=$COUNTRY,city=$CITY,building=$BUILDING,floor=$FLOOR,wing=$WING,room=$ROOM $("$P/dht22-influx" 3)"
	curl --silent -XPOST "http://$SERVER/write?db=data" --data-binary "$data"
	sleep 5
done &
