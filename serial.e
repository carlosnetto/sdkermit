/* ------------------------------------------------------------------- */

#ifndef _SERIAL_E_
#define _SERIAL_E_

/* ------------------------------------------------------------------- */

#ifndef _TYPES_E_
#include <types.e>
#endif

/* ------------------------------------------------------------------- */

enum eBaudRate
  {
    B300    = 384,
    B600    = 192,
    B1200   = 96,
    B2400   = 48,
    B4800   = 24,
    B9600   = 12,
    B19200  = 6,
    B38400  = 3,
    B57600  = 2
  };

enum ePort
  {
    COM1   = 0x3f8,
    COM2   = 0x2f8
  };

enum eParity
  {
    None,
    Even,
    Odd
  };

typedef struct
  {
    enum eBaudRate BaudRate;
    enum eParity Parity;
    enum ePort Port;
    bool XonXoff;
    int8 StopBits;  /* 1,2 */
    int8 Size;  /* 7,8 */
    uint16 BufferSize;
  } tSerialDef;


/* ------------------------------------------------------------------- */

void OpenSerial( tSerialDef *SerialDef );
void CloseSerial( void );
uint16 ReadSerial( uint8 *DestBuffer, uint16 Bytes );
void PutSerialCh( char Ch );

/* ------------------------------------------------------------------- */

#endif

/* ------------------------------------------------------------------- */
