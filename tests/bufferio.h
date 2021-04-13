// Simple FIFO I/O
// Author: Max Schwarz <max@x-quadraht.de>

#ifndef BUFFERIO_H
#define BUFFERIO_H

#include <stdlib.h>
#include <stdint.h>

class BufferIO
{
public:
    BufferIO(size_t size = 1024);
    virtual ~BufferIO();

    // Implement interface
    bool writeChar(uint8_t c);

    // Implement BufferedWriter interface
    uint8_t* dataPointer();
    size_t dataSize() const;
    void packetComplete(size_t n);

    // Read methods
    uint8_t getChar();
    bool isCharAvailable();

private:
    size_t m_size;
    size_t m_writePos;
    size_t m_readPos;
    uint8_t* m_buffer;
};

#endif
