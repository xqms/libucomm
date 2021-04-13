// Checksum generators
// Author: Max Schwarz <max@x-quadraht.de>

#ifndef CHECKSUM_H
#define CHECKSUM_H

#include <stdint.h>

namespace uc
{

class ModSumGenerator
{
public:
    typedef uint8_t SumType;

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
    typedef uint8_t SumType;

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

/**
 * @brief Fletcher-16 checksum generator
 **/
class Fletcher16Generator
{
public:
    typedef uint16_t SumType;

    void reset()
    {
        m_sum1 = 0;
        m_sum2 = 0;
    }

    void add(uint8_t c)
    {
        m_sum1 = (m_sum1 + c) % 255;
        m_sum2 = (m_sum2 + m_sum1) % 255;
    }

    uint16_t value() const
    {
        return (m_sum2 << 8) | m_sum1;
    }
private:
    uint16_t m_sum1;
    uint16_t m_sum2;
};

}

#endif
