// Test

#include "test.h"
#include <stdio.h>

class SimpleIOHandler
{
public:
	class Reader
	{
	};

	bool write(const void* data, size_t size)
	{
		printf("write:");

		for(size_t i = 0; i < size; ++i)
			printf(" %02X", ((const uint8_t*)data)[i]);
		printf("\n");

		return true;
	}
};

typedef uc::IO<SimpleIOHandler, uc::IO_W> SimpleWriter;
typedef Proto<SimpleWriter> WProto;

template<class SizeType>
bool fillServoCommands(WProto::ServoCommand* cmd, SizeType idx)
{
	cmd->id = idx;
	cmd->command = 2 * idx;

	return true;
}

int main()
{
	WProto::ServoPacket pkt;
	pkt.flags = 0;
	pkt.cmds.setCallback(fillServoCommands, 4);

	SimpleIOHandler output;
	pkt.serialize(&output);
}
