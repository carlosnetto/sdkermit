/* ------------------------------------------------------------------- */

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <conio.h>

#include <types.e>
#include <keyboard.e>
#include <malloc.e>
#include <scrker.e>

#include "writemsg.e"

/* ------------------------------------------------------------------- */

int WriteMsg( int X, int Y, char *Title, char *Format, ... )

  {
    va_list ArgPtr;
    int Len, StrLen, Key;
    char Msg[ 100 ];
    char *SavedScreen;
    uint16 LastCursor;

    va_start( ArgPtr, Format );
    vsprintf( Msg, Format, ArgPtr );
    va_end( ArgPtr );

    StrLen = strlen( Msg );
    Len = StrLen + 6;
    if ( Len + X >= 80 )
      {
	Len = 79 - X;
	StrLen = Len - 6;
      }

    SavedScreen = Malloc( Len * 3 * 2 );
    gettext( X, Y, X + Len - 1, Y + 2, SavedScreen );
    PutAttrStr( "#2" );
    ClearBox( X, Y, X + Len - 1, Y + 2 );
    DrawBox( X, Y, X + Len - 1, Y + 2, 3 );
    if ( Title != NULL )
      {
        gotoxy( X + ( Len - strlen( Title ) ) / 2, Y );
	cprintf( " %s ", Title );
      }
    gotoxy( X + 2, Y + 1 );
    cprintf( " %.*s ", StrLen, Msg );

    LastCursor = TurnOffCursor();
    Key = GetCh();
    RestoreCursor( LastCursor );

    puttext( X, Y, X + Len - 1, Y + 2, SavedScreen );
    free( SavedScreen );

    return( Key );
  }

/* ------------------------------------------------------------------- */
