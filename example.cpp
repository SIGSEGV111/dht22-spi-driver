#include <stdio.h>
#include "dht22.hpp"

using namespace dht22;

int main()
{
	try
	{
		// a small example
		dht22::DEBUG = true;

		// attach an instance of the SPI-driver to the first SPI device on the first bus (0.0)
		// transfer speed will be set by the driver
		TSPIDriver spi_dev_1("/dev/spidev0.0", 0);
		TDHT22 sensor1(&spi_dev_1);

		printf("temp: %f °C; humidity %f %%\n", sensor1.TemperatureCelsius(), sensor1.HumidityPercent());

		// c++ destructors will take care of proper shutdown and release of resources...
	}
	catch(const char* msg)
	{
		fprintf(stderr, "ERROR: %s\n", msg);
		return 1;
	}
	return 0;
}
