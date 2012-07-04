// Small collection of test macros
// Author: Max Schwarz <max@x-quadraht.de>

#ifndef ASSERTS_H
#define ASSERTS_H

#include <assert.h>
#include <iostream>

template<class X, class Y>
void doAssert(const X& firstVal, const Y& secondVal, const char* firstText, const char* secondText, const char* file, int line)
{
	if(firstVal != secondVal)
	{
		std::cerr << "Assertion " << firstText << " == " << secondText
			<< " failed in file " << file << " at line " << line << std::endl;
		std::cerr << "First expression evaluated to " << firstVal << std::endl;
		std::cerr << "Second expression evaluated to " << secondVal << std::endl;
		assert(0);
	}
}

#define ASSERT_EQUAL(first, second) \
	doAssert(first, second, #first, #second, __FILE__, __LINE__)

#endif
