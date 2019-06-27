#!/bin/bash

set -e
set -u
set -o pipefail

export PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/sbin:/usr/local/bin
mkdir -p /var/lib/dht22

exec >> /var/lib/dht22/csv
if ! flock --exclusive --nonblock 1; then
	echo "another instance of this script is already running => terminating" 1>&2
	exit 1
fi

dht22-csv

exec 2> /var/log/dht22-csv.err
set +e

for ((;;)); do
	sleep 10
	dht22-csv
done &
