// Test

#include "test.h"
#include "envelope.h"
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

class DebugCharIO
{
public:
	bool writeChar(uint8_t c)
	{
		printf("write: 0x%02X\n", c);
		return true;
	}
};

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

typedef uc::EnvelopeWriter<DebugCharIO, ModSumGenerator> EnvelopeWriter;

typedef uc::IO<EnvelopeWriter, uc::IO_W> SimpleWriter;
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

	EnvelopeWriter output;
	output.startEnvelope(0);
	pkt.serialize(&output);
	output.endEnvelope();
}
