// Error handling macros
// Author: Max Schwarz <Max@x-quadraht.de>

#ifndef LIBUCOMM_ERROR_H
#define LIBUCOMM_ERROR_H

namespace uc
{

#if 1
#define RETURN_IF_ERROR(code) \
	do { if(!(code)) return false; } while (0)
#else
#define RETURN_IF_ERROR(code) code
#endif

}

#endif //LIBUCOMM_ERROR_H
