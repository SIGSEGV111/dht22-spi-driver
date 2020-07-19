#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

#include "dht22.hpp"

namespace dht22
{
	bool DEBUG = false;

	inline static bool Bit(const void* const arr, const size_t index)
	{
		const size_t idx_byte = index / 8;
		const uint8_t idx_bit = index % 8;
		const uint8_t mask = 1 << (7 - idx_bit);
		const bool value = (((const uint8_t*)arr)[idx_byte] & mask) != 0;
		return value;
	}

	static unsigned CountBits(const void* const buffer, size_t& bitpos, const unsigned min, const unsigned max, const bool value)
	{
		unsigned n = 0;
		for(; n < max && Bit(buffer, bitpos) == value; bitpos++)
			n++;
		if(n > max)
			throw(value ? "too many high bits" : "too many low bits");
		if(n < min)
			throw(value ? "too few high bits" : "too few low bits");
		return n;
	}

	static void SeekOverHeader(const void* const buffer, size_t& bitpos)
	{
		if(Bit(buffer, bitpos) != true)
			throw "first bit must be a high bit => circuit/bus error?";

		// initial no-mans-land phase; resistor will pull high for 20-40 µs until DHT22 takes control
		CountBits(buffer, bitpos, 1, 6, true);

		// 80µs low
		CountBits(buffer, bitpos, 6, 10, false);

		// 80µs high
		CountBits(buffer, bitpos, 6, 10, true);
	}

	bool TDHT22::ParsePwmBit(const void* const buffer, size_t& bitpos)
	{
		// 50µs low pulse (this is represented by 3 to 7 low bits, depending on alignment of bus clock and pulse modulation from DHT22)
		CountBits(buffer, bitpos, 3, 7, false);

		// 26µs to 70µs high pulse (this is represented by 2 to 9 high bits, depending on alignment of bus clock and pulse modulation from DHT22)
		const unsigned n = CountBits(buffer, bitpos, 2, 9, true);

		// a short pulse should be exactly 2 high bits and borderline even 3 high bits
		// a long pulse could at worst be interpreted as only 5 high bits and at most as 9 high bits
		// ...if the DHT22 and SPI bus are working to specs...

		// so if it is 4 high bits, we have no idea what this is supposed to mean
		if(n == 4)
			throw "misleading PWM data encountered (4 high bits)";

		return n >= 5;
	}

	uint8_t TDHT22::ParsePwmByte(const void* const buffer, size_t& bitpos)
	{
		uint8_t v = 0;
		for(int i = 7; i >= 0; i--)
			v |= ((ParsePwmBit(buffer, bitpos) ? 1 : 0) << i);
		return v;
	}

	void TDHT22::ParsePwmData(const void* const buffer, float& celsius_temp, float& percent_humidity)
	{
		if(buffer == NULL)
			throw "'buffer', 'celsius_temp' and 'percent_humidity' cannot be NULL";

		size_t bitpos = 0;
		SeekOverHeader(buffer, bitpos);
		const uint8_t humidity_int    = ParsePwmByte(buffer, bitpos);
		const uint8_t humidity_dec    = ParsePwmByte(buffer, bitpos);
		const uint8_t temperature_int = ParsePwmByte(buffer, bitpos);
		const uint8_t temperature_dec = ParsePwmByte(buffer, bitpos);
		const uint8_t checksum_rx     = ParsePwmByte(buffer, bitpos);
		const uint8_t checksum_calc   = humidity_int + humidity_dec + temperature_int + temperature_dec;

		if(checksum_calc != checksum_rx)
			throw "checksum does not match";

		// write sensor readings
		percent_humidity = (humidity_int * 256 + humidity_dec) / 10.0f;
		celsius_temp = ((temperature_int & 0x7F) * 256 + temperature_dec) / 10.0f;
		if(temperature_int & 0x80)
			celsius_temp *= -1.0;
	}

	static void Hexdump(const char* const msg, const void* const buffer, const unsigned sz_buffer)
	{
		fprintf(stderr, "[DEBUG] %s:", msg);
		for(unsigned i = 0; i < sz_buffer; i++)
			fprintf(stderr, " %02hhx", reinterpret_cast<const uint8_t* const>(buffer)[i]);
		fprintf(stderr, "\n");
	}

	void TDHT22::Refresh()
	{
		const unsigned n_start_signal = 250; // 20 * 1000 / 10 / 8
		const unsigned n_data = 60;
		const unsigned sz_buffer = n_start_signal + n_data + 10; // 250 + 60 + a couple of bytes extra (=> 320 bytes)
		uint8_t tx[sz_buffer];
		uint8_t rx[sz_buffer];

		// set the first 250 bytes to 0, to create a 20ms long low pulse at the start (each bit takes 10µs @ 100KHz)
		// set the remainder of the buffer to 1 to create a constant high value on the bus, which will be filtered out by the diode
		memset(tx, 0, n_start_signal);
		memset(tx + n_start_signal, 255, sz_buffer - n_start_signal);

		// clear RX buffer
		memset(rx, 0, sz_buffer);

		if(DEBUG) Hexdump("tx-buffer", tx, sz_buffer);

		// sent TX buffer and at the same time receive RX buffer
		this->dev->ExchangeData(tx, rx, sz_buffer);

		if(DEBUG) Hexdump("rx-buffer", rx, sz_buffer);

		// the first 'n_start_signal' bytes of the RX buffer must be zeros, or something is wrong with the circuit/bus
		for(unsigned i = 0; i < n_start_signal; i++)
			if(rx[i] != 0)
				throw "non-zero bus voltage received during 20ms start-signal phase => circuit/bus error?";

		// parse the received PWM data (hand only the data part over, skip the start-signal part)
		TDHT22::ParsePwmData(rx + n_start_signal, this->celsius_temp, this->percent_humidity);
	}

	static unsigned long long SampleTimeToFrequency(unsigned long long ns)
	{
		const unsigned long long ns_per_second = 1000000000;
		return ns_per_second / ns;
	}

	TDHT22::TDHT22(TSPIDriver* dev) : dev(dev)
	{
		const unsigned long long freq = SampleTimeToFrequency(10000);
		if(DEBUG) fprintf(stderr, "[DEBUG] spi-freq = %llu Hz\n", freq);
		dev->Speed(freq);
		this->Refresh();
	}

	void TSPIDriver::SendData(const void* buffer, const size_t n_bytes)
	{
		struct spi_ioc_transfer xfer_cmd;
		memset(&xfer_cmd, 0, sizeof(xfer_cmd));

		xfer_cmd.tx_buf = (size_t)buffer;
		xfer_cmd.len = n_bytes;
		xfer_cmd.delay_usecs = 0;
		xfer_cmd.speed_hz = this->hz_speed;
		xfer_cmd.bits_per_word = 8;

		SYSERR(ioctl(this->fd, SPI_IOC_MESSAGE(1), &xfer_cmd));
	}

	void TSPIDriver::ExchangeData(const void* buffer_send, void* buffer_receive, const size_t n_bytes)
	{
		struct spi_ioc_transfer xfer_cmd;
		memset(&xfer_cmd, 0, sizeof(xfer_cmd));

		xfer_cmd.tx_buf = (size_t)buffer_send;
		xfer_cmd.rx_buf = (size_t)buffer_receive;
		xfer_cmd.len = n_bytes;
		xfer_cmd.delay_usecs = 0;
		xfer_cmd.speed_hz = this->hz_speed;
		xfer_cmd.bits_per_word = 8;

		SYSERR(ioctl(this->fd, SPI_IOC_MESSAGE(1), &xfer_cmd));
	}

	TSPIDriver::TSPIDriver(const char* const spidev, const long long hz_speed) : hz_speed(hz_speed), fd(-1)
	{
		SYSERR(this->fd = open(spidev, O_RDWR | O_CLOEXEC | O_NOCTTY));
	}

	TSPIDriver::~TSPIDriver()
	{
		if(close(this->fd) == -1)
			perror("TSPIDriver::~TSPIDriver: failed to close handle to spidev");
	}
}

