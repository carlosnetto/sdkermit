/* ------------------------------------------------------------------- */

#include <dos.h>
#include <types.e>

#include "bios.e"

/* ------------------------------------------------------------------- */

void BiosVideo( void )

  {
    static uint16 AX, SI, DI, BP;

    AX  = _AX;			/* Saving SI, DI and BP */
    _AX = _SI;
    SI  = _AX;
    _AX = _DI;
    DI  = _AX;
    _AX = _BP;
    BP  = _AX;
    _AX = AX;

    geninterrupt( 0x10 );	/* Calling BIOS */

     AX = _AX;			/* Restoring SI, DI and BP */
    _AX = SI;
    _SI = _AX;
    _AX = DI;
    _DI = _AX;
    _AX = BP;
    _BP = _AX;
    _AX = AX;
  }

/* ------------------------------------------------------------------- */

void BiosKey( void )

  {
    static uint16 AX, SI, DI, BP;

    AX  = _AX;			/* Saving SI, DI and BP */
    _AX = _SI;
    SI  = _AX;
    _AX = _DI;
    DI  = _AX;
    _AX = _BP;
    BP  = _AX;
    _AX = AX;

    geninterrupt( 0x16 );	/* Calling BIOS */

     AX = _AX;			/* Restoring SI, DI and BP */
    _AX = SI;
    _SI = _AX;
    _AX = DI;
    _DI = _AX;
    _AX = BP;
    _BP = _AX;
    _AX = AX;
  }

/* ------------------------------------------------------------------- */
