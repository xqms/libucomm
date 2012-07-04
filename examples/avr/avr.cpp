// Small AVR example (target: ATmega32)
// Author: Max Schwarz <max@x-quadraht.de>

#include <avr/io.h>
#include <libucomm/envelope.h>
#include <libucomm/io.h>
#include <libucomm/checksum.h>
#include "test.h"

void uart_init()
{
  UCSRB = (1 << TXEN) | (1 << RXEN);
  UBRRH = 0; // 1Mbit
  UBRRL = 0;
}

void uart_putc(uint8_t c)
{
  while(!(UCSRA & (1 << UDRE)));
  UDR = c;
}

class UARTWriter
{
public:
  void writeChar(uint8_t c)
  {
    uart_putc(c);
  }
};

typedef uc::EnvelopeWriter<uc::InvertedModSumGenerator, UARTWriter> EnvelopeWriter;

typedef uc::IO<EnvelopeWriter, uc::IO_W> SimpleWriter;
typedef Proto<SimpleWriter> WProto;

typedef uc::EnvelopeReader<uc::InvertedModSumGenerator, 1024> EnvelopeReader;
typedef uc::IO<EnvelopeReader, uc::IO_R> SimpleReader;
typedef Proto<SimpleReader> RProto;

const int NUM_SERVOS = 10;

bool getServoFeedback(WProto::ServoStatus* data, uint8_t index)
{
  data->position = 2*index;
  return true;
}

int main()
{
  uart_init();
  UARTWriter uart;
  EnvelopeWriter output(&uart);
  
  while(1)
  {
    WProto::ServoStatusPacket pkt;
    pkt.servos.setCallback(getServoFeedback, NUM_SERVOS);
    
    output << pkt;
  }  
}