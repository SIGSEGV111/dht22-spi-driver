#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "dht22.hpp"

using namespace dht22;

int main(int argc, char* argv[])
{
	try
	{
		// Raspberry Pi DHT22 logger

		// attach an instance of the SPI-driver to the first SPI device on the first bus (0.0)
		// transfer speed will be set by the driver
		TSPIDriver spi_dev_1("/dev/spidev0.0", 0);

		timeval tv_start, tv_end;
		SYSERR(gettimeofday(&tv_start, NULL));
		TDHT22 sensor1(&spi_dev_1);
		SYSERR(gettimeofday(&tv_end, NULL));

		// write in influx compatible format to stdout
		const unsigned long long ns = tv_start.tv_sec * 1000000000ULL + tv_start.tv_usec * 1000ULL;
		printf("temperature=%f,humidityPerc=%f %llu\n", sensor1.TemperatureCelsius(), sensor1.HumidityPercent(), ns);

		// also write to CSV on an extra FD if requested
		if(argc > 1)
		{
			const int fd_csv = atoi(argv[1]);
			char buffer[256];
			ssize_t sz = snprintf(buffer, sizeof(buffer), "%ld.%06ld;%ld.%06ld;%f;%f\n", tv_start.tv_sec, tv_start.tv_usec, tv_end.tv_sec, tv_end.tv_usec, sensor1.TemperatureCelsius(), sensor1.HumidityPercent());

			ssize_t written = 0, w;
			while(written < sz)
			{
				SYSERR(w = write(fd_csv, buffer, sz));
				if(w <= 0)
					throw "disk full while writing CSV data";
				written += w;
			}
		}

		// c++ destructors will take care of proper shutdown and release of resources...
	}
	catch(const char* msg)
	{
		fprintf(stderr, "ERROR: %s\n", msg);
		return 1;
	}
	return 0;
}
