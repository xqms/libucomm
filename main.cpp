// Test

#include "test.h"
#include "envelope.h"
#include "checksum.h"
#include <stdio.h>

const int BUFSIZE = 1024;

class DebugCharIO : public uc::CharWriter
{
public:
	DebugCharIO()
	 : m_readIdx(0)
	 , m_writeIdx(0)
	{
	}

	bool writeChar(uint8_t c)
	{
		m_buffer[m_writeIdx++] = c;
		// FIXME: Range check
		return true;
	}

	bool isDataAvailable() const
	{
		return m_writeIdx != m_readIdx;
	}

	uint8_t getc()
	{
		// FIXME: Range check
		return m_buffer[m_readIdx++];
	}
private:
	uint8_t m_buffer[BUFSIZE];
	uint8_t m_readIdx;
	uint8_t m_writeIdx;
};

typedef uc::EnvelopeWriter<uc::InvertedModSumGenerator> EnvelopeWriter;

typedef uc::IO<EnvelopeWriter, uc::IO_W> SimpleWriter;
typedef Proto<SimpleWriter> WProto;

typedef uc::EnvelopeReader<uc::InvertedModSumGenerator, 1024> EnvelopeReader;
typedef uc::IO<EnvelopeReader, uc::IO_R> SimpleReader;
typedef Proto<SimpleReader> RProto;

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

	DebugCharIO dbg;
	EnvelopeWriter output(&dbg);
	output << pkt << pkt;

	EnvelopeReader input;
	while(dbg.isDataAvailable())
	{
		if(input.take(dbg.getc()))
		{
			if(input.msgCode() == RProto::ServoPacket::MSG_CODE)
			{
				RProto::ServoPacket pkt2;
				input >> pkt2;

				printf("Flags: %d\n", pkt2.flags);
				RProto::ServoCommand cmd;
				while(pkt2.cmds.next(&cmd))
				{
					printf("cmd: %d -> %d\n", cmd.id, cmd.command);
				}
			}
		}
	}
}
