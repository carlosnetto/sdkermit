/* ------------------------------------------------------------------- */

#include <stddef.h>
#include <dos.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <types.e>
#include <malloc.e>
#include <scrker.e>
#include <keyboard.e>

#include "serial.e"

/* ------------------------------------------------------------------- */

/* 8259 Port Offsets */

#define Dll 0x00
#define Thr 0x00
#define Rbr 0x00
#define Dlm 0x01
#define Ier 0x01
#define Iir 0x02
#define Lcr 0x03
#define Mcr 0x04
#define Lsr 0x05
#define Msr 0x06

/* Masks */

#define Dr    0x01	/* Data Ready */
#define Thre  0x20      /* Transm Holding Reg. */

/* Control Chars */

#define CtrlS ( 'S' - '@' )
#define CtrlQ ( 'Q' - '@' )

/* Variables */

static uint8 *Buffer = NULL;
static uint16 InputIndex, OutputIndex, NumberOfChars;
static bool Xoff;
static tSerialDef Serial;
static int SerialInt;   /* Serial interrupt number */
static uint8 IntMask;
static void interrupt ( *OldHandler )( void );

/* Variables for demo version */

#ifdef DEMO

static uint16 TransmitedChars = 30000;

#endif

/* ------------------------------------------------------------------- */

#ifdef DEMO

void ShowDemoMessage( void )

  {
    TurnOffCursor();
    PutAttrStr( "#1" );
    ClearBox( 13, 5, 65, 18 );
    DrawBox( 13, 5, 65, 18, 3 );
    gotoxy( 14, 6 );
    PutAttrStr( "#2        SDKermit - Versao para Demonstracao        #1" );
    gotoxy( 16, 8 );
    PutAttrStr( "    Tempo de demonstracao esgotado! Se voce de-" );
    gotoxy( 16, 9 );
    PutAttrStr( "seja adquirir este produto, procure  nas  lojas" );
    gotoxy( 16, 10 );
    PutAttrStr( "especializadas ou diretamente com:" );
    gotoxy( 22, 12 );
    PutAttrStr( "Software Design Informatica" );
    gotoxy( 22, 13 );
    PutAttrStr( "R. Antonio Galizia, 27 - Cambui" );
    gotoxy( 22, 14 );
    PutAttrStr( "13.025              Campinas-SP" );
    gotoxy( 22, 15 );
    PutAttrStr( "Fone: (0192) 51-1153" );
    gotoxy( 26, 17 );
    PutAttrStr( "#3 Control-C retorna ao DOS #1" );
    while ( GetCh() != KeyCtrlC );
    exit( 0 );
  }

#endif

/* ------------------------------------------------------------------- */

void interrupt SerialHandler( void )

  {
    register uint8 Char;

    Char = inportb( Serial.Port + Rbr );
    if ( NumberOfChars != Serial.BufferSize )
      {
	NumberOfChars++;
	Buffer[ InputIndex++ ] = Char;
	if ( InputIndex == Serial.BufferSize )
	  InputIndex = 0;
      }
    outportb( 0x20, 0x20 );
  }

/* ------------------------------------------------------------------- */

void OpenSerial( tSerialDef *SerialDef )

  {
    register uint8 Byte;

    Serial = *SerialDef;

    assert( Buffer == NULL );

    Buffer = Malloc( Serial.BufferSize );
    Xoff = False;
    InputIndex = OutputIndex = NumberOfChars = 0;
    SerialInt = ( Serial.Port == COM1 ) ? 0x0c : 0x0b;
    IntMask = ( Serial.Port == COM1 ) ? 0xef : 0xf7;

    disable();

    OldHandler = getvect( SerialInt );
    setvect( SerialInt, SerialHandler );

    disable();	/* I don't know if setvect preserve interrupts */

    outportb( 0x21, IntMask & inportb( 0x21 ) );  /* Enable 8250 interrupts */
    outportb( Serial.Port + Lcr, 0x80 );          /* Making DLAB equal to one */
    outportb( Serial.Port + Dll, Serial.BaudRate & 0xff );
    outportb( Serial.Port + Dlm, Serial.BaudRate >> 8   );

    Byte = Serial.Size - 5;
    if ( Serial.StopBits == 2 )
      Byte |= 1 << 2;
    if ( Serial.Parity != None )
      {
	Byte |= 1 << 3;
	if ( Serial.Parity == Even )
	  Byte |= 1 << 4;
      }

    outportb( Serial.Port + Lcr, Byte );
    outportb( Serial.Port + Ier, 1 );	/* Enable 8250 interrupt when avaible character */
    outportb( Serial.Port + Mcr, 0x0b );

    enable();

    ( void ) inportb( Serial.Port + Rbr );
  }

/* ------------------------------------------------------------------- */

void CloseSerial( void )

  {
    if ( Buffer == NULL )    /* Enable many calls to CloseSerial */
      return;
    disable();
    outportb( 0x21, inportb( 0x21 ) | ( ~IntMask ) );
    setvect( SerialInt, OldHandler );
    free( Buffer );
    Buffer = NULL;
    enable();
  }

/* ------------------------------------------------------------------- */

uint16 ReadSerial( uint8 *DestBuffer, uint16 Bytes )

  {
    register uint16 MyNumberOfChars = NumberOfChars;
    register uint16 Slice;

    if ( Xoff && MyNumberOfChars < Serial.BufferSize / 2 )
      {
	Xoff = False;
	PutSerialCh( CtrlQ );
      }

    if ( MyNumberOfChars == 0 )
      return( 0 );

    if ( Serial.XonXoff && MyNumberOfChars > ( 2 * Serial.BufferSize ) / 3 )
      {
	Xoff = True;
	PutSerialCh( CtrlS );
      }

    if ( MyNumberOfChars < Bytes )
      Bytes = MyNumberOfChars;

#ifdef DEMO
    if ( TransmitedChars == 0 )
      ShowDemoMessage();
    if ( Bytes >= TransmitedChars )
      TransmitedChars = 0;
    else
      TransmitedChars -= Bytes;
#endif

    if ( OutputIndex + Bytes <= Serial.BufferSize )
      {
	memcpy( DestBuffer, &( Buffer[ OutputIndex ] ), Bytes );
	OutputIndex += Bytes;
      }
    else if ( OutputIndex + Bytes == Serial.BufferSize )
      {
	memcpy( DestBuffer, &( Buffer[ OutputIndex ] ), Bytes );
	OutputIndex = 0;
      }
    else
      {
	Slice = Serial.BufferSize - OutputIndex;
	memcpy( DestBuffer, &( Buffer[ OutputIndex ] ), Slice );
	OutputIndex = Bytes - Slice;
	memcpy( &DestBuffer[ Slice ], Buffer, OutputIndex );
      }

    disable();
    NumberOfChars -= Bytes;
    enable();

    return( Bytes );
  }

/* ------------------------------------------------------------------- */

void PutSerialCh( char Ch )

  {
#ifdef DEMO
    if ( TransmitedChars == 0 )
      ShowDemoMessage();
    TransmitedChars--;
#endif

    while ( ( inportb( Serial.Port + Lsr ) & Thre ) == 0 );

    outportb( Serial.Port + Thr, Ch );
  }

/* ------------------------------------------------------------------- */
