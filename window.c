/* -------------------------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <conio.h>

#include <types.e>
#include <malloc.e>
#include <scrker.e>
#include "window.e"

/* -------------------------------------------------------------------- */

tWindowInfo *OpenWindow( int Left, int Upper, int Right, int Lower,
			 int BoxKind, char *Title,
			 char *BoxAttr, char *BordAttr, char *TitleAttr )

  {
    tWindowInfo *WindowInfo = Malloc( sizeof( tWindowInfo ) );
    void *SavedScreen = Malloc( ( Right - Left + 1 ) * ( Lower - Upper + 1 ) * 2 );

    gettextinfo( &( WindowInfo -> TextInfo ) );
    WindowInfo -> Left = Left;
    WindowInfo -> Upper = Upper;
    WindowInfo -> Right = Right;
    WindowInfo -> Lower = Lower;
    WindowInfo -> SavedScreen = SavedScreen;

    gettext( Left, Upper, Right, Lower, SavedScreen );
    PutAttrStr( BoxAttr );
    ClearBox( Left, Upper, Right, Lower );
    PutAttrStr( BoxAttr );
    DrawBox( Left, Upper, Right, Lower, BoxKind );
    window( Left, Upper, Right, Lower );
    gotoxy( ( Right - Left - 1 - strlen( Title ) ) / 2, 1 );
    PutAttrStr( TitleAttr );
    cprintf( " %s ", Title );

    return( WindowInfo );
  }

/* -------------------------------------------------------------------- */

void CloseWindow( tWindowInfo *WindowInfo )

  {
    struct text_info Ti;

    Ti = WindowInfo -> TextInfo;
    window( Ti.winleft, Ti.wintop, Ti.winright, Ti.winbottom );
    puttext( WindowInfo -> Left,
	     WindowInfo -> Upper,
	     WindowInfo -> Right,
	     WindowInfo -> Lower,
	     WindowInfo -> SavedScreen );
    free( SavedScreen );
    free( WindowInfo );
  }

/* -------------------------------------------------------------------- */
