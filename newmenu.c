/* ------------------------------------------------------------------- */

#include <stdio.h>
#include <conio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include <types.e>
#include <keyboard.e>
#include <malloc.e>
#include <scrker.e>

#include "newmenu.e"

/* ------------------------------------------------------------------- */

#define MaxOptions  15
#define MagicNumber 31233

/* ------------------------------------------------------------------- */

/* Internal Flags */

#define MenuDrawn 0x0001

/* ------------------------------------------------------------------- */

static void DrawOption( tMenu *Menu, int Option )

  {
    int Xpos, Ypos;
    char *NormalAttr, *CapitalAttr, *ChPtr;
    int Width;

    if ( Menu -> Flags & MenuHorizontal )
      {
	Ypos = 0;
	Xpos = Menu -> Xpos + 3;
	while ( Ypos < Option - 1 )
	  {
	    Xpos += strlen( Menu -> Options[ Ypos ] ) + 3;
	    Ypos++;
	  }
	Ypos = Menu -> Ypos;
      }
    else
      {
	Ypos = Menu -> Ypos + Option - 1;
	Xpos = Menu -> Xpos;
      }
    if ( Menu -> BoxKind )
      {
	Xpos++;
	Ypos++;
      }
    Menu -> Xopt = Xpos;
    Menu -> Yopt = Ypos;

    if ( Option == Menu -> CurrentOption )
      NormalAttr = CapitalAttr = "#1";
    else
      {
	NormalAttr = "#2";
	CapitalAttr = "#3";
      }

    gotoxy( Xpos, Ypos );
    PutAttrStr( NormalAttr );
    if ( ! ( Menu -> Flags & MenuHorizontal ) )
      PutAttrStr( " " );
    ChPtr = Menu -> Options[ Option - 1 ];
    Width = 0;
    while ( !isupper( *ChPtr ) )
      {
	PutCh( *ChPtr++ );
	Width++;
      }
    PutAttrStr( CapitalAttr );
    PutCh( *ChPtr++ );
    Width++;
    PutAttrStr( NormalAttr );
    while ( *ChPtr )
      {
	PutCh( *ChPtr++ );
	Width++;
      }

    if ( !( Menu -> Flags & MenuHorizontal ) )
      {
	if ( Menu -> BoxKind )
	  Width += 2;
	while ( ++Width < Menu -> Width )
	  PutCh( ' ' );
      }
  }

/* ------------------------------------------------------------------- */

static void DrawMenu( tMenu *Menu )

  {
    int Option;

    if ( ! ( Menu -> InternalFlags & MenuDrawn ) )
      {
	Menu -> InternalFlags |= MenuDrawn;
	gettext( Menu -> Xpos, Menu -> Ypos,
		 Menu -> Xpos + Menu -> Width - 1,
		 Menu -> Ypos + Menu -> Heigth - 1,
		 ( ( int * ) Menu -> MemToSave ) + 1 );
	PutAttrStr( "#2" );
	ClearBox( Menu -> Xpos, Menu -> Ypos,
		  Menu -> Xpos + Menu -> Width - 1,
		  Menu -> Ypos + Menu -> Heigth - 1 );
	if ( Menu -> BoxKind )
	  DrawBox( Menu -> Xpos, Menu -> Ypos,
		   Menu -> Xpos + Menu -> Width - 1,
		   Menu -> Ypos + Menu -> Heigth - 1,
		   Menu -> BoxKind == 1 ? 0 : 3 );
	for ( Option = 1; Option <= Menu -> LastOption; Option++ )
	  DrawOption( Menu, Option );
      }
  }

/* ------------------------------------------------------------------- */

static void InitMenu( tMenu *Menu )

  {
    int Width;
    int MaxWidth = 0;
    int TotWidth = 3;
    char OptKeys[ MaxOptions ], *OptStr;
    char **OptPtr = Menu -> Options;
    int Option;

    Menu -> InternalFlags = 0;

    Option = 0;
    while ( **OptPtr != '\0' )
      {
	assert( Option <= MaxOptions );
	OptStr = *OptPtr;
	Width = strlen( OptStr );
	TotWidth += ( 3 + Width );
	if ( Width > MaxWidth )
	  MaxWidth = Width;
	while ( *OptStr )
	  if ( isupper( *OptStr ) )
	    {
	      OptKeys[ Option ] = *OptStr;
	      break;
	    }
	  else
	    OptStr++;
	assert( *OptStr != '\0' );
	Option++;
	OptPtr++;
      }

    Menu -> LastOption = Option;
    OptKeys[ Option ] = '\0';
    Menu -> OptKeys = Malloc( Option + 1 );
    strcpy( Menu -> OptKeys, OptKeys );

    if ( Menu -> Width == 0 )
      {
	if ( Menu -> Flags & MenuHorizontal )
	  Menu -> Width = TotWidth;
	else
	  Menu -> Width = MaxWidth + 2;
	if ( Menu -> BoxKind )
	  Menu -> Width += 2;
      }

    if ( Menu -> Heigth == 0 )
      {
	if ( Menu -> Flags & MenuHorizontal )
	  Menu -> Heigth = 1;
	else
	  Menu -> Heigth = Option;
	if ( Menu -> BoxKind )
	  Menu -> Heigth += 2;
      }

    Menu -> MemToSave = Malloc( Menu -> Width * Menu -> Heigth * 2 + 2 );
    *( ( int * ) Menu -> MemToSave ) = MagicNumber;
    gettext( Menu -> Xpos, Menu -> Ypos,
	     Menu -> Xpos + Menu -> Width - 1,
	     Menu -> Ypos + Menu -> Heigth - 1,
	     ( ( int * ) Menu -> MemToSave ) + 1 );
  }

/* ------------------------------------------------------------------- */

static void DrawRule( void )

  {
  }

/* ------------------------------------------------------------------- */

static void EraseRule( void )

  {
  }

/* ------------------------------------------------------------------- */

static bool MustExit( register uint16 Key, register uint16 *ExitKeys )

  {
    if ( ExitKeys == NULL )
      return( False );
    while ( *ExitKeys )
      if ( Key == *ExitKeys++ )
	return( True );
    return( False );
  }

/* ------------------------------------------------------------------- */

int ReadMenu( tMenu *Menu )

  {
    int RetValue = -1;
    uint16 Key;
    char *KeyPos;
    char OldOption;

    if ( Menu -> MemToSave == NULL )
      InitMenu( Menu );

    DrawMenu( Menu );

    if ( Menu -> Flags & MenuDrawRule )
      DrawRule();

    while ( RetValue == -1 )
      {
	switch ( Key = GetCh() )
	  {
	    case KeyEsc :
	      RetValue = 0;
	      break;

	    case KeyRight :
	      if ( Menu -> Flags & MenuHorizontal )
		{
		  OldOption = Menu -> CurrentOption;
		  if ( Menu -> CurrentOption++ == Menu -> LastOption )
		    Menu -> CurrentOption = 1;
		  DrawOption( Menu, OldOption );
		  DrawOption( Menu, Menu -> CurrentOption );
		}
              break;

            case KeyLeft :
	      if ( Menu -> Flags & MenuHorizontal )
		{
		  OldOption = Menu -> CurrentOption;
		  if ( Menu -> CurrentOption-- == 1 )
		    Menu -> CurrentOption = Menu -> LastOption;
		  DrawOption( Menu, OldOption );
		  DrawOption( Menu, Menu -> CurrentOption );
		}
	      break;

	    case KeyDown :
	      if ( ! ( Menu -> Flags & MenuHorizontal ) )
		{
		  OldOption = Menu -> CurrentOption;
		  if ( Menu -> CurrentOption++ == Menu -> LastOption )
		    Menu -> CurrentOption = 1;
		  DrawOption( Menu, OldOption );
		  DrawOption( Menu, Menu -> CurrentOption );
		}
              break;

	    case KeyUp :
	      if ( ! ( Menu -> Flags & MenuHorizontal ) )
		{
		  OldOption = Menu -> CurrentOption;
		  if ( Menu -> CurrentOption-- == 1 )
		    Menu -> CurrentOption = Menu -> LastOption;
		  DrawOption( Menu, OldOption );
		  DrawOption( Menu, Menu -> CurrentOption );
		}
              break;

	    case KeyHome :
	      OldOption = Menu -> CurrentOption;
	      Menu -> CurrentOption = 1;
	      DrawOption( Menu, OldOption );
	      DrawOption( Menu, Menu -> CurrentOption );
              break;

	    case KeyEnd :
	      OldOption = Menu -> CurrentOption;
	      Menu -> CurrentOption = Menu -> LastOption;
	      DrawOption( Menu, OldOption );
	      DrawOption( Menu, Menu -> CurrentOption );
	      break;

	    case KeyCtrlM :
	      RetValue = Menu -> CurrentOption;
	      break;

	    default :
	      if ( Key >= ' ' && Key < '~' && ( KeyPos = strchr( Menu -> OptKeys, toupper( Key ) ) ) != NULL )
		{
		  OldOption = Menu -> CurrentOption;
		  RetValue = Menu -> CurrentOption = KeyPos - Menu -> OptKeys + 1;
		  DrawOption( Menu, OldOption );
		  DrawOption( Menu, RetValue );
		}
	      break;
	  }
	if ( RetValue == -1 && MustExit( Key, Menu -> ExitKeys ) )
	  break;
      }
    Menu -> LastKey = Key;

    if ( Menu -> Flags & MenuEraseRule )
      EraseRule();

    return( RetValue );
  }

/* ------------------------------------------------------------------- */

void EraseMenu( tMenu *Menu )

  {
    assert( *( ( int * ) Menu -> MemToSave ) == MagicNumber );
    Menu -> InternalFlags &= ~MenuDrawn;
    puttext( Menu -> Xpos, Menu -> Ypos,
	     Menu -> Xpos + Menu -> Width - 1,
	     Menu -> Ypos + Menu -> Heigth - 1,
	     ( ( int * ) Menu -> MemToSave ) + 1 );
  }

/* ------------------------------------------------------------------- */

void RedrawMenu( tMenu *Menu )

  {
    Menu -> InternalFlags &= ~MenuDrawn;
  }

/* ------------------------------------------------------------------- */
