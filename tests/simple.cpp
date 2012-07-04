// Simple tests
// Author: Max Schwarz <max@x-quadraht.de>

#include <libucomm/envelope.h>
#include <libucomm/checksum.h>
#include <libucomm/io.h>

#include "simple.h"

#include "bufferio.h"
#include "asserts.h"

#include <stdio.h>

typedef uc::EnvelopeWriter<uc::InvertedModSumGenerator, BufferIO> EnvelopeWriter;

typedef uc::IO<EnvelopeWriter, uc::IO_W> SimpleWriter;
typedef Proto<SimpleWriter> WProto;

typedef uc::EnvelopeReader<uc::InvertedModSumGenerator, 1024> EnvelopeReader;
typedef uc::IO<EnvelopeReader, uc::IO_R> SimpleReader;
typedef Proto<SimpleReader> RProto;

template<class SizeType>
bool fillStruct(WProto::Struct* data, SizeType idx)
{
	data->index = idx;
	data->some_value = 5*idx;

	return true;
}

int main()
{
	WProto::Message pkt;
	pkt.flags = 0;
	pkt.list.setCallback(fillStruct, 4);

	BufferIO dbg;
	EnvelopeWriter output(&dbg);
	output << pkt << pkt;

	int packetCount = 0;

	EnvelopeReader input;
	while(dbg.isCharAvailable())
	{
		if(input.take(dbg.getChar()))
		{
			ASSERT_EQUAL(input.msgCode(), RProto::Message::MSG_CODE);

			RProto::Message pkt2;
			input >> pkt2;

			ASSERT_EQUAL(pkt2.flags, pkt.flags);

			RProto::Struct data;
			int i = 0;
			while(pkt2.list.next(&data))
			{
				ASSERT_EQUAL(data.index, i);
				ASSERT_EQUAL(data.some_value, 5*i);
				++i;
			}

			ASSERT_EQUAL(i, 4);

			packetCount++;
		}
	}

	ASSERT_EQUAL(packetCount, 2);
	return 0;
}
