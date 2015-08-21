// Envelope format using COBS (consistent overhead byte stuffing)
// Author: Max Schwarz <max.schwarz@online.de>

#ifndef LIBUCOMM_COBS_ENVELOPE_H
#define LIBUCOMM_COBS_ENVELOPE_H

#include <stdint.h>
#include <string.h>

#include "writer.h"
#include "util/error.h"
#include "util/integers.h"

/*
 * This envelope format uses the COBS algorithm for byte stuffing. See
 * Cheshire, Stuart, and Mary Baker. "Consistent overhead byte stuffing."
 * IEEE/ACM Transactions on Networking (TON) 7.2 (1999): 159-172.
 *
 */

namespace uc
{

template<class ChecksumGenerator, class WriterType = CharWriter>
class COBSWriter
{
public:
	class Reader
	{
	};

	COBSWriter(WriterType* writer)
	 : m_writer(writer)
	{}

	bool startEnvelope(uint8_t msg_code)
	{
		m_dstPtr = m_writer->dataPointer();
		m_dstEnd = m_dstPtr + m_writer->dataSize();

		if(m_dstPtr + 2 >= m_dstEnd)
			return false;

		m_checksum.reset();

		if(msg_code >= 255)
			return false;

		*m_dstPtr++ = 0x00;

		RETURN_IF_ERROR(writeAndChecksum(msg_code + 1));

		// Init COBS (if we get a zero, this is the first code)
		m_code = 0x01;

		// Reserve a byte for the code
		m_codePtr = m_dstPtr++;

		return true;
	}

	bool write(const void* data, size_t size)
	{
		const uint8_t* ptr = reinterpret_cast<const uint8_t*>(data);
		const uint8_t* end = ptr + size;

		while(ptr != end)
		{
			if(*ptr == 0x00)
			{
				m_checksum.add(0x00);
				RETURN_IF_ERROR(finishBlock(m_code));
			}
			else
			{
				RETURN_IF_ERROR(writeAndChecksum(*ptr));
				if(++m_code == 0xFF)
					RETURN_IF_ERROR(finishBlock(m_code));
			}

			ptr++;
		}

		return true;
	}

	bool endEnvelope()
	{
		typename ChecksumGenerator::SumType sum = m_checksum.value();

		// Write the checksum
		RETURN_IF_ERROR(write(&sum, sizeof(sum)));

		// Finish the last COBS block
		RETURN_IF_ERROR(finishBlock(m_code));

		// m_dstPtr points now past the last data byte plus an empty space
		// where the next COBS code would be.
		m_dstPtr--;

		// Append a zero (this starts the receive handler immediately)
		if(m_dstPtr == m_dstEnd)
			return false;
		*m_dstPtr++ = 0x00;

		m_writer->packetComplete(m_dstPtr - m_writer->dataPointer());

		return true;
	}

	template<class MSG>
	COBSWriter<ChecksumGenerator, WriterType>& operator<< (const MSG& msg)
	{
		if(!startEnvelope(MSG::MSG_CODE))
			return *this;

		if(!msg.serialize(this))
			return *this;

		if(!endEnvelope())
			return *this;

		return *this;
	}
private:
	//! Write byte and add it to the checksum
	bool writeAndChecksum(uint8_t c)
	{
		m_checksum.add(c);

		if(m_dstPtr == m_dstEnd)
			return false;

		*m_dstPtr++ = c;
		return true;
	}

	//! Finish the current COBS block
	bool finishBlock(uint8_t code)
	{
		*m_codePtr = code;

		if(m_dstPtr == m_dstEnd)
			return false;
		m_codePtr = m_dstPtr++;

		m_code = 0x01;

		return true;
	}

	WriterType* m_writer;
	ChecksumGenerator m_checksum;
	uint8_t m_code;
	uint8_t* m_codePtr;
	uint8_t* m_dstPtr;
	uint8_t* m_dstEnd;
};

template<class ChecksumGenerator, int MaxPacketSize>
class COBSReader
{
public:
	typedef typename IntForSize<MaxPacketSize>::Type SizeType;

	class Reader
	{
	public:
		Reader()
		{
		}

		Reader(COBSReader* envReader)
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
		COBSReader* m_envReader;
		SizeType m_idx;
	};

	friend class Reader;

	COBSReader()
	 : m_state(STATE_START)
	{
	}

	enum TakeResult
	{
		NEW_MESSAGE,
		NEED_MORE_DATA,
		CHECKSUM_ERROR,
		FRAME_ERROR
	};

	TakeResult take(uint8_t c)
	{
		switch(m_state)
		{
			case STATE_START:
				if(c == 0x00)
					m_state = STATE_MSG_CODE;
				break;
			case STATE_MSG_CODE:
				if(c != 0x00)
				{
					m_msgCode = c - 1;
					m_idx = 0;
					m_state = STATE_COBS_CODE;
				}
				break;
			case STATE_COBS_CODE:
				if(c == 0x00)
					return finish();

				if(c == 0x01)
				{
					if(m_idx == MaxPacketSize)
					{
						m_state = STATE_START;
						break;
					}

					m_buffer[m_idx++] = 0x00;
				}
				else
				{
					m_cobsCode = c;
					m_cobsLength = c - 1;
					m_state = STATE_COBS_DATA;
				}
				break;
			case STATE_COBS_DATA:
				if(c == 0x00)
					return finish();

				if(m_idx == MaxPacketSize)
				{
					m_state = STATE_START;
					break;
				}

				m_buffer[m_idx++] = c;

				if(--m_cobsLength == 0)
				{
					if(m_cobsCode != 0xFF)
					{
						if(m_idx == MaxPacketSize)
						{
							m_state = STATE_START;
							break;
						}

						m_buffer[m_idx++] = 0x00;
					}

					m_state = STATE_COBS_CODE;
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
	COBSReader<ChecksumGenerator, MaxPacketSize>& operator>>(MSG& msg)
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
		STATE_START,
		STATE_MSG_CODE,
		STATE_COBS_CODE,
		STATE_COBS_DATA
	};

	TakeResult finish()
	{
		if(m_idx < sizeof(typename ChecksumGenerator::SumType) + 1)
		{
			m_state = STATE_START;
			return FRAME_ERROR; // Short packet
		}

		// Remove the trailing zero introduced by COBS
		m_idx--;

		// Check if the checksum matches
		m_generator.reset();
		m_generator.add(m_msgCode+1);

		for(SizeType i = 0; i < m_idx - sizeof(typename ChecksumGenerator::SumType); ++i)
			m_generator.add(m_buffer[i]);

		const typename ChecksumGenerator::SumType* sum =
			reinterpret_cast<typename ChecksumGenerator::SumType*>(
				&m_buffer[m_idx - sizeof(typename ChecksumGenerator::SumType)]
			);

		if(m_generator.value() != *sum)
		{
			m_state = STATE_START;
			return CHECKSUM_ERROR;
		}

		// Remove checksum
		m_idx -= sizeof(typename ChecksumGenerator::SumType);

		m_state = STATE_START;
		return NEW_MESSAGE;
	}

	uint8_t m_state;
	uint8_t m_msgCode;
	uint8_t m_buffer[MaxPacketSize];
	SizeType m_idx;
	uint8_t m_cobsCode;
	uint8_t m_cobsLength;

	ChecksumGenerator m_generator;
};

}

#endif
