.PHONY: all clean run install-influx

all: example dht22-csv dht22-influx

example: dht22.cpp example.cpp Makefile
	g++ dht22.cpp example.cpp -o example -O3 -flto -g  -Wall -Wextra -Werror

dht22-csv: dht22.cpp dht22-csv.cpp Makefile
	g++ dht22.cpp dht22-csv.cpp -o dht22-csv -O3 -flto -g  -Wall -Wextra -Werror

dht22-influx: dht22.cpp dht22-influx.cpp Makefile
	g++ dht22.cpp dht22-influx.cpp -o dht22-influx -O3 -flto -g  -Wall -Wextra -Werror

run: example
	./example

clean:
	rm -vf example dht22-influx dht22-csv

install-influx: Makefile dht22-influx
	install -v dht22-influx dht22-influx.sh /usr/local/sbin/
