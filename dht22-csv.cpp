#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/file.h>
#include "dht22.hpp"

#define SYSERR(expr) (([&](){ const auto r = ((expr)); if( (long)r == -1L ) { throw #expr; } else return r; })())

static volatile bool do_run = true;

static void OnSignal(int)
{
	do_run = false;
}

int main(int argc, char* argv[])
{
	signal(SIGINT,  &OnSignal);
	signal(SIGTERM, &OnSignal);
	signal(SIGHUP,  &OnSignal);
	signal(SIGQUIT, &OnSignal);

	close(STDIN_FILENO);

	try
	{
		if(argc < 3)
			throw "need exactly two arguments: <SPI device> <location>";

		using namespace dht22;
		::dht22::DEBUG = false;

		TSPIDriver spidev(argv[1]);
		TDHT22 dht22(&spidev);

		while(do_run)
		{
			timeval ts;

			dht22.Refresh();
			SYSERR(gettimeofday(&ts, NULL));

			SYSERR(flock(STDOUT_FILENO, LOCK_EX));
			printf("%ld.%06ld;\"%s\";\"dht22\";\"temperature\";%f\n", ts.tv_sec, ts.tv_usec, argv[2], dht22.TemperatureCelsius());
			printf("%ld.%06ld;\"%s\";\"dht22\";\"humidity\";%f\n", ts.tv_sec, ts.tv_usec, argv[2], dht22.HumidityPercent());
			fflush(stdout);
			SYSERR(flock(STDOUT_FILENO, LOCK_UN));

			usleep(15 * 1000 * 1000);
		}

		fprintf(stderr,"\n[INFO] bye!\n");
		return 0;
	}
	catch(const char* const err)
	{
		fprintf(stderr, "[ERROR] %s\n", err);
		perror("[ERROR] kernel message");
		return 1;
	}
	return 2;
}
