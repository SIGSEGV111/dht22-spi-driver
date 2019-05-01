#!/bin/bash

set -e
set -u
set -o pipefail
# shopt -s extglob

set -x
source "/etc/location.conf"
exec <"$CSV"
set +x

function FormatTimestamp()
{
	sec="${ts1/.*/}"
	us="${ts1:$((${#sec}+1))}"
	if ((${#us} == 0)); then us="000000"; fi
	if ((${#us} != 6)); then return 1; fi
	ts="${sec}${us}000"
}

pv | while IFS=';' read ts1 ts2 temp humidity; do
	FormatTimestamp

	if ((${#temp} != 0 && ${#humidity} != 0)); then
		echo "environment,country=$COUNTRY,city=$CITY,building=$BUILDING,floor=$FLOOR,wing=$WING,room=$ROOM temperature=$temp,humidityPerc=$humidity $ts"
	fi
done | curl -i -XPOST "http://$SERVER/write?db=data" --data-binary @-
