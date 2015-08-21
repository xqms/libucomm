// Base class for low-level output
// Author: Max Schwarz <max.schwarz@online.de>

#ifndef LIBUCOMM_WRITER_H
#define LIBUCOMM_WRITER_H

#include <stdint.h>

namespace uc
{

/**
 * @brief Base class for low-level output
 *
 * You *may* subclass this class to implement your stream output method
 * (e.g. serial output). You can also specify another class type in the
 * envelope template (e.g. EnvelopeWriter) to avoid the overhead introduced
 * by the virtual function calls.
 **/
class CharWriter
{
public:
	/**
	 * Write a single character.
	 *
	 * @return true on success
	 **/
	virtual bool writeChar(uint8_t c) = 0;

	/**
	 * Called when a packet is complete. You may want to buffer the byte writes
	 * until this method is called, if your underlying system supports batch
	 * writes.
	 **/
	virtual void flush() = 0;
};

class BufferedWriter
{
public:
	typedef size_t SizeType;

	virtual uint8_t* dataPointer() = 0;
	virtual SizeType dataSize() const = 0;

	virtual void packetComplete(SizeType n);

};

}

#endif
