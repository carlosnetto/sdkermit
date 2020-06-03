/* ------------------------------------------------------------------- */

#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <process.h>
#include <ctype.h>
#include <dir.h>

#include <types.e>
#include <keyboard.e>
#include <malloc.e>
#include <readstr.e>
#include <writemsg.e>
#include <menutree.e>
#include <scrker.e>
#include <expfile.e>
#include <strtool.e>

#include "serial.e"
#include "config.e"
#include "cfgcomm.e"
#include "cfgterm.e"
#include "cfgscr.e"
#include "protocol.e"
#include "expfile.e"
#include "terminal.e"
#include "critical.e"

/* ------------------------------------------------------------------- */

struct
  {
    char Key[ 15 ];
    long int No;
  } SerialNumber = { "A0Z1XPUGJRQPOT", 909090L };

/* ------------------------------------------------------------------- */

void main( int Argc, char *Argv[] )

  {
    static char DirName[ 70 ];
    static char SendFileName[ 70 ];
    static char GetFileName[ 70 ];

    InitAll( Argc, Argv );

    while ( 1 )
      {
	switch( MenuTree() )
	  {
	    case 1 :
#ifdef DEMO
	      WriteMsg( 5, 4, "Versao do Programa", "SDKermit V1.1 - Demonstracao" );
#else
	      WriteMsg( 5, 4, "Versao do Programa", "SDKermit V1.1  -  %07.7ld", SerialNumber.No );
#endif
	      break;

	    case 2 :  /* Chdir */
	      {
		char *ColonPtr;
		char *DestDir;

		getcwd( DirName, sizeof( DirName ) );
		if ( ! ReadBoxStr( "Novo Diretorio", DirName, sizeof( DirName ) - 2, 5, 5, 45 ) )
		  break;
		DestDir = LeftTrim( DirName );
		if ( *DestDir == '\0' )
		  break;
		if ( ( ColonPtr = strchr( DestDir, ':' ) ) != NULL )
		  {
		    if ( ColonPtr != DestDir && isalpha( ColonPtr[ -1 ] ) && ColonPtr[ 1 ] == '\0' )
		      strcat( DestDir, "." );
		  }
		if ( chdir( DestDir ) != 0 )
		  WriteMsg( 5, 5, "Erro", "\"%.65s\" nao existe", DestDir );
		else if ( ColonPtr != NULL )
		  setdisk( toupper( ColonPtr[ -1 ] ) - 'A' );
	      }
	      break;

	    case 3 :   /* Branch to Shell */
	      {
		void *Screen;
		char *Comspec = getenv( "COMSPEC" );
		int RetValue;

		if ( Comspec == NULL )
		  WriteMsg( 5, 6, "Erro", "Interpretador de comandos nao encontrado" );
		else
		  {
		    Screen = malloc( 0x1000 );
		    if ( Screen == NULL )
		      WriteMsg( 5, 6, "Erro", "Memoria insuficiente" );
		    else
		      {
			gettext( 1, 1, 80, 25, Screen );
			textattr( 7 );  /* The same attribute used by command.com */
			clrscr();
			RestoreCursor( OriginalCursor );
			CloseSerial();
			CloseKeyboard();
			CloseCriticalErrorHandler();
			RetValue = spawnl( P_WAIT, Comspec, Comspec, NULL );
			PutAttrStr( "#0" );
			TurnOffCursor();
			puttext( 1, 1, 80, 25, Screen );
			free( Screen );
			InstallCriticalErrorHandler();
			InitKeyboard();
			OpenSerial( &ProgCfg.SerialDef );
			if ( RetValue == -1 )
			  WriteMsg( 5, 6, "Erro", "Interpretador de comandos nao encontrado" );
		      }
                  }
	      }
              break;

	    case 4 :
	      exit( 0 );

	    case 5 :
	      UnGetCh( Connect() );
	      break;

	    case 6 :   /* Send Files */
              {
                int NoOfFiles;
                char **Files;

		if ( ! ReadBoxStr( "Nome do Arquivo", SendFileName, sizeof( SendFileName ), 26, 4, 45 ) )
		  break;
		if ( *LeftTrim( SendFileName ) == '\0' )
		  break;
		if ( ( Files = ExpandFile( SendFileName, &NoOfFiles ) ) == NULL )
                  {
		    WriteMsg( 26, 4, "Erro", "\"%.70s\" nao existe", SendFileName );
                    break;
                  }
		SendFiles( NoOfFiles, Files, False );
                do
		  free( Files[ --NoOfFiles ] );
                while( NoOfFiles != 0 );
                free( Files );
              }
              break;

	    case 7 :
	      ReceiveFiles( NULL, False );
	      break;

	    case 8 :
	      if ( ReadBoxStr( "Nome do Arquivo", GetFileName, sizeof( GetFileName ), 26, 6, 45 ) )
		if ( *LeftTrim( GetFileName ) != '\0' )
		  ReceiveFiles( GetFileName, False );
	      break;

	    case 17 :
	      Server();
	      break;

	    case 9 :
	      if ( Finish() )
		WriteMsg( 26, 8, "Aviso", "Servidor finalizado" );
	      else
		WriteMsg( 26, 8, "Erro", "Servidor nao encontrado" );
	      break;

	    case 10 :
	      if ( Logout() )
		WriteMsg( 26, 9, "Aviso", "Sessao finalizada" );
	      else
		WriteMsg( 26, 9, "Erro", "Servidor nao encontrado" );
	      break;

	    case 13 :  /* Terminal */
              ConfigTerminal();
	      break;

	    case 14 :   /* Options for Communications */
	      ConfigSerial();
	      break;

	    case 15 :   /* Options for screen */
              ConfigScreen();
	      break;

	    case 16 :   /* Save Options */
	      {
		char *Name;
		int X1, X2;

		if ( ( Name = SaveConfig() ) != NULL )
		  {
		    X1 = 35;
		    X2 = X1 + 15 + strlen( Name );  /* 15 == strlen( "Salva em " ) + espacos para janela */
		    if ( X2 > 79 )
		      X1 -= ( X2 - 79 );
		    if ( X1 < 1 )
		      X1 = 1;
		    WriteMsg( X1, 7, "Aviso", "Salva em %s", Name );
		  }
		else
		  WriteMsg( 35, 7, "Erro", "Configuracao nao pode ser salva" );
		break;
	      }

	    default :
	      assert( False );
	  }
      }
  }

/* ------------------------------------------------------------------- */
