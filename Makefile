.PHONY: clean all

all: dht22-csv

dht22-csv: dht22-csv.cpp dht22.cpp dht22.hpp Makefile
	g++ -Wall -Wextra -flto -O3 -march=native -fdata-sections -ffunction-sections -Wl,--gc-sections dht22-csv.cpp dht22.cpp -o dht22-csv

clean:
	rm -vf dht22-csv
