// Checksum generators
// Author: Max Schwarz <max@x-quadraht.de>

#ifndef CHECKSUM_H
#define CHECKSUM_H

namespace uc
{

class ModSumGenerator
{
public:
	void add(uint8_t c)
	{
		m_value += c;
	}

	void reset()
	{
		m_value = 0;
	}

	uint8_t value() const
	{
		return m_value;
	}
private:
	uint8_t m_value;
};

class InvertedModSumGenerator
{
public:
	void add(uint8_t c)
	{
		m_value += c;
	}

	void reset()
	{
		m_value = 0;
	}

	uint8_t value() const
	{
		return ~m_value;
	}
private:
	uint8_t m_value;
};


}

#endif
