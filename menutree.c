/* ------------------------------------------------------------------- */

#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <assert.h>

#include <keyboard.e>
#include <types.e>
#include <scrker.e>
#include <newmenu.e>

#include "menutree.e"

/* ------------------------------------------------------------------- */

static char *MainOpts[] = { "Sistema",
			    "Terminal",
			    "Kermit",
			    "Configuracao",
			    "" };

static char *FileOpts[] = { "Versao",
			    "Muda dir",
			    "Dos",
			    "Sair",
			    "" };

static char *KermitOpts[] = { "Envia",
			      "Recebe",
			      "Busca",
			      "Servidor",
			      "Fim servidor",
			      "fIm sessao",
			      "" };

static char *OptionsOpts[] = { "Terminal",
			       "Comunicacao",
			       "Video",
			       "Salvar",
			       "" };

/* ------------------------------------------------------------------- */

static uint16 ExitKeysH[] = { KeyRight, KeyLeft, KeyAltS, KeyAltT, KeyAltK, KeyAltC, KeyHome, KeyEnd, 0 };
static uint16 ExitKeysV[] = { KeyRight, KeyLeft, KeyAltS, KeyAltT, KeyAltK, KeyAltC, 0 };

static tMenu MenuTab[] =

  {
    {
      /* 0 - Main */
      "",
      1, 1, 80, 0,
      0,
      MenuHorizontal,
      MainOpts,
      1,
      ExitKeysH,
      NULL
    },
    {
      /* 1 - File */
      "",
      3, 2, 0, 0,
      1,
      0,
      FileOpts,
      1,
      ExitKeysV,
      NULL
    },
    {
      /* 2 - Kermit */
      "",
      24, 2, 0, 0,
      1,
      0,
      KermitOpts,
      1,
      ExitKeysV,
      NULL
    },
    {
      /* 3 - Options */
      "",
      33, 2, 0, 0,
      1,
      0,
      OptionsOpts,
      1,
      ExitKeysV,
      NULL
    }
  };

/* ------------------------------------------------------------------- */

static int TreeTab[ 5 ][ 10 ] = {  { 0, 1, -5, 2, 3 },
				   { 0, -1, -2, -3, -4 },
				   { 0, -6, -7, -8, -17, -9, -10 },
				   { 0, -13, -14, -15, -16 } };

/* ------------------------------------------------------------------- */

bool TreatAlts( uint16 Key )

  {
    switch ( Key )
      {
	case KeyAltS :
	  UnGetCh( 'S' );
	  return ( True );

	case KeyAltT :
	  UnGetCh( 'T' );
	  return ( True );

	case KeyAltK :
	  UnGetCh( 'K' );
	  return ( True );

	case KeyAltC :
	  UnGetCh( 'C' );
	  return ( True );
      }
    return ( False );
  }

/* ------------------------------------------------------------------- */

int MenuTree( void )

  {
    static int CurrMenu = 0;
    static bool ShowSoon = False;
    int Opt;

    while ( 1 )
      {
	Opt = ReadMenu( &MenuTab[ CurrMenu ] );
	switch ( Opt )
	  {
	    case -1 :  /* Some exit key was pressed */
	      if ( TreatAlts( MenuTab[ CurrMenu ].LastKey ) )
		{
		  while ( CurrMenu )
		    {
		      EraseMenu( &MenuTab[ CurrMenu ] );
		      CurrMenu = TreeTab[ CurrMenu ][ 0 ];
		    }
		}
	      else if ( CurrMenu == 0 )
		{
		  if ( ShowSoon )
		    {
		      CurrMenu = TreeTab[ 0 ][ MenuTab[ 0 ].CurrentOption ];
		      if ( CurrMenu < 0 )   /* This option is a leaf */
			CurrMenu = 0;
		    }
                }
	      else if ( TreeTab[ CurrMenu ][ 0 ] == 0 )
		{
		  UnGetCh( MenuTab[ CurrMenu ].LastKey );
		  EraseMenu( &MenuTab[ CurrMenu ] );
		  CurrMenu = 0;
		}
	      break;

	    case 0 :   /* ESC key was pressed */
	      if ( CurrMenu )
		{
		  ShowSoon = False;
		  EraseMenu( &MenuTab[ CurrMenu ] );
		  CurrMenu = TreeTab[ CurrMenu ][ 0 ];
		}
	      else
		RedrawMenu( &MenuTab[ TreeTab[ 0 ][ 0 ] ] );
	      break;

	    default :  /* Some option was selected */
	      Opt = TreeTab[ CurrMenu ][ Opt ];
	      if ( Opt < 0 )
		return( -Opt );
	      else
		{
		  CurrMenu = Opt;
		  ShowSoon = True;
		}
          }
      }
  }

/* ------------------------------------------------------------------- */
