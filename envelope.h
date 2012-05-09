// Envelope wrapping/unwrapping
// Author: Max Schwarz <max@x-quadraht.de>

#ifndef LIBUCOMM_ENEVELOPE_H
#define LIBUCOMM_ENEVELOPE_H

#include <stdint.h>
#include "util/error.h"
#include "util/integers.h"

/*
 * LIBUCOMM ESCAPE CODES
 *
 * Code       | Meaning
 * ====================
 * 0xFF 0xFE  | 0xFF
 * 0xFF 0xFD  | End-of-Packet
 * 0xFF ****  | Start-of-Packet
 *
 */

namespace uc
{

template<class CharIO, class ChecksumGenerator>
class EnvelopeWriter : public CharIO
{
public:
	bool startEnvelope(uint8_t msg_code)
	{
		RETURN_IF_ERROR(CharIO::writeChar(0xFF));
		RETURN_IF_ERROR(CharIO::writeChar(0xFF));
		RETURN_IF_ERROR(CharIO::writeChar(msg_code));

		m_checksum.reset();
		m_checksum.add(msg_code);
	}

	bool write(const void* data, size_t size)
	{
		for(size_t i = 0; i < size; ++i)
		{
			uint8_t c = ((const uint8_t*)data)[i];
			RETURN_IF_ERROR(CharIO::writeChar(c));
			if(c == 0xFF)
				RETURN_IF_ERROR(CharIO::writeChar(0));

			m_checksum.add(c);
		}

		return true;
	}

	bool endEnvelope(uint8_t msg_code)
	{
		CharIO::writeChar(m_checksum.value());
	}
private:
	ChecksumGenerator m_checksum;
};

template<class CharIO, class ChecksumGenerator, int MaxPacketSize>
class EnvelopeReader : public CharIO
{
public:
	typedef IntForSize<MaxPacketSize>::Type SizeType;

	bool take
private:
	enum State
	{
		STATE_START1,
		STATE_START2,
		STATE_MSGCODE,
		STATE_DATA,
		STATE_CHECKSUM
	};

	uint8_t m_state;
	uint8_t m_buffer[MaxPacketSize];
};

}

#endif

