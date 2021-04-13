// enable_if idiom
// Author: Max Schwarz <max@x-quadraht.de>

#ifndef LIBUCOMM_ENABLE_IF_H
#define LIBUCOMM_ENABLE_IF_H

namespace uc
{

template<bool, class T = void>
struct enable_if {};

template<class T>
struct enable_if<true, T>
{
    typedef T Type;
};

}

#endif
