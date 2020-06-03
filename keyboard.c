/* -------------------------------------------------------------------- */

#include <stdlib.h>
#include <conio.h>
#include <bios.h>
#include <dos.h>
#include <types.e>

#include "bios.e"

#include "keyboard.e"

/* -------------------------------------------------------------------- */

static uint16 UnGotKeyBuffer[ 2 ] = { 0, 0 };
static uint16 *UnGotKeys = UnGotKeyBuffer;
static void interrupt ( *OldCtrlBreakHandler )( void );
static void interrupt ( *OldCtrlCHandler )( void );
static void ( *CtrlBreakHandler)( void ) = NULL;

/* -------------------------------------------------------------------- */

static void interrupt LocalCtrlBreakHandler( void )

  {
    if ( CtrlBreakHandler != NULL )
      (*CtrlBreakHandler)();
  }

/* -------------------------------------------------------------------- */

void CloseKeyboard( void )

  {
    setvect( 0x1b, OldCtrlBreakHandler );
    setvect( 0x23, OldCtrlCHandler );
  }

/* -------------------------------------------------------------------- */

void InitKeyboard( void )

  {
    static bool FirstTime = True;

    if ( FirstTime )
      {
	FirstTime = False;
	OldCtrlBreakHandler = getvect( 0x1b );
	OldCtrlCHandler = getvect( 0x23 );
	atexit( CloseKeyboard );
      }
    setvect( 0x1b, LocalCtrlBreakHandler );
    setvect( 0x23, LocalCtrlBreakHandler );
  }

/* -------------------------------------------------------------------- */

uint16 KeyPressed( void )

  {
    register uint16 ZeroFlag;
    register uint16 Ch;

    if ( *UnGotKeys )
      return( *UnGotKeys );
    else
      {
	_AH = 1;
	BiosKey();  /* Keyboard services */
	ZeroFlag = _FLAGS;
	Ch = _AX;
	if ( ( ZeroFlag & ( 1 << 6 ) ) == 0 )   /* Zero Flag */
	  return( Ch ? Ch : KeyCtrlC );
	else
	  return( 0 );
      }
  }

/* -------------------------------------------------------------------- */

void UnGetCh( uint16 Key )

  {
    UnGotKeyBuffer[ 0 ] = Key;
    UnGotKeys = UnGotKeyBuffer;
  }

/* -------------------------------------------------------------------- */

void UnGetChs( uint16 *Keys )

  {
    UnGotKeys = Keys;
  }

/* -------------------------------------------------------------------- */

uint16 GetCh( void )

  {
    register uint16 Ch;

    if ( *UnGotKeys )
      return( *UnGotKeys++ );
    else
      {
	_AH = 0;
	BiosKey();
	Ch = _AX;
	if ( ( Ch & 0x00ff ) == 0 )
	  return( Ch ? Ch : KeyCtrlC );
	else
	  return( Ch & 0x00ff );
      }
  }

/* -------------------------------------------------------------------- */

uint16 GetChScan( void )

  {
    register uint16 Ch;

    if ( *UnGotKeys )
      return( *UnGotKeys++ );
    else
      {
	_AH = 0;
	BiosKey();
	Ch = _AX;
	return( Ch ? Ch : KeyCtrlC );
      }
  }

/* -------------------------------------------------------------------- */
