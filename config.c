/* ------------------------------------------------------------------- */

#include <stdio.h>
#include <conio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <dir.h>

#include <types.e>
#include <writemsg.e>
#include <scrker.e>
#include <critical.e>
#include <malloc.e>
#include <strtool.e>
#include <keyboard.e>

#include "terminal.e"
#include "serial.e"
#include "config.e"

/* ------------------------------------------------------------------- */

tProgramConfig ProgCfg =
  {
    "SD-Kermit",
    -1L,
    { B9600, None, COM1, False, 1, 8, BufSize },
    { TermVT100, CursorBlock, False, True, False },
    { 0x07, 0x0f, 0x60, 0x6f }
  };

uint16 OriginalCursor;
static char *ThisProgramName;

/* ------------------------------------------------------------------- */

static void NoMemoryHandler( void )

  {
    WriteMsg( 10, 10, "Erro", "Falta memoria para executar o SDKermit" );
    exit( 1 );
  }

/* ------------------------------------------------------------------- */

static void FinishKermit( void )

  {
    RestoreCursor( OriginalCursor );
    CloseSerial();
    textattr( 7 );  /* The same attribute used by "cls" */
    clrscr();
  }

/* ------------------------------------------------------------------- */

void InitAll( int Argc, char *Argv[] )

  {
    int I;

    if ( ( ThisProgramName = Argv[ 0 ] ) == NULL )   /* Argv[ 0 ] works only for DOS 3.x */
      ThisProgramName = searchpath( "SDKER.EXE" );
    if ( ThisProgramName != NULL )
      ThisProgramName = StrSave( ThisProgramName );

    directvideo = 1;
    for ( I = 1; I < Argc; I++ )
      if ( strchr( "-/", Argv[ I ][ 0 ] ) != NULL && strchr( "wW", Argv[ I ][ 1 ] ) != NULL )
	directvideo = 0;

    atexit( FinishKermit );

    OriginalCursor = TurnOffCursor();

    SetNoMemoryHandler( NoMemoryHandler );

    SetAttrTab( ProgCfg.ScreenDef.Normal,
                ProgCfg.ScreenDef.Highlight,
                ProgCfg.ScreenDef.Reverse,
                ProgCfg.ScreenDef.HighReverse );
    PutAttrStr( "#1" );
    InitKeyboard();
    InstallCriticalErrorHandler();
    OpenSerial( &ProgCfg.SerialDef );
    InitTerminal( &ProgCfg.TermDef );
  }

/* ------------------------------------------------------------------- */

char *SaveConfig( void )

  {
    char *Key = ProgCfg.Key;
    int I, C;
    FILE *ExeFile;

    if ( ThisProgramName == NULL )
      return( NULL );
    ExeFile = fopen( ThisProgramName, "r+b" );
    if ( ExeFile == NULL )
      return( NULL );
    if ( ProgCfg.Position == -1L )
      {
	I = 0;
	C = getc( ExeFile );
	while ( C != EOF && Key[ I ] )
	  {
	    if ( C == Key[ I ] )
	      I++;
	    else
	      I = ( C == Key[ 0 ] ) ? 1 : 0;
	    C = getc( ExeFile );
	  }
        if ( Key[ I ] )
	  return( NULL );
	ProgCfg.Position = ftell( ExeFile ) - I - 1;
      }
    fseek( ExeFile, ProgCfg.Position, SEEK_SET );
    fwrite( &ProgCfg, sizeof( ProgCfg ), 1, ExeFile );
    I = fclose( ExeFile );
    return( I != EOF ? ThisProgramName : NULL );
  }

/* ------------------------------------------------------------------- */
