#ifndef __include_dht22_dht22_hpp__
#define __include_dht22_dht22_hpp__

#include <stdint.h>

namespace dht22
{
	class TSPIDriver;

	extern bool DEBUG;

	class TDHT22
	{
		protected:
			TSPIDriver* dev;
			float celsius_temp;
			float percent_humidity;

		public:

			inline float TemperatureCelsius() const { return this->celsius_temp; }
			inline float HumidityPercent() const { return this->percent_humidity; }

			static bool ParsePwmBit(const void* const buffer, size_t& bitpos);
			static uint8_t ParsePwmByte(const void* const buffer, size_t& bitpos);
			static void ParsePwmData(const void* const buffer, float& celsius_temp, float& percent_humidity);
			void Refresh();

			TDHT22(TSPIDriver* dev);
	};

	// this is a pretty simple wrapper driver for the SPI bus
	// all it does it handle the open/close and data transfer calls to the kernel
	class TSPIDriver
	{
		private:
			TSPIDriver(const TSPIDriver&);

		protected:
			long long hz_speed;
			int fd;

		public:
			void Speed(long long hz) { this->hz_speed = hz; }
			long long Speed() const { return this->hz_speed; }
			void SendData(const void* buffer, const size_t n_bytes);
			void ExchangeData(const void* buffer_send, void* buffer_receive, const size_t n_bytes);

			TSPIDriver(const char* const spidev, const long long hz_speed = 0);
			~TSPIDriver();
	};
}

#endif
