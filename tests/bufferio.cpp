// Simple FIFO I/O
// Author: Max Schwarz <max@x-quadraht.de>

#include "bufferio.h"

#include <assert.h>

inline size_t nextPos(size_t pos, size_t size)
{
	return (pos+1) % size;
}

BufferIO::BufferIO(size_t size)
 : m_size(size)
 , m_writePos(0)
 , m_readPos(0)
{
	m_buffer = (uint8_t*)malloc(size);
}

BufferIO::~BufferIO()
{
	free(m_buffer);
}

bool BufferIO::writeChar(uint8_t c)
{
	size_t next = nextPos(m_writePos, m_size);

	if(next == m_readPos)
		return false; // Buffer full

	m_buffer[m_writePos] = c;
	m_writePos = next;
}

bool BufferIO::isCharAvailable()
{
	return m_writePos != m_readPos;
}

uint8_t BufferIO::getChar()
{
	assert(isCharAvailable());

	uint8_t c = m_buffer[m_readPos];
	m_readPos = nextPos(m_readPos, m_size);

	return c;
}
