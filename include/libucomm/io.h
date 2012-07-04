// IO template definition
// Author: Max Schwarz <Max@x-quadraht.de>

#ifndef LIBUCOMM_IO_H
#define LIBUCOMM_IO_H

namespace uc
{

struct IO_W { enum { IsWritable = 1, IsReadable = 0 }; };
struct IO_R { enum { IsWritable = 0, IsReadable = 1 }; };

template<class _Handler, class _Mode>
class IO
{
public:
	typedef _Mode Mode;
	typedef _Handler Handler;
	typedef typename Handler::Reader Reader;
};

template<class _IO, bool _IsLast>
class IOInstance
{
public:
	typedef _IO IO;
	typedef typename IO::Reader Reader;
	enum { IsLast = _IsLast };
};

}

#endif //LIBUCOMM_IO_H
