/* ------------------------------------------------------------------- */

#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <assert.h>

#include <types.e>
#include <malloc.e>
#include <scrker.e>
#include <keyboard.e>
#include <newmenu.e>

#include "config.e"
#include "cfgscr.e"

/* ------------------------------------------------------------------- */

static char *ScrOpts[] = { "Normal",
			   "Ressaltado",
			   "Invertido",
                           "" };

static tMenu ScrMenu = { "",
			 35, 6, 0, 0,
                         1,
                         0,
                         ScrOpts,
			 1,
			 NULL,
                         NULL };

static char *ColorOpts[] = { "Preto",
			     "aZul",
			     "vErde",
			     "Ciano",
			     "Vermelho",
			     "maGenta",
			     "Marrom",
			     "Branco",
                             "" };

static tMenu ColorMenu = { "",
			   37, 8, 0, 0,
                           1,
                           0,
                           ColorOpts,
			   1,
			   NULL,
                           NULL };



/* ------------------------------------------------------------------- */

/* Pattern window size definition */

#define Left 10
#define Upper 10
#define Right 25
#define Lower 20

/* ------------------------------------------------------------------- */

static void UpdatePatterns( void )

  {
    gotoxy( Left + 2, Upper + 2 );
    textattr( ProgCfg.ScreenDef.Normal );
    cprintf( "Normal      " );
    gotoxy( Left + 2, Upper + 4 );
    textattr( ProgCfg.ScreenDef.Highlight );
    cprintf( "Ressaltado  " );
    gotoxy( Left + 2, Upper + 6 );
    textattr( ProgCfg.ScreenDef.Reverse );
    cprintf( "Invertido   " );
    gotoxy( Left + 2, Upper + 8 );
    textattr( ProgCfg.ScreenDef.HighReverse );
    cprintf( "Inver. Ress." );
  }

/* ------------------------------------------------------------------- */

void ConfigScreen( void )

  {
    void *SavedScreen;
    int Opt;
    static uint16 KeysToRedraw[] = { KeyEsc, KeyEsc, 'C', 0 };

    SavedScreen = Malloc( ( Right - Left + 1 ) * ( Lower - Upper + 1 ) * 2 );
    gettext( Left, Upper, Right, Lower, SavedScreen );
    PutAttrStr( "#1" );
    ClearBox( Left, Upper, Right, Lower );
    DrawBox( Left, Upper, Right, Lower, 2 );

    UpdatePatterns();

    while ( ( Opt = ReadMenu( &ScrMenu ) ) != 0 )
      {
        switch ( Opt )
          {
            case 1 :  /* Normal */
	      ColorMenu.Ypos = 8;
	      ColorMenu.CurrentOption = ProgCfg.ScreenDef.Normal + 1;
              ReadMenu( &ColorMenu );
              EraseMenu( &ColorMenu );
              ProgCfg.ScreenDef.Normal = ColorMenu.CurrentOption - 1;
              break;

            case 2 :  /* Highlight */
	      ColorMenu.Ypos = 9;
	      ColorMenu.CurrentOption = ( ProgCfg.ScreenDef.Highlight & 0x7 ) + 1;
              ReadMenu( &ColorMenu );
              EraseMenu( &ColorMenu );
              ProgCfg.ScreenDef.Highlight = ( ColorMenu.CurrentOption - 1 ) | 0x8;
	      ProgCfg.ScreenDef.HighReverse &= 0xf0;
	      ProgCfg.ScreenDef.HighReverse |= ProgCfg.ScreenDef.Highlight;
              break;

            case 3 :  /* Reverse */
	      ColorMenu.Ypos = 10;
	      ColorMenu.CurrentOption = ( ProgCfg.ScreenDef.Reverse >> 4 ) + 1;
              ReadMenu( &ColorMenu );
              EraseMenu( &ColorMenu );
              ProgCfg.ScreenDef.Reverse = ( ColorMenu.CurrentOption - 1 ) << 4;
	      ProgCfg.ScreenDef.HighReverse &= 0x0f;
	      ProgCfg.ScreenDef.HighReverse |= ProgCfg.ScreenDef.Reverse;
              break;

            default :
              assert( False );
          }
        UpdatePatterns();
      }
    EraseMenu( &ScrMenu );
    puttext( Left, Upper, Right, Lower, SavedScreen );
    free( SavedScreen );

    SetAttrTab( ProgCfg.ScreenDef.Normal,
                ProgCfg.ScreenDef.Highlight,
                ProgCfg.ScreenDef.Reverse,
                ProgCfg.ScreenDef.HighReverse );
    UnGetChs( KeysToRedraw );
  }

/* ------------------------------------------------------------------- */
