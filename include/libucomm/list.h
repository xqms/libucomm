// List with variable length
// Author: Max Schwarz <max@x-quadraht.de>

#ifndef LIBUCOMM_LIST_H
#define LIBUCOMM_LIST_H

#include <type_traits>

#include <stdint.h>
#include "io.h"
#include "util/integers.h"
#include "util/enable_if.h"
#include "util/error.h"

namespace uc
{

template<class IOI, class T, int Size=255, class Enable=void>
class List
{
public:
    typedef typename IntForSize<Size>::Type SizeType;
    typedef bool (*Callback)(T* dest, SizeType idx);

    bool next(T* dest);
    void setData(T* data, SizeType size);
    void setCallback(Callback cb, SizeType size);
};

template<class IOI, class T, int Size>
class List<IOI, T, Size, typename enable_if<IOI::IO::Mode::IsReadable>::Type >
{
public:
    typedef typename IntForSize<Size>::Type SizeType;

    inline SizeType remaining() const
    { return m_count; }

    bool deserialize(typename IOI::IO::Reader* reader)
    {
        RETURN_IF_ERROR(reader->read(&m_count, sizeof(m_count)));

        // Save starting point for element access
        m_reader = *reader;

        if(!IOI::IsLast)
        {
            if constexpr(std::is_integral_v<T>)
            {
                RETURN_IF_ERROR(reader->skip(sizeof(T)*m_count));
            }
            else
            {
                for(SizeType i = m_count; i; --i)
                    RETURN_IF_ERROR(reader->skip(T::POD_SIZE));
            }
        }

        return true;
    };

    bool next(T* dest)
    {
        if(m_count == 0)
            return false;

        m_count--;

        if constexpr(std::is_integral_v<T>)
            return m_reader.read(dest, sizeof(T));
        else
            return dest->deserialize(&m_reader);
    }
private:
    SizeType m_count;
    typename IOI::Reader m_reader;
};

template<class IOI, class T, int Size>
class List<IOI, T, Size, typename enable_if<IOI::IO::Mode::IsWritable>::Type >
{
public:
    typedef typename IntForSize<Size>::Type SizeType;
    typedef bool (*Callback)(T* dest, SizeType idx);

    List()
     : m_mode(MODE_EMPTY)
    {}

    inline void setData(T* data, SizeType count)
    {
        m_mode = MODE_DIRECT_DATA;
        m_data = data;
        m_count = count;
    }

    inline void setCallback(Callback callback, SizeType count)
    {
        m_mode = MODE_CALLBACK;
        m_callback = callback;
        m_count = count;
    }

    inline bool serialize(typename IOI::IO::Handler* writer) const
    {
        RETURN_IF_ERROR(
            writer->write(&m_count, sizeof(m_count))
        );

        if(m_mode == MODE_DIRECT_DATA)
        {
            if constexpr (std::is_integral_v<T>)
            {
                RETURN_IF_ERROR(
                    writer->write(m_data, sizeof(T)*m_count)
                );
            }
            else
            {
                for(SizeType i = 0; i != m_count; ++i)
                    RETURN_IF_ERROR(m_data[i].serialize(writer));
            }
        }
        else if(m_mode == MODE_CALLBACK)
        {
            T buf;
            for(SizeType i = 0; i != m_count; ++i)
            {
                RETURN_IF_ERROR(m_callback(&buf, i));

                if constexpr (std::is_integral_v<T>)
                {
                    RETURN_IF_ERROR(
                        writer->write(&buf, sizeof(T))
                    );
                }
                else
                    RETURN_IF_ERROR(buf.serialize(writer));
            }
        }

        return true;
    }

private:
    enum Mode {
        MODE_EMPTY,
        MODE_DIRECT_DATA,
        MODE_CALLBACK
    };

    SizeType m_count;
    Mode m_mode;

    union
    {
        T* m_data;
        Callback m_callback;
    };
};

}

#endif
