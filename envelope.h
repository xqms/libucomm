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
	class Reader
	{
	};

	bool startEnvelope(uint8_t msg_code)
	{
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

	bool endEnvelope()
	{
		RETURN_IF_ERROR(CharIO::writeChar(0xFF));
		RETURN_IF_ERROR(CharIO::writeChar(0xFD));
		RETURN_IF_ERROR(CharIO::writeChar(m_checksum.value()));
	}
private:
	ChecksumGenerator m_checksum;
};

template<class CharIO, class ChecksumGenerator, int MaxPacketSize>
class EnvelopeReader : public CharIO
{
public:
	typedef typename IntForSize<MaxPacketSize>::Type SizeType;

	EnvelopeReader()
	 : m_state(STATE_START1)
	{
	}

	bool take(uint8_t c)
	{
		switch(m_state)
		{
			case STATE_START1:
				if(c == 0xFF)
					m_state = STATE_START2;
				break;
			case STATE_START2:
				if(c != 0xFE && c != 0xFD)
				{
					m_msgCode = c;
					m_idx = 0;
					m_state = STATE_DATA;
					m_generator.reset();
				}
				break;
			case STATE_DATA:
				if(m_idx == MaxPacketSize)
				{
					m_state = STATE_START1;
					break;
				}

				if(c == 0xFF)
				{
					m_state = STATE_ESCAPE;
					break;
				}

				m_buffer[m_idx++] = c;
				m_generator.add(c);
				break;
			case STATE_ESCAPE:
				if(c == 0xFE)
				{
					// Already checked if there is enough room in STATE_DATA
					m_buffer[m_idx++] = 0xFF;
					m_state = STATE_DATA;
					m_generator.add(0xFF);
					break;
				}

				if(c == 0xFD)
				{
					m_state = STATE_CHECKSUM;
				}

				// Start-of-Packet detetected
				m_msgCode = c;
				m_idx = 0;
				m_state = STATE_DATA;
				m_generator.reset();
				break;
			case STATE_CHECKSUM:
				if(c == m_generator.value())
				{
					// Packet detected
				}
				else if(c == 0xFF)
					m_state = STATE_START2;
				else
					m_state = STATE_START1;
				break;
		}
	}
private:
	enum State
	{
		STATE_START1,
		STATE_START2,
		STATE_DATA,
		STATE_ESCAPE,
		STATE_CHECKSUM
	};

	uint8_t m_state;
	uint8_t m_msgCode;
	uint8_t m_buffer[MaxPacketSize];
	SizeType m_idx;

	ChecksumGenerator m_generator;
};

}

#endif

