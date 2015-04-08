// Envelope wrapping/unwrapping
// Author: Max Schwarz <max@x-quadraht.de>

#ifndef LIBUCOMM_ENEVELOPE_H
#define LIBUCOMM_ENEVELOPE_H

#include <stdint.h>
#include <string.h>
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

class CharWriter
{
public:
	virtual bool writeChar(uint8_t c) = 0;
};

template<class ChecksumGenerator, class CharWriterType = CharWriter>
class EnvelopeWriter
{
public:
	class Reader
	{
	};

	EnvelopeWriter(CharWriterType* charWriter)
	 : m_charWriter(charWriter)
	{
	}

	bool startEnvelope(uint8_t msg_code)
	{
		RETURN_IF_ERROR(m_charWriter->writeChar(0xFF));
		RETURN_IF_ERROR(m_charWriter->writeChar(msg_code));

		m_checksum.reset();
		m_checksum.add(msg_code);

		return true;
	}

	bool write(const void* data, size_t size)
	{
		for(size_t i = 0; i < size; ++i)
		{
			uint8_t c = ((const uint8_t*)data)[i];
			RETURN_IF_ERROR(m_charWriter->writeChar(c));
			if(c == 0xFF)
				RETURN_IF_ERROR(m_charWriter->writeChar(0xFE));

			m_checksum.add(c);
		}

		return true;
	}

	bool endEnvelope()
	{
		RETURN_IF_ERROR(m_charWriter->writeChar(0xFF));
		RETURN_IF_ERROR(m_charWriter->writeChar(0xFD));
		RETURN_IF_ERROR(m_charWriter->writeChar(m_checksum.value()));

		return true;
	}

	template<class MSG>
	EnvelopeWriter<ChecksumGenerator, CharWriterType>& operator<< (const MSG& msg)
	{
		startEnvelope(MSG::MSG_CODE);
		msg.serialize(this);
		endEnvelope();

		return *this;
	}
private:
	CharWriterType* m_charWriter;
	ChecksumGenerator m_checksum;
};

template<class ChecksumGenerator, int MaxPacketSize>
class EnvelopeReader
{
public:
	typedef typename IntForSize<MaxPacketSize>::Type SizeType;

	class Reader
	{
	public:
		Reader()
		{
		}

		Reader(EnvelopeReader* envReader)
		 : m_envReader(envReader)
		 , m_idx(0)
		{
		}

		bool read(void* data, size_t size)
		{
			if(m_idx + size > m_envReader->m_idx)
				return false;

			memcpy(data, m_envReader->m_buffer + m_idx, size);
			m_idx += size;

			return true;
		}

		bool skip(size_t size)
		{
			if(m_idx + size > m_envReader->m_idx)
				return false;

			m_idx += size;

			return true;
		}
	private:
		EnvelopeReader* m_envReader;
		SizeType m_idx;
	};

	friend class Reader;

	EnvelopeReader()
	 : m_state(STATE_START1)
	{
	}

	enum TakeResult
	{
		NEW_MESSAGE,
		NEED_MORE_DATA,
		CHECKSUM_ERROR
	};

	TakeResult take(uint8_t c)
	{
		switch(m_state)
		{
			case STATE_START1:
				if(c == 0xFF)
					m_state = STATE_START2;
				break;
			case STATE_START2:
				if(c != 0xFF && c != 0xFE && c != 0xFD)
				{
					m_msgCode = c;
					m_idx = 0;
					m_state = STATE_DATA;
					m_generator.reset();
					m_generator.add(c);
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
					break;
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
					m_state = STATE_START1;
					return NEW_MESSAGE;
				}
				else
				{
					if(c == 0xFF)
						m_state = STATE_START2;
					else
						m_state = STATE_START1;

					return CHECKSUM_ERROR;
				}
				break;
		}

		return NEED_MORE_DATA;
	}

	uint8_t msgCode() const
	{ return m_msgCode; }

	inline Reader makeReader()
	{
		return Reader(this);
	}

	template<class MSG>
	EnvelopeReader<ChecksumGenerator, MaxPacketSize>& operator>>(MSG& msg)
	{
		Reader reader = makeReader();
		msg.deserialize(&reader);

		return *this;
	}

	template<class MSG>
	bool read(MSG* msg)
	{
		Reader reader = makeReader();
		return msg->deserialize(&reader);
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

