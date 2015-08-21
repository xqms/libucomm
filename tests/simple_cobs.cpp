// Simple tests
// Author: Max Schwarz <max@x-quadraht.de>

#include <libucomm/cobs_envelope.h>
#include <libucomm/checksum.h>
#include <libucomm/io.h>

#include "catch.hpp"

#include "simple.h"

#include "bufferio.h"

#include <stdio.h>

typedef uc::Fletcher16Generator ChecksumGenerator;

typedef uc::COBSWriter<ChecksumGenerator, BufferIO> EnvelopeWriter;

typedef uc::IO<EnvelopeWriter, uc::IO_W> SimpleWriter;
typedef Proto<SimpleWriter> WProto;

typedef uc::COBSReader<ChecksumGenerator, 1024> EnvelopeReader;
typedef uc::IO<EnvelopeReader, uc::IO_R> SimpleReader;
typedef Proto<SimpleReader> RProto;

template<class SizeType>
bool fillStruct(WProto::Struct* data, SizeType idx)
{
	data->index = idx;
	data->some_value = 5*idx;

	return true;
}

TEST_CASE("simple_cobs", "[cobs]")
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
		EnvelopeReader::TakeResult ret = input.take(dbg.getChar());

		switch(ret)
		{
			case EnvelopeReader::NEW_MESSAGE:
			{
				REQUIRE(input.msgCode() == RProto::Message::MSG_CODE);

				RProto::Message pkt2;
				input >> pkt2;

				REQUIRE(pkt2.flags == pkt.flags);

				RProto::Struct data;
				int i = 0;
				while(pkt2.list.next(&data))
				{
					REQUIRE(data.index == i);
					REQUIRE(data.some_value == 5*i);
					++i;
				}

				REQUIRE(i == 4);

				packetCount++;
				break;
			}
			case EnvelopeReader::NEED_MORE_DATA:
				break;
			case EnvelopeReader::CHECKSUM_ERROR:
				FAIL("Checksum error");
				break;
			case EnvelopeReader::FRAME_ERROR:
				FAIL("Frame error");
				break;
			default:
				FAIL("Unknown error");
				break;
		}
	}

	REQUIRE(packetCount == 2);
}

TEST_CASE("corrupt_cobs", "[cobs]")
{
	WProto::Message pkt;
	pkt.flags = 0;
	pkt.list.setCallback(fillStruct, 4);

	BufferIO dbg;
	EnvelopeWriter output(&dbg);
	output << pkt;

	int checksumErrors = 0;

	EnvelopeReader input;
	int count = 0;

	while(dbg.isCharAvailable())
	{
		uint8_t byte = dbg.getChar();

		if(++count == 5)
			byte |= (1 << 3);

		EnvelopeReader::TakeResult ret = input.take(byte);

		switch(ret)
		{
			case EnvelopeReader::NEW_MESSAGE:
				FAIL("Got message even though it was corrupted");
				break;
			case EnvelopeReader::NEED_MORE_DATA:
				break;
			case EnvelopeReader::CHECKSUM_ERROR:
				checksumErrors++;
				break;
			case EnvelopeReader::FRAME_ERROR:
				FAIL("Frame error");
				break;
			default:
				FAIL("Unknown error");
				break;
		}
	}

	REQUIRE(checksumErrors == 1);
}
