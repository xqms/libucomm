// Envelope format using COBS (consistent overhead byte stuffing)
// Author: Max Schwarz <max.schwarz@online.de>

#ifndef LIBUCOMM_COBS_ENVELOPE_H
#define LIBUCOMM_COBS_ENVELOPE_H

#include <cstring>

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

/**
 * @brief COBS envelope writer
 *
 * This class stuffs the packet payload using the COBS algorithm. Packets are
 * started and finished by zero bytes on the wire, which guarantees
 * synchronization.
 *
 * The underlying writer class must support the BufferedWriter interface, since
 * COBS needs to modify the COBS code bytes after the payload bytes have been
 * processed.
 **/
template<class ChecksumGenerator, class WriterType = BufferedWriter>
class COBSWriter
{
public:
    class Reader
    {
    };

    /**
     * @brief Constructor
     *
     * @param writer Pointer to the writer instance we will use to output packet
     *   bytes.
     **/
    COBSWriter(WriterType* writer);

    bool startEnvelope(uint8_t msg_code);

    //! Implement the IO writer interface
    bool write(const void* data, size_t size);

    bool endEnvelope(bool terminate = true);

    /**
     * @brief Write message
     *
     * Processes & outputs the message @a msg. This should be your main
     * interaction point with this class.
     *
     * @note For more options & error checking, use send().
     **/
    template<class MSG>
    COBSWriter<ChecksumGenerator, WriterType>& operator<< (const MSG& msg);

    /**
     * @brief Write message
     *
     * Processes & outputs the message @a msg. This should be your main
     * interaction point with this class.
     *
     * @param terminate If this is false, the system will not send the final
     *   zero byte. You can do this if you immediately follow up with the
     *   next packet.
     * @return true on success
     **/
    template<class MSG>
    bool send(const MSG& msg, bool terminate = true);
private:
    //! Write byte and add it to the checksum
    bool writeAndChecksum(uint8_t c);

    //! Finish the current COBS block
    bool finishBlock(uint8_t code);

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
        Reader();
        Reader(COBSReader* envReader);

        // Implement IO::Reader interface
        bool read(void* data, size_t size);
        bool skip(size_t size);
    private:
        COBSReader* m_envReader;
        SizeType m_idx;
    };

    friend class Reader;

    COBSReader();

    //! Possible take() return codes
    enum TakeResult
    {
        NEW_MESSAGE,      //!< New message available, use msgCode() + read()
        NEED_MORE_DATA,   //!< Message not yet finished (this is the default)
        CHECKSUM_ERROR,   //!< There was a checksum error in the current packet
        FRAME_ERROR       //!< The current packet was not correctly framed
    };

    /**
     * Handle a byte of wire data.
     *
     * @return Status code, see TakeResult.
     **/
    TakeResult take(uint8_t c);

    /**
     * If take() returned NEW_MESSAGE, you can use this method to query the
     * code of the last decoded message.
     **/
    uint8_t msgCode() const
    { return m_msgCode; }

    /**
     * Deserialize the last message. Take care to check msgCode()!
     *
     * @note Better use read() and check the return value.
     **/
    template<class MSG>
    COBSReader<ChecksumGenerator, MaxPacketSize>& operator>>(MSG& msg)
    {
        Reader reader = makeReader();
        msg.deserialize(&reader);

        return *this;
    }

    /**
     * Deserialize the last message. Take care to check msgCode()!
     *
     * @return true on success.
     **/
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

    inline Reader makeReader()
    { return Reader(this); }

    //! Check if decoded data is a complete & valid packet
    TakeResult finish();

    uint8_t m_state;
    uint8_t m_msgCode;
    uint8_t m_buffer[MaxPacketSize];
    SizeType m_idx;
    uint8_t m_cobsCode;
    uint8_t m_cobsLength;

    ChecksumGenerator m_generator;
};

////////////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION

template<class ChecksumGenerator, class WriterType>
COBSWriter<ChecksumGenerator, WriterType>::COBSWriter(WriterType* writer)
 : m_writer(writer)
{
}

template<class ChecksumGenerator, class WriterType>
bool COBSWriter<ChecksumGenerator, WriterType>::startEnvelope(uint8_t msg_code)
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

template<class ChecksumGenerator, class WriterType>
bool COBSWriter<ChecksumGenerator, WriterType>::write(const void* data, size_t size)
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

template<class ChecksumGenerator, class WriterType>
bool COBSWriter<ChecksumGenerator, WriterType>::endEnvelope(bool terminate)
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
    if(terminate)
    {
        if(m_dstPtr == m_dstEnd)
            return false;
        *m_dstPtr++ = 0x00;
    }

    m_writer->packetComplete(m_dstPtr - m_writer->dataPointer());

    return true;
}

template<class ChecksumGenerator, class WriterType>
template<class MSG>
COBSWriter<ChecksumGenerator, WriterType>&
COBSWriter<ChecksumGenerator, WriterType>::operator<<(const MSG& msg)
{
    send(msg);
    return *this;
}

template<class ChecksumGenerator, class WriterType>
template<class MSG>
bool COBSWriter<ChecksumGenerator, WriterType>::send(const MSG& msg, bool terminate)
{
    RETURN_IF_ERROR(startEnvelope(MSG::MSG_CODE));
    RETURN_IF_ERROR(msg.serialize(this));
    RETURN_IF_ERROR(endEnvelope(terminate));

    return true;
}

template<class ChecksumGenerator, class WriterType>
bool COBSWriter<ChecksumGenerator, WriterType>::writeAndChecksum(uint8_t c)
{
    m_checksum.add(c);

    if(m_dstPtr == m_dstEnd)
        return false;

    *m_dstPtr++ = c;
    return true;
}

template<class ChecksumGenerator, class WriterType>
bool COBSWriter<ChecksumGenerator, WriterType>::finishBlock(uint8_t code)
{
    *m_codePtr = code;

    if(m_dstPtr == m_dstEnd)
        return false;
    m_codePtr = m_dstPtr++;

    m_code = 0x01;

    return true;
}

////////////////////////////////////////////////////////////////////////////////

template<class ChecksumGenerator, int MaxPacketSize>
COBSReader<ChecksumGenerator, MaxPacketSize>::Reader::Reader()
{
}

template<class ChecksumGenerator, int MaxPacketSize>
COBSReader<ChecksumGenerator, MaxPacketSize>::Reader::Reader(COBSReader* envReader)
 : m_envReader(envReader)
 , m_idx(0)
{
}

template<class ChecksumGenerator, int MaxPacketSize>
bool COBSReader<ChecksumGenerator, MaxPacketSize>::Reader::read(void* data, size_t size)
{
    if(m_idx + size > m_envReader->m_idx)
        return false;

    memcpy(data, m_envReader->m_buffer + m_idx, size);
    m_idx += size;

    return true;
}

template<class ChecksumGenerator, int MaxPacketSize>
bool COBSReader<ChecksumGenerator, MaxPacketSize>::Reader::skip(size_t size)
{
    if(m_idx + size > m_envReader->m_idx)
        return false;

    m_idx += size;

    return true;
}

template<class ChecksumGenerator, int MaxPacketSize>
COBSReader<ChecksumGenerator, MaxPacketSize>::COBSReader()
 : m_state(STATE_START)
{
}

template<class ChecksumGenerator, int MaxPacketSize>
typename COBSReader<ChecksumGenerator, MaxPacketSize>::TakeResult
COBSReader<ChecksumGenerator, MaxPacketSize>::take(uint8_t c)
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

template<class ChecksumGenerator, int MaxPacketSize>
typename COBSReader<ChecksumGenerator, MaxPacketSize>::TakeResult
COBSReader<ChecksumGenerator, MaxPacketSize>::finish()
{
    // Precondition: we just received a 0x00 byte. So the next state
    // *must* be STATE_MSG_CODE.

    if(m_idx < sizeof(typename ChecksumGenerator::SumType) + 1)
    {
        m_state = STATE_MSG_CODE;
        return FRAME_ERROR; // Short packet
    }

    // Remove the trailing zero introduced by COBS
    m_idx--;

    // Check if the checksum matches
    m_generator.reset();
    m_generator.add(m_msgCode+1);

    for(SizeType i = 0; i < m_idx - sizeof(typename ChecksumGenerator::SumType); ++i)
        m_generator.add(m_buffer[i]);

    typename ChecksumGenerator::SumType sum;
    std::memcpy(&sum, &m_buffer[m_idx - sizeof(typename ChecksumGenerator::SumType)], sizeof(sum));

    if(m_generator.value() != sum)
    {
        m_state = STATE_MSG_CODE;
        return CHECKSUM_ERROR;
    }

    // Remove checksum
    m_idx -= sizeof(typename ChecksumGenerator::SumType);

    m_state = STATE_MSG_CODE;
    return NEW_MESSAGE;
}

}

#endif
