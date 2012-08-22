
Introduction
============

libucomm aims to be a generic C++ library for serial communication
between space-constrained low-power devices such as microcontrollers.

Features
========

 - Small C++ header library, no linking required.
 - Optimizes away to no overhead in most cases
 - Very little assumptions about communication architecture
 - Flexible checksum system (add your own checksum method)
 - Default envelope system guarantees synchronization

Caveats
=======

 - Assumes endianness is the same. If you want to improve on this, it should
   be fairly easy to fix in parse.py.

Usage
=====

Protocol definition
-------------------

The messaging protocol is defined in a C-like format:

	struct USSensorData
	{
		uint8_t flags;
		uint16_t distance;
	};

	msg SensorDataMessage
	{
		uint8_t temperature;
		USSensorData sensors[];
	};

	msg Alert
	{
		uint8_t code;
	};

This file can be parsed by a small python script (parse.py), which creates a
C++ header ready for use in your application.

Sending data
------------

First some handy typedefs (you can see the flexibility here):

	typedef uc::EnvelopeWriter< uc::InvertedModSumGenerator > EnvelopeWriter;
	typedef uc::IO< EnvelopeWriter, uc::IO_W > SimpleWriter;
	typedef Proto< SimpleWriter > WProto;

Setting up I/O:

	class MyWriter : public uc::CharWriter
	{
	public:
		bool writeChar(uint8_t c)
		{
			// Send c away
			return true; // success!
		}
	};


	MyWriter writer;
	EnvelopeWriter output(&writer);

To send a simple message without arrays, just fill in the C structure and pass
it into the EnvelopeWriter:

	WProto::Alert alert;
	alert.code = 0xA5;
	output << alert;

Arrays require a bit more infrastructure, since we do not want to waste memory.
They can be pre-filled:

	WProto::SensorDataMessage msg;
	WProto::USSensorData sensors[5] = {...}; // Fill in some data
	msg.sensors.setData(sensors, sizeof(sensors));
	msg.temperature = 0; // brr..
	output << msg;

Or filled on the go while outputting the packet (requires least memory):

	template<class SizeType>
	bool fillInSensor(WProto::USSensorData* data, SizeType index)
	{
		data->distance = 4 * index; // Fake some distance data
		return true;
	}

	WProto::SensorDataMessage msg;
	// Create 5 elements and lazily call fillInSensor to get data when required
	msg.sensors.setCallback(&fillInSensor, 5);
	msg.temperature = 0; // brr..
	output << msg;

Reading data
------------

... is even more simple. First, some typedefs!

	typedef uc::EnvelopeReader< uc::InvertedModSumGenerator, 1024 > EnvelopeReader;
	typedef uc::IO< EnvelopeReader, uc::IO_R > SimpleReader;
	typedef Proto< SimpleReader > RProto;


	EnvelopeReader input;

	while(1)
	{
		uint8_t byte = ...; // Got some input!
		if(input.take(byte) == EnvelopeReader::NEW_MESSAGE)
		{
			// We got a message
			switch(input.msgCode())
			{
				case RProto::Alert::MSG_CODE:
				{
					RProto::Alert alert;
					input >> alert;
					printf("Got an alert with code %d\n", alert.code);
				}
					break;
				case RProto::SensorDataMessage::MSG_CODE:
				{
					RProto::SensorDataMessage msg;
					input >> msg;

					printf("Temperature: %d\n", msg.temperature);

					RProto::USSensorData sensor;
					while(msg.sensors.next(&sensor))
					{
						printf("Distance: %d\n", sensor.distance);
					}
				}
					break;
			}
		}
	}

Convinced?

TODO
====

 - Hide some "abstraction uglyness" like the typedefs away
 - Support for more basic types (floats?)
 - Strings
 - Real API documentation
 - Flexible list sizes (it's all there)
