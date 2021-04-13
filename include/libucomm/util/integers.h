// Integer templates
// Author: Max Schwarz <max@x-quadraht.de>

#ifndef LIBUCOMM_INTEGERS_H
#define LIBUCOMM_INTEGERS_H

namespace uc
{

template<int T>
class TypeForBytes;

template<>
class TypeForBytes<0>
{
public:
    typedef uint8_t Type;
};
template<>
class TypeForBytes<1>
{
public:
    typedef uint8_t Type;
};
template<>
class TypeForBytes<2>
{
public:
    typedef uint16_t Type;
};
template<>
class TypeForBytes<3>
{
public:
    typedef uint32_t Type;
};
template<>
class TypeForBytes<4>
{
public:
    typedef uint32_t Type;
};

template<int Size, int Acc>
class DoIntForSize
{
public:
    typedef typename DoIntForSize<Size / 256, Acc+1>::Type Type;
};

template<int Acc>
class DoIntForSize<0, Acc>
{
public:
    typedef typename TypeForBytes<Acc>::Type Type;
};

template<int Size>
class IntForSize
{
public:
    typedef typename DoIntForSize<Size, 0>::Type Type;
};

}

#endif
