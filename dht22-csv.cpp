#include <stdio.h>
#include <sys/time.h>
#include "dht22.hpp"

using namespace dht22;

int main()
{
	try
	{
		// a small example (but write as CSV including timestamps)

		// attach an instance of the SPI-driver to the first SPI device on the first bus (0.0)
		// transfer speed will be set by the driver
		TSPIDriver spi_dev_1("/dev/spidev0.0", 0);

		timeval tv_start, tv_end;
		SYSERR(gettimeofday(&tv_start, NULL));
		TDHT22 sensor1(&spi_dev_1);
		SYSERR(gettimeofday(&tv_end, NULL));

		printf("%ld.%06ld;%ld.%06ld;%f;%f\n", tv_start.tv_sec, tv_start.tv_usec, tv_end.tv_sec, tv_end.tv_usec, sensor1.TemperatureCelsius(), sensor1.HumidityPercent());

		// c++ destructors will take care of proper shutdown and release of resources...
	}
	catch(const char* msg)
	{
		fprintf(stderr, "ERROR: %s\n", msg);
		return 1;
	}
	return 0;
}
