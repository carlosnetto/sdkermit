/* ------------------------------------------------------------------- */

#include <dos.h>

#include "critical.e"

/* ------------------------------------------------------------------- */

static void interrupt ( *OldCriticalHandler )( void );

/* ------------------------------------------------------------------- */

static int CriticalHandler( int ErrVal, int Ax, int Bp, int Si )

  {
    if ( ( ( int ) _osmajor ) * 100 + _osminor >= 301 )  /* if DOS >= 3.1 */
      return( 3 );				/* Fail */
    else
      hardretn( -1 );		      /* else return -1 to application */
  }

/* ------------------------------------------------------------------- */

void InstallCriticalErrorHandler( void )

  {
    OldCriticalHandler = getvect( 0x24 );
    harderr( CriticalHandler );
  }

/* ------------------------------------------------------------------- */

void CloseCriticalErrorHandler( void )

  {
    setvect( 0x24, OldCriticalHandler );
  }

/* ------------------------------------------------------------------- */

