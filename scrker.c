/* -------------------------------------------------------------------- */

#include <stdlib.h>
#include <conio.h>
#include <bios.h>
#include <dos.h>

#include <types.e>

#include "bios.e"
#include "scrker.e"

/* -------------------------------------------------------------------- */

/* Box Characters - Norton page 409 */

typedef struct
  {
    int Hor;                    /* Horizontal char */
    int Ver;                    /* Vertical char */
    int LeftUpper;              /* Left Upper char */
    int LeftLower;              /* Left Lower char */
    int RightUpper;             /* Right Upper char */
    int RightLower;             /* Right Lower char */
  } tBoxDescr;

static tBoxDescr BoxDescrs[ 4 ] =
  { { 196, 179, 218, 192, 191, 217 },
    { 205, 179, 213, 212, 184, 190 },
    { 196, 186, 214, 211, 183, 189 },
    { 205, 186, 201, 200, 187, 188 } };

/* -------------------------------------------------------------------- */

static char AttrTab[] = {    0x07, 0x0f,        0x60,        0x6f,
			     0x87, 0x8f, 0x80 + 0x60, 0x80 + 0x6f };

static char CurrAttribute = 15;

/* -------------------------------------------------------------------- */

static unsigned int ModeTab[] = { 36, 32, 37, 33, 34, 38, 55 };

/* -------------------------------------------------------------------- */

void TurnOffVideo( void )

  {
    unsigned int Mode;

    Mode = peekb( 0, 0x449 );
    if ( Mode > 6 )
      return;
    Mode = ModeTab[ Mode ];

    while( ( inportb( 0x3da ) & 8 ) == 0 );
    outportb( 0x3d8, Mode );
  }

/* -------------------------------------------------------------------- */

void TurnOnVideo( void )

  {
    unsigned int Mode;

    Mode = peekb( 0, 0x449 );
    if ( Mode > 6 )
      return;
    Mode = ModeTab[ Mode ] | 8;
    outportb( 0xd38, Mode );
  }

/* -------------------------------------------------------------------- */

uint16 TurnOffCursor( void )

  {
    static uint16 OldValue;

    _AH = 3;              /* Read cursor position and size */
    _BH = 0;              /* Page 0 */
    BiosVideo();
    OldValue = _CX;
    _CH |= 32;
    _AH = 1;              /* Set cursor size */
    BiosVideo();
    return( OldValue );
  }

/* -------------------------------------------------------------------- */

uint16 TurnOnCursor( void )

  {
    static uint16 OldValue;

    _AH = 3;              /* Read cursor position and size */
    _BH = 0;              /* Page 0 */
    BiosVideo();
    OldValue = _CX;
    _CH &= ~32;
    _AH = 1;              /* Set cursor size */
    BiosVideo();
    return( OldValue );
  }

/* -------------------------------------------------------------------- */

uint16 SetCursorSize( int NewSize )

  {
    uint16 OldValue;

    _AH = 3;              /* Read cursor position and size */
    _BH = 0;              /* Page 0 */
    BiosVideo();
    OldValue = _CX;
    _CL = 7;
    _CH = 8 - NewSize;
    _AH = 1;              /* Set cursor size */
    BiosVideo();
    return( OldValue );
  }

/* -------------------------------------------------------------------- */

uint16 RestoreCursor( uint16 NewValue )

  {
    static uint16 OldValue;

    _AH = 3;              /* Read cursor position and size */
    _BH = 0;              /* Page 0 */
    BiosVideo();
    OldValue = _CX;
    _CX = NewValue;
    _AH = 1;              /* Set cursor size */
    BiosVideo();
    return( OldValue );
  }

/* -------------------------------------------------------------------- */

void ClearBox( int Left, int Upper, int Right, int Lower )

  {
    struct text_info SaveWindow;

    gettextinfo( &SaveWindow );
    window( Left, Upper, Right, Lower );
    clrscr();
    window( SaveWindow.winleft,
	    SaveWindow.wintop,
            SaveWindow.winright,
            SaveWindow.winbottom );
    gotoxy( SaveWindow.curx, SaveWindow.cury );
  }

/* -------------------------------------------------------------------- */

void ScrollUpBox( int Left, int Upper, int Right, int Lower, int Nlines )

  {
    struct text_info SaveWindow;
    char CurAttr;

    gettextinfo( &SaveWindow );
    CurAttr = SaveWindow.attribute;

    _AH = 6;              /* Scrool UP - Norton page 177 */
    _CH = Upper;
    _CL = Left;
    _DH = Lower;
    _DL = Right;
    _BH = CurAttr;
    _AL = Nlines;
    BiosVideo();
  }

/* -------------------------------------------------------------------- */

void ScrollDownBox( int Left, int Upper, int Right, int Lower, int Nlines )

  {
    struct text_info SaveWindow;
    char CurAttr;

    gettextinfo( &SaveWindow );
    CurAttr = SaveWindow.attribute;

    _AH = 7;              /* Scrool Down - Norton page 177 */
    _CH = Upper;
    _CL = Left;
    _DH = Lower;
    _DL = Right;
    _BH = CurAttr;
    _AL = Nlines;
    BiosVideo();
  }

/* -------------------------------------------------------------------- */

void PutCh( char Char )

  {
    _AH = 9; 	/* Write character and attribute */
    _AL = Char;
    _BL = CurrAttribute;
    _BH = 0;
    _CX = 1;
    BiosVideo();

    _AH = 3;	/* Read cursor position - Norton page 176 */
    _BH = 0;	/* page */
    BiosVideo();
    _DL = _DL + 1;  /* Incrementing column */
    _AH = 2;	/* Set cursor position - Norton page 175 */
    BiosVideo();
  }

/* -------------------------------------------------------------------- */

void PutChs( char Char, int NumberOfChs )

  {
    _AH = 9; 	/* Write character and attribute */
    _AL = Char;
    _BL = CurrAttribute;
    _BH = 0;
    _CX = NumberOfChs;
    BiosVideo();

    _AH = 3;	/* Read cursor position - Norton page 176 */
    _BH = 0;	/* page */
    BiosVideo();
    _DL = _DL + NumberOfChs;  /* Incrementing column */
    _AH = 2;	/* Set cursor position - Norton page 175 */
    BiosVideo();
  }

/* -------------------------------------------------------------------- */

void DrawBox( int Left, int Upper, int Right, int Lower, int BoxKind  )

  {
    register int I;
    register char C;
    tBoxDescr *BoxDescr;

    BoxDescr = &( BoxDescrs[ BoxKind ] );

    gotoxy( Left, Upper );
    PutCh( BoxDescr -> LeftUpper );
    gotoxy( Left, Lower );
    PutCh( BoxDescr -> LeftLower );
    gotoxy( Right, Upper );
    PutCh( BoxDescr -> RightUpper );
    gotoxy( Right, Lower );
    PutCh( BoxDescr -> RightLower );

    C = BoxDescr -> Hor;
    gotoxy( Left + 1, Upper );
    PutChs( C, Right - Left - 1 );
    gotoxy( Left + 1, Lower );
    PutChs( C, Right - Left - 1 );

    C = BoxDescr -> Ver;
    for ( I = Upper + 1; I < Lower; I++ )
      {
        gotoxy( Left, I );
        PutCh( C );
        gotoxy( Right, I );
        PutCh( C );
      }
  }

/* -------------------------------------------------------------------- */

void PutAttrStr( register char *Str )

  {
    while ( *Str )
      {
        if ( *Str == '#' )
          {
            Str++;
            if ( *Str >= '0' && *Str <= '7' )
	      textattr( CurrAttribute = AttrTab[ *Str - '0' ] );
            else if ( *Str == '#' )
              PutCh( '#' );
            else if ( *Str == '\0' )
              break;
          }
        else
          PutCh( *Str );
        Str++;
      }
  }

/* -------------------------------------------------------------------- */

int AttrStrLen( register char *Str )

  {
    register int Len = 0;

    while ( *Str )
      if ( *Str++ == '#' )
        {
          if ( *Str++ == '#' )
            Len++;
        }
      else
        Len++;
     return( Len );
  }

/* -------------------------------------------------------------------- */

void SetAttrTab( int Normal, int HiLight, int Reverse, int ReverseHiLight )

  {
    AttrTab[ 0 ] = Normal;
    AttrTab[ 1 ] = HiLight;
    AttrTab[ 2 ] = Reverse;
    AttrTab[ 3 ] = ReverseHiLight;
    AttrTab[ 4 ] = Normal + 128;
    AttrTab[ 5 ] = HiLight + 128;
    AttrTab[ 6 ] = Reverse + 128;
    AttrTab[ 7 ] = ReverseHiLight + 128;
    CurrAttribute = HiLight;
  }

/* -------------------------------------------------------------------- */
