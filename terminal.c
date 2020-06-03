/* ------------------------------------------------------------------- */

#include <conio.h>
#include <stdlib.h>
#include <ctype.h>
#include <dos.h>
#include <bios.h>
#include <assert.h>

#ifdef MYSDKER
#include <stdio.h>
#include <time.h>
#include <string.h>
#endif

#include <types.e>
#include <keyboard.e>
#include <scrker.e>

#ifdef MYSDKER
#include <writemsg.e>
#include <readstr.e>
#endif

#include "serial.e"
#include "bios.e"
#include "terminal.e"

/* ------------------------------------------------------------------- */

/*
  Screen Size and Position parameters
*/

#define FirstRow   1
#define LastRow   24
#define FirstCol   0
#define LastCol   79

/*
  Maximum number of ANSI-Arguments
*/

#define MaxArguments 15

/*
  BIOS Screen Services
*/

#define BiosSetVideoMode   0x00
#define BiosSetCursorSize  0x01
#define BiosSetCursor      0x02
#define BiosReadCursor     0x03
#define BiosSetDisplayPage 0x05
#define BiosScrollUp       0x06
#define BiosScrollDown     0x07
#define BiosWriteChar      0x09

/* ------------------------------------------------------------------- */

typedef enum { StNormal, StEscape, StRow, StCol, StPar, StSetG0, StSetG1 } tState;
typedef enum { ChNormal, ChGraphic } tCharSet;

/*
  Type of the terminal (either VT52 or VT100) and initial parameters
*/

static tTermDef  TermDef;
static tTermType TermType;

/*
  State of the "state-machine"
*/

static tState State;

/*
  Cursor position
*/

static int CurrRow;
static int CurrCol;

/*
  Table of graphic-characteres
*/

static char ChToGraphic[] =
  {
    0x04,     /*  96 - ` */
    0xb1,     /*  97 - a */
    0x1a,     /*  98 - b */
    0x1e,     /*  99 - c */
    0x1b,     /* 100 - d */
    0x18,     /* 101 - e */
    0xf8,     /* 102 - f */
    0xf1,     /* 103 - g */
    0x19,     /* 104 - h */
    0x17,     /* 105 - i */
    0xd9,     /* 106 - j */
    0xbf,     /* 107 - k */
    0xda,     /* 108 - l */
    0xc0,     /* 109 - m */
    0xc5,     /* 110 - n */
    0xc4,     /* 111 - o */
    0xc4,     /* 112 - p */
    0xc4,     /* 113 - q */
    0x5f,     /* 114 - r */
    0x5f,     /* 115 - s */
    0xc3,     /* 116 - t */
    0xb4,     /* 117 - u */
    0xc1,     /* 118 - v */
    0xc2,     /* 119 - w */
    0xb3,     /* 120 - x */
    0xf3,     /* 121 - y */
    0xf2,     /* 122 - z */
    0x14,     /* 123 - { */
    0xf7,     /* 124 - | */
    0x9c,     /* 125 - } */
    0xfa      /* 126 - ~ */
  };

/*
  ANSI arguments
*/

static int  Arguments[ MaxArguments ];
static int  ArgIdx;
static bool IsPrivate;

/*
  Character-set control
*/

static tCharSet G0Set;
static tCharSet G1Set;
static tCharSet *CharSet;

/*
   Attribute control flags
*/

#define ReverseAttribute 0x01
#define BoldAttribute	 0x02
#define BlinkAttribute   0x04

static uint8 CharAttribute;
static uint8 AttributeFlags;

/*
  Keyboard mode flags
*/

static bool CrLfMode;
static bool ApplicationCursor;
static bool NumericMode;
static bool IsBackspaceDel;

#ifdef MYSDKER
/*
  It's possible to read from a file, not only from the keyboard
*/

static char FileName[ 60 ] = "";
static FILE *KeyboardFile = NULL;
#endif

/*
  Screen operation flags and variables
*/

static bool VT52Emulation;
static bool AbsoluteOrigin;
static bool AutoWrap;
static bool TabArray[ ( LastCol - FirstCol ) + 1 ];
static bool ReverseScreen;
static char ScreenAttribute;
static tCursorType CursorType;
static bool LocalEcho;

/*
  Variables used to save the cursor position and the current attribute
*/

static int SaveRow, SaveCol;
static uint8 SaveAttributeFlags;

/*
  Scroll Region
*/

static int TopMargin;
static int BottomMargin;

/*
  IBM-PC screen page
*/

static uint8 CurrentPage = 0;


/* ------------------------------------------------------------------- */
/*
/*  Auxiliary macros
/*
/* ------------------------------------------------------------------- */

#define Get1stPar( n ) ( Arguments[ 0 ] ? Arguments[ 0 ] : ( n ) )
#define Get2ndPar( n ) ( ArgIdx && Arguments[ 1 ] ? Arguments[ 1 ] : ( n ) )


/* ------------------------------------------------------------------- */
/*
/*  Auxiliary functions
/*
/* ------------------------------------------------------------------- */

static void SerialPrintf( char *Format, ... )

  {
    va_list ArgPtr;
    char Msg[ 40 ];
    register char *ChPtr = Msg;

    va_start( ArgPtr, Format );
    vsprintf( Msg, Format, ArgPtr );
    va_end( ArgPtr );

    while ( *ChPtr )
      PutSerialCh( *ChPtr++ );
  }


/* ------------------------------------------------------------------- */

/*
    Function Name: ScrollUp
    Description  : Scrolls up a retangular region of the screen
*/

static void ScrollUp( int Top, int Bottom, int Lines )

  {
    if ( Lines > Bottom - Top )
      Lines = 0;
    _AH = BiosScrollUp;
    _AL = Lines;
    _BH = ScreenAttribute;
    _CH = Top;
    _CL = FirstCol;
    _DH = Bottom;
    _DL = LastCol;
    BiosVideo();
  }


/* ------------------------------------------------------------------- */

/*
    Function Name: ScrollDown
    Description  : Scrolls down a retangular region of the screen
*/

static void ScrollDown( int Top, int Bottom, int Lines )

  {
    if ( Lines > Bottom - Top )
      Lines = 0;
    _AH = BiosScrollDown;
    _AL = Lines;
    _BH = ScreenAttribute;
    _CH = Top;
    _CL = FirstCol;
    _DH = Bottom;
    _DL = LastCol;
    BiosVideo();
  }


/* ------------------------------------------------------------------- */

/*
    Function Name: AdjustCursor
    Description  : Put the cursor on CurrRow, CurrCol
*/

static void AdjustCursor( void )

  {
    _AH = BiosSetCursor;
    _BH = CurrentPage;
    _DH = CurrRow;
    _DL = CurrCol;
    BiosVideo();
  }


/* ------------------------------------------------------------------- */

/*
    Function Name: AdjustCharAttribute
    Description  : Computes the character attribute based on attribute-flags
*/

static void AdjustCharAttribute( void )

  {
    if ( ReverseScreen ^ ( ( AttributeFlags & ReverseAttribute ) ? 1 : 0 ) )
      if ( AttributeFlags & BoldAttribute )
	CharAttribute = 0x71;
      else
	CharAttribute = 0x70;
    else
      if ( AttributeFlags & BoldAttribute )
	CharAttribute = 0x0f;
      else
	CharAttribute = 0x07;

    if ( AttributeFlags & BlinkAttribute )
      CharAttribute |= 0x80;
  }



/* ------------------------------------------------------------------- */

/*
    Function Name: ChangeScreen
    Description  : Reverse or UnReverse the screen depending on the argument
*/

static void ChangeScreen( bool Reverse )

  {
    int Row, I;
    uint8 RowArray[ 2 * ( LastCol - FirstCol + 1 ) ];
    uint8 *Attr;
    uint8 BlinkMask;

    if ( ReverseScreen != Reverse )
      {
	ReverseScreen = Reverse;
	ScreenAttribute = Reverse ? 0x70 : 0x07;
	AdjustCharAttribute();
	for ( Row = FirstRow; Row <= LastRow; Row++ )
	  {
	    gettext( FirstCol + 1, Row + 1, LastCol + 1, Row + 1, RowArray );
	    Attr = RowArray + 1;
	    for ( I = 0; I < sizeof( RowArray ); I += 2 )
	      {
		BlinkMask = *Attr & 0x80;
		switch ( *Attr & 0x7f )
		  {
		    case 0x70 :
		      *Attr = 0x07 | BlinkMask;
		      break;

		    case 0x71 :
		      *Attr = 0x0f | BlinkMask;
		      break;

		    case 0x07 :
		      *Attr = 0x70 | BlinkMask;
		      break;

		    case 0x0f :
		      *Attr = 0x71 | BlinkMask;
		      break;

		    default :
		      *Attr = ScreenAttribute;
		      break;
		  }
		Attr += 2;
	      }
	    puttext( FirstCol + 1, Row + 1, LastCol + 1, Row + 1, RowArray );
	  }
      }
  }



/* ------------------------------------------------------------------- */
/*
/*  Actions performed by VT52 and VT100
/*
/* ------------------------------------------------------------------- */


/* ------------------------------------------------------------------- */

/*
    Function Name: CursorMotion
    Description  : Moves the cursor to a line, column
*/

static void CursorMotion( int Row, int Column )

  {
    CurrCol = Column + FirstCol - 1;
    CurrRow = Row + FirstRow - 1;
    if ( AbsoluteOrigin )
      {
	if ( ( uint16 ) CurrRow > LastRow )
	  CurrRow = LastRow;
      }
    else
      {
	CurrRow += ( TopMargin - FirstRow );
	if ( CurrRow < TopMargin )
	  CurrRow = TopMargin;
	else if ( CurrRow > BottomMargin )
	  CurrRow = BottomMargin;
      }
    if ( ( uint16 ) CurrCol > LastCol )
      CurrCol = LastCol;
    AdjustCursor();
  }


/* ------------------------------------------------------------------- */

/*
    Function Name: CursorRight
    Description  : Moves right the cursor
*/

static void CursorRight( int N )

  {
    CurrCol += N;
    if ( CurrCol > LastCol )
      CurrCol = LastCol;
    AdjustCursor();
  }


/* ------------------------------------------------------------------- */

/*
    Function Name: CursorLeft
    Description  : Moves left the cursor
*/

static void CursorLeft( int N )

  {
    if ( N > CurrCol - FirstCol )
      CurrCol = FirstCol;
    else
      CurrCol -= N;
    AdjustCursor();
  }


/* ------------------------------------------------------------------- */

/*
    Function Name: CursorUp
    Description  : Moves up the cursor
*/

static void CursorUp( int N )

  {
    if ( N > CurrRow - TopMargin )
      CurrRow = TopMargin;
    else
      CurrRow -= N;
    AdjustCursor();
  }


/* ------------------------------------------------------------------- */

/*
    Function Name: CursorDown
    Description  : Moves down the cursor
*/

static void CursorDown( int N )

  {
    CurrRow += N;
    if ( CurrRow > BottomMargin )
      CurrRow = BottomMargin;
    AdjustCursor();
  }


/* ------------------------------------------------------------------- */

/*
    Function Name: SaveCursorAndAttr
    Description  : Saves the cursor position and the current attributes
*/

static void SaveCursorAndAttr( void )

  {
    SaveCol = CurrCol;
    SaveRow = CurrRow;
    SaveAttributeFlags = AttributeFlags;
  }


/* ------------------------------------------------------------------- */

/*
    Function Name: RestoreCursorAndAttr
    Description  : Restores the values saved by SaveCursorAndAttr()
*/

static void RestoreCursorAndAttr( void )

  {
    CurrCol = SaveCol;
    CurrRow = SaveRow;
    AttributeFlags = SaveAttributeFlags;
    AdjustCharAttribute();
    AdjustCursor();
  }


/* ------------------------------------------------------------------- */

/*
    Function Name: HomeCursor
    Description  : Put the cursor on the first column, first line
*/

static void HomeCursor( void )

  {
    CurrCol = FirstCol;
    if ( AbsoluteOrigin )
      CurrRow = FirstRow;
    else
      CurrRow = TopMargin;
    AdjustCursor();
  }


/* ------------------------------------------------------------------- */

/*
    Function Name: CarriageReturn
    Description  : Put the cursor on the first column of the current line
*/

static void CarriageReturn( void )

  {
    CurrCol = FirstCol;
    AdjustCursor();
  }


/* ------------------------------------------------------------------- */

/*
    Function Name: LineFeed
    Description  : Moves down the cursor on line
*/

static void LineFeed( void )

  {
    if ( CurrRow == BottomMargin )
      ScrollUp( TopMargin, BottomMargin, 1 );
    else if ( CurrRow != LastRow )
      {
	CurrRow++;
	AdjustCursor();
      }
  }


/* ------------------------------------------------------------------- */

/*
    Function Name: ReverseLineFeed
    Description  : Moves up the cursor on line
*/

static void ReverseLineFeed( void )

  {
    if ( CurrRow == TopMargin )
      ScrollDown( TopMargin, BottomMargin, 1 );
    else if ( CurrRow != FirstRow )
      {
	CurrRow--;
	AdjustCursor();
      }
  }


/* ------------------------------------------------------------------- */

/*
    Function Name: NextTab
    Description  : Moves the cursor to the next tab
*/

static void NextTab( void )

  {
    while ( CurrCol < LastCol )
      if ( TabArray[ ++CurrCol - FirstCol ] )
        break;
    AdjustCursor();
  }


/* ------------------------------------------------------------------- */

/*
    Function Name: TabSet
    Description  : Set the tabulation for the current column
*/

static void TabSet( void )

  {
    TabArray[ CurrCol - FirstCol ] = True;
  }


/* ------------------------------------------------------------------- */

/*
    Function Name: ClearTab
    Description  : Clears current or all tabs
*/

static void ClearTab( int Code )

  {
    register int I;

    switch( Code )
      {
        case 0 :
          TabArray[ CurrCol - FirstCol ] = False;
          break;

        case 3 :
          for ( I = FirstCol; I <= LastCol; I++ )
            TabArray[ I - FirstCol ] = False;
      }
  }


/* ------------------------------------------------------------------- */

/*
    Function Name: ClearLine
    Description  : Clears part of the current line
*/

static void ClearLine( int Part )
    
  {
    int Left, Right;

    switch ( Part )
      {
	case 0 : /* To eol */
	  Left = CurrCol;
	  Right = LastCol;
	  break;

	case 1 : /* To cursor */
	  Left = FirstCol;
	  Right = CurrCol;
	  break;

	case 2 : /* entry line */
	  Left = FirstCol;
	  Right = LastCol;
	  break;
      }
    if ( Right > Left )
      {
	_AH = BiosScrollUp;
	_AL = 0;
	_BH = ScreenAttribute;
	_CH = CurrRow;
	_CL = Left;
	_DH = CurrRow;
	_DL = Right;
	BiosVideo();
      }
  }


/* ------------------------------------------------------------------- */

/*
    Function Name: ClearScreen
    Description  : Clears part of the screen
*/

static void ClearScreen( int Part )

  {
    uint16 Top, Bottom;

    switch( Part )
      {
	case 0 :
	  ClearLine( 0 );
	  Top = CurrRow + 1;
	  Bottom = LastRow;
	  break;

	case 1 :
	  ClearLine( 1 );
	  Top = FirstRow;
	  Bottom = CurrRow - 1;
	  break;

	case 2 :
	  Top = FirstRow;
	  Bottom = LastRow;
	  break;
      }
    if ( Bottom >= Top )
      {
	_AH = BiosScrollUp;
	_AL = 0;
	_BH = ScreenAttribute;
	_CH = Top;
	_CL = FirstCol;
	_DH = Bottom;
	_DL = LastCol;
	BiosVideo();
      }
  }


/* ------------------------------------------------------------------- */

/*
    Function Name: SetAttributes
    Description  : Sets the character attributes
*/

static void SetAttributes( void )

  {
    int I;

    for ( I = 0; I <= ArgIdx; I++ )
      switch ( Arguments[ I ] )
	{
	  case 0 :
	    AttributeFlags = 0;
	    break;

	  case 1 :
	  case 4 :
	    AttributeFlags |= BoldAttribute;
	    break;

	  case 5 :
	    AttributeFlags |= BlinkAttribute;
	    break;

	  case 7 :
	    AttributeFlags |= ReverseAttribute;
	    break;
	}
    AdjustCharAttribute();
  }


/* ------------------------------------------------------------------- */

/*
    Function Name: SetMode
    Description  : Sets some modes of operation
*/

static void SetMode( int Code )

  {
    switch ( Code )
      {
	case 20 :
	  CrLfMode = True;
	  break;

	case 1 :
	  ApplicationCursor = True;
	  break;

	case 3 :
	  /* PutInto132Columns() */
	  HomeCursor();
	  ClearScreen( 2 );
	  break;

	case 4 :
	  /* Turns on smooth scroll */
	  break;

	case 5 :
	  ChangeScreen( True );
	  break;

	case 6 :
	  AbsoluteOrigin = False;
	  HomeCursor();
	  break;

	case 7 :
	  AutoWrap = True;
	  break;

	case 8 :
	  /* Turns on auto-repeat */
	  break;
      }
  }


/* ------------------------------------------------------------------- */

/*
    Function Name: ResetMode
    Description  : Resets some modes of operation
*/

static void ResetMode( int Code )

  {
    switch ( Code )
      {
	case 20 :
	  CrLfMode = False;
	  break;

	case 1 :
	  ApplicationCursor = False;
	  break;

	case 2 :
	  VT52Emulation = True;
	  break;

	case 3 :
	  /* PutInto80Columns() */
	  HomeCursor();
	  ClearScreen( 2 );
	  break;

	case 4 :
	  /* Turns on jump-scroll */
	  break;

	case 5 :
	  ChangeScreen( False );
	  break;

	case 6 :
	  AbsoluteOrigin = True;
	  break;

	case 7 :
	  AutoWrap = False;
	  break;

	case 8 :
	  /* Turns off auto-repeat */
	  break;
      }
  }


/* ------------------------------------------------------------------- */

/*
    Function Name: SetScrollRegion
    Description  : Sets the Top and Bottom margins (from arguments[])
*/

static void SetScrollRegion( int Top, int Bottom )

  {
    TopMargin = Top - 1 + FirstRow;
    BottomMargin = Bottom - 1 + FirstRow;
    if ( TopMargin < FirstRow || TopMargin > LastRow )
      TopMargin = FirstRow;
    if ( BottomMargin < FirstRow || BottomMargin > LastRow )
      BottomMargin = LastRow;
    HomeCursor();
  }


/* ------------------------------------------------------------------- */

/*
    Function Name: Report
    Description  : Reports some informations about the terminal
*/

static void Report( int Code )

  {
    switch ( Code )
      {
	case 5 :
	  SerialPrintf( "\033[0n" );
	  break;

	case 6 :
	  SerialPrintf( "\033[%d;%dR", CurrRow - FirstRow + 1, CurrCol - FirstCol + 1 );
	  break;
      }
  }


/* ------------------------------------------------------------------- */

/*
    Function Name: IdentifyTerm
    Description  : Writes the terminal-identification to the serial port
*/

static void IdentifyTerm( void )

  {
    if ( TermType == TermVT52 || VT52Emulation )
      SerialPrintf( "\033/%c", TermType == TermVT52 ? 'K' : 'Z' );
    else
      SerialPrintf( "\033[?1;2c" );
  }


/* ------------------------------------------------------------------- */

/*
    Function Name: OutCh
    Description  : Writes one printable character on the screen
*/

static void OutCh( char Ch )

  {
    if ( *CharSet == ChGraphic && Ch >= '`' && Ch <= '~' )
      Ch = ChToGraphic[ Ch - '`' ];
    _AH = BiosWriteChar;
    _AL = Ch;
    _BH = CurrentPage;
    _BL = CharAttribute;
    _CX = 1;
    BiosVideo();
    if ( CurrCol < LastCol )
      {
	CurrCol++;
	AdjustCursor();
      }
    else if ( AutoWrap )
      {
	CarriageReturn();
	LineFeed();
      }
  }


/* ------------------------------------------------------------------- */

/*
    Function Name: ResetTerm
    Description  : Resets the VT100 parameters
*/

static void ResetTerm( void )

  {
    register int I;

    State        = StNormal;
    TopMargin    = FirstRow;
    BottomMargin = LastRow;

    G0Set = G1Set = ChNormal;
    CharSet = &G0Set;

    AttributeFlags = 0;
    AdjustCharAttribute();

    TermType            = TermDef.TermType;
    CursorType          = TermDef.CursorType;
    IsBackspaceDel      = TermDef.IsBackspaceDel;
    LocalEcho           = TermDef.LocalEcho;
    AutoWrap	        = TermDef.WrapOn80;
    CrLfMode 		= False;
    ApplicationCursor 	= False;
    NumericMode 	= True;
    VT52Emulation 	= False;
    AbsoluteOrigin 	= True;

    I = LastCol - FirstCol;
    while ( I-- )
      TabArray[ I ] = ( I % 8 ) ? False : True;

    ReverseScreen = False;
    ScreenAttribute = 0x07;
    ClearScreen( 2 );
    PutAttrStr( "#0" );
    DrawBox( 25, 9, 55, 15, 0 );
    gotoxy( 31, 11 );
    PutAttrStr( "#1SDKermit Versao 1.1#0" );
    gotoxy( 33, 13 );
    PutAttrStr( "#1Software Design#0" );
    HomeCursor();
    SaveCursorAndAttr();
  }

/* ------------------------------------------------------------------- */

static void HandleNormal( register uint8 Ch )

  {
    if ( Ch >= ' ' && Ch < 127 )  /* 127 == DEL */
      OutCh( Ch );
    else switch ( Ch )
      {
	case 000 :	/* ENQ */
	  /* transmit answer-back message */
	  break;

	case 007 :   	/* BEL */
	  sound( 1000 );
	  delay( 100 );
          nosound();
	  break;

	case 010 :   	/* BS */
	  CursorLeft( 1 );
	  break;

	case 011 :   	/* TAB */
	  NextTab();
	  break;

	case 012 :   	/* LF */
	case 013 :	/* VT */
	case 014 :	/* FF */
	  if ( CrLfMode )
	    CarriageReturn();
	  LineFeed();
	  break;

	case 015 :   	/* CR */
	  CarriageReturn();
	  break;

	case 016 :	/* SO */
	  CharSet = &G1Set;
	  break;

	case 017 :	/* SI */
	  CharSet = &G0Set;
	  break;

	case 033 :   	/* ESC */
	  State = StEscape;
	  break;
      }
  }


/* ------------------------------------------------------------------- */

static void HandleEscape( register uint8 Ch )

  {
    if ( TermType == TermVT52 || VT52Emulation )
      {
	switch( Ch )
	  {
	    case '<' :
	      if ( TermType == TermVT100 )
		VT52Emulation = False;
	      break;

	    case '=' :
	      NumericMode = False;
	      break;

	    case '>' :
	      NumericMode = True;
	      break;

	    case 'A' :
	      CursorUp( 1 );
	      break;

	    case 'B' :
	      CursorDown( 1 );
	      break;

	    case 'C' :
	      CursorRight( 1 );
	      break;

	    case 'D' :
	      CursorLeft( 1 );
	      break;

	    case 'F' :   /* Turns On Graph Mode */
	      *CharSet = ChGraphic;
	      break;

	    case 'G' :   /* Turns Off Graph Mode */
	      *CharSet = ChNormal;
	      break;

	    case 'I' :
	      ReverseLineFeed();
	      break;

	    case 'H' :
	      HomeCursor();
	      break;

	    case 'J' :
	      ClearScreen( 0 );
	      break;

	    case 'K' :
	      ClearLine( 0 );
	      break;

	    case 'Y' :
	      State = StRow;
	      break;

	    case 'Z' :
	      IdentifyTerm();
	      break;
	  }
	if ( State == StEscape )
	  State = StNormal;
      }
    else
      {
	switch( Ch )
	  {
	    case '=' :
	      NumericMode = False;
	      break;

	    case '>' :
	      NumericMode = True;
	      break;

	    case '7' :
	      SaveCursorAndAttr();
	      break;

	    case '8' :
	      RestoreCursorAndAttr();
	      break;

	    case '(' :
	      State = StSetG0;
	      break;

	    case ')' :
	      State = StSetG1;
	      break;

	    case '[' :
	      IsPrivate = False;
	      State = StPar;
	      ArgIdx = 0;
	      Arguments[ 0 ] = 0;
	      break;

	    case 'D' :
	      LineFeed();
	      break;

	    case 'M' :
	      ReverseLineFeed();
	      break;

	    case 'E' :
	      CarriageReturn();
	      LineFeed();
	      break;

	    case 'H' :
	      TabSet();
	      break;

	    case 'c' :
	      ResetTerm();
	      break;

	    case 'Z' :
	      IdentifyTerm();
	      break;
	  }
	if ( State == StEscape && Ch != 033 )   /* 033 == ESC */
	  State = StNormal;
      }
  }


/* ------------------------------------------------------------------- */

static void HandlePar( uint8 Ch )

  {
    switch( Ch )
      {
	case 030 :	/* CAN */
	case 032 :	/* SUB */
	  OutCh( 168 );    /* Inverted question-mark */
	  State = StNormal;
	  break;

	case '0' :
	case '1' :
	case '2' :
	case '3' :
	case '4' :
	case '5' :
	case '6' :
	case '7' :
	case '8' :
	case '9' :
	  Arguments[ ArgIdx ] = Arguments[ ArgIdx ] * 10 + Ch - '0';
	  break;

	case ';' :
	  if ( ++ArgIdx < MaxArguments )
	    Arguments[ ArgIdx ] = 0;
	  else
	    Arguments[ --ArgIdx ] = 0;
	  break;

	case '?' :
	  IsPrivate = True;
	  break;

	case 'h' :
	  SetMode( Arguments[ 0 ] );
	  State = StNormal;
	  break;

	case 'l' :
	  ResetMode( Arguments[ 0 ] );
	  State = StNormal;
	  break;

	case 'm' :
	  SetAttributes();
	  State = StNormal;
	  break;

	case 'r' :
	  SetScrollRegion( Get1stPar( 1 ), Get2ndPar( 24 ) );
	  State = StNormal;
	  break;

	case 'A' :
	  CursorUp( Get1stPar( 1 ) );
	  State = StNormal;
	  break;

	case 'B' :
	  CursorDown( Get1stPar( 1 ) );
	  State = StNormal;
	  break;

	case 'C' :
	  CursorRight( Get1stPar( 1 ) );
	  State = StNormal;
	  break;

	case 'D' :
	  CursorLeft( Get1stPar( 1 ) );
	  State = StNormal;
	  break;

	case 'L' :
	  ScrollDown( CurrRow, BottomMargin, Get1stPar( 1 ) );
	  CarriageReturn();
	  State = StNormal;
	  break;

	case 'M' :
	  ScrollUp( CurrRow, BottomMargin, Get1stPar( 1 ) );
	  CarriageReturn();
	  State = StNormal;
	  break;

	case 'H' :
	case 'f' :
	  CursorMotion( Get1stPar( 1 ), Get2ndPar( 1 ) );
	  State = StNormal;
	  break;

	case 'g' :
	  ClearTab( Arguments[ 0 ] );
	  State = StNormal;
	  break;

	case 'K' :
	  ClearLine( Arguments[ 0 ] );
	  State = StNormal;
	  break;

	case 'J' :
	  ClearScreen( Arguments[ 0 ] );
	  State = StNormal;
	  break;

	case 'n' :
	  Report( Arguments[ 0 ] );
	  State = StNormal;
	  break;

	case 'c' :
	  IdentifyTerm();
	  State = StNormal;
	  break;
      }
  }


/* ------------------------------------------------------------------- */

static void HandleSetG0( uint8 Ch )

  {
    switch ( Ch )
      {
	case '0' :
	case '1' :
	case '2' :
	  G0Set = ChGraphic;
	  break;

	default :
	  G0Set = ChNormal;
      }
    State = StNormal;
  }

/* ------------------------------------------------------------------- */

static void HandleSetG1( uint8 Ch )

  {
    switch ( Ch )
      {
	case '0' :
	case '1' :
	case '2' :
	  G1Set = ChGraphic;
	  break;

	default :
	  G1Set = ChNormal;
      }
    State = StNormal;
  }

/* ------------------------------------------------------------------- */

static void HandleRow( uint8 Ch )

  {
    CurrRow = Ch - ' ' + FirstRow;
    if ( ( uint16 ) CurrRow > LastRow )
      CurrRow = LastRow;
    State = StCol;
  }

/* ------------------------------------------------------------------- */

static void HandleCol( uint8 Ch )

  {
    CurrCol = Ch - ' ' + FirstCol;
    if ( ( uint16 ) CurrCol > LastCol )
      CurrCol = LastCol;
    AdjustCursor();
    State = StNormal;
  }

/* ------------------------------------------------------------------- */

static void TermPutCh( register uint8 Ch )

  {
    Ch &= 0x7f;
    switch( State )
      {
	case StNormal :
	  HandleNormal( Ch );
	  break;

	case StEscape :
	  HandleEscape( Ch );
	  break;

	case StRow :
	  HandleRow( Ch );
	  break;

	case StCol :
	  HandleCol( Ch );
	  break;

	case StPar :
	  HandlePar( Ch );
	  break;

	case StSetG0 :
	  HandleSetG0( Ch );
	  break;

	case StSetG1 :
	  HandleSetG1( Ch );
	  break;
      }
  }



/* ------------------------------------------------------------------- */
/*
/* Keyboard management part
/* ------------------------
/*
/* ------------------------------------------------------------------- */

static uint8 *CharPtr;			/* Next char to return */
static uint8 StringSize    = 0;		/* Number of chars in the buffer */

/* ------------------------------------------------------------------- */

static struct
  {
    uint8 Scan;
    uint8 String[ 4 ];
  } KeypadTab[] = { { 0x37, "\033?l" },	   /* * (Comma)  */
		    { 0x4e, "\033?M" },    /* + (Enter)  */
		    { 0x4e, "\033?M" },    /* , (Unused) */
		    { 0x4a, "\033?m" },    /* - (Minus)  */
		    { 0x53, "\033?n" },    /* . */
		    { 0x4e, "\033?M" },    /* \ (Unused) */
		    { 0x52, "\033?p" },    /* 0 */
		    { 0x4f, "\033?q" },    /* 1 */
		    { 0x50, "\033?r" },    /* 2 */
		    { 0x51, "\033?s" },    /* 3 */
		    { 0x4b, "\033?t" },    /* 4 */
		    { 0x4c, "\033?u" },    /* 5 */
		    { 0x4d, "\033?v" },    /* 6 */
		    { 0x47, "\033?w" },    /* 7 */
		    { 0x48, "\033?x" },    /* 8 */
		    { 0x49, "\033?y" } };  /* 9 */

/* ------------------------------------------------------------------- */

static void FillString( void )

  {
    static uint8 ChrBuffer[ 4 ];
    register uint16 Hi, Lo;
    enum { CursorKey, PFKey, AltKey } KeyKind;
    char LastChar;

    if ( KeyPressed() )
      {
	Hi = GetChScan();
	Lo = Hi & 0xff;
	if ( Lo == 0 )
	  {
	    switch( Hi )
	      {
		case KeyUp :		/* Up Arrow */
		  KeyKind = CursorKey;
		  LastChar = 'A';
		  break;
		case KeyDown :		/* Down Arrow */
		  KeyKind = CursorKey;
		  LastChar = 'B';
		  break;
		case KeyRight :		/* Right Arrow */
		  KeyKind = CursorKey;
		  LastChar = 'C';
		  break;
		case KeyLeft :		/* Left Arrow */
		  KeyKind = CursorKey;
		  LastChar = 'D';
		  break;
		case KeyF1 :		/* F1 */
		  KeyKind = PFKey;
		  LastChar = 'P';
		  break;
		case KeyF2 :		/* F2 */
		  KeyKind = PFKey;
		  LastChar = 'Q';
		  break;
		case KeyF3 :    	/* F3 */
		  KeyKind = PFKey;
		  LastChar = 'R';
		  break;
		case KeyF4 :		/* F4 */
		  KeyKind = PFKey;
		  LastChar = 'S';
		  break;
		case KeyAltS :
		case KeyAltK :
		case KeyAltC :
		  KeyKind = AltKey;
		  CharPtr = "\377";
		  StringSize = 1;
		  switch ( Hi )
		    {
		      case KeyAltS :
			UnGetCh( 'S' );
			break;
		      case KeyAltK :
			UnGetCh( 'K' );
			break;
		      case KeyAltC :
			UnGetCh( 'C' );
			break;
		    }
		  break;
#ifdef MYSDKER
		case KeyAltF :         /* Read from a file */
		  if ( ReadBoxStr( "Arquivo", FileName, sizeof( FileName ), 3, 3, 40 ) && strlen( FileName ) )
		    if ( ( KeyboardFile = fopen( FileName, "r" ) ) == NULL )
		      WriteMsg( 3, 3, "Erro", "\"%.65s\" nao existe", FileName );
		  AdjustCursor();
		  break;
#endif
	      }
	    if ( KeyKind != AltKey )
	      {
		CharPtr = ChrBuffer;
		*CharPtr++ = '\033';   /* ESC */
		if ( KeyKind == CursorKey && TermType == TermVT100 && ! VT52Emulation )
		  *CharPtr++ = ApplicationCursor ? 'O' : '[';
		else if ( KeyKind == PFKey )
		  {
		    if ( TermType == TermVT100 && ! VT52Emulation )
		      *CharPtr++ = 'O';
		    else if ( ! NumericMode )
		      *CharPtr++ = '?';
		  }
		*CharPtr++ = LastChar;
		StringSize = CharPtr - ChrBuffer;
		CharPtr = ChrBuffer;
	      }
	  }
        else
	  {
	    Hi >>= 8;
	    if ( ! NumericMode && Lo >= '*' && Lo <= '9' && Hi == KeypadTab[ Lo - '*' ].Scan  )  /* Keypad Keys */
	      {
		CharPtr = KeypadTab[ Lo - '*' ].String;
		StringSize = 3;
		if ( TermType == TermVT100 && ! VT52Emulation )
		  CharPtr[ 1 ] = 'O';
		else
		  CharPtr[ 1 ] = '?';
              }
	    else if ( Lo == '*' && Hi == KeypadTab[ '*' - '*' ].Scan )
	      {
		CharPtr = ",";
		StringSize = 1;
	      }
	    else if ( IsBackspaceDel && Lo == 0x08 && Hi == 0x0e )   /* Backspace */
	      {
		CharPtr = "\177";
		StringSize = 1;
	      }
	    else if ( Lo == '\r' || ( Lo == '+' && Hi == KeypadTab[ Lo - '*' ].Scan ) )
	      {
		if ( CrLfMode )
		  {
		    CharPtr = "\r\n";
		    StringSize = 2;
		  }
		else
		  {
		    CharPtr = "\r";
		    StringSize = 1;
		  }
	      }
	    else
	      {
		CharPtr = ChrBuffer;
		ChrBuffer[ 0 ] = Lo;
		StringSize = 1;
	      }
	  }
      }
  }

/* ------------------------------------------------------------------- */

static int16 VT52GetCh( void )

  {
#ifdef MYSDKER
    if ( KeyboardFile != NULL )
      {
	char Ch = getc( KeyboardFile );

	if ( Ch == EOF )
	  {
	    fclose( KeyboardFile );
	    KeyboardFile = NULL;
	  }
	else
	  {
	    if ( Ch == '\n' )
	      {
		clock_t Time = clock();
		while ( clock() - Time <= 10 );
		return( '\r' );
	      }
	    else
	      return( Ch );
	  }
      }
#endif

    if ( StringSize == 0 )
      FillString();

    if ( StringSize != 0 )
      {
	StringSize--;
	return( *CharPtr++ );
      }
    else
      return( -1 );
  }

/* ------------------------------------------------------------------- */



/* ------------------------------------------------------------------- */
/*
/* Terminal emulator management part
/* ---------------------------------
/*
/*-------------------------------------------------------------------- */


void ResetTerminal( tTermDef *TermDef1 )

  {
    TermDef 	   = *TermDef1;
    TermType       = TermDef.TermType;
    CursorType     = TermDef.CursorType;
    IsBackspaceDel = TermDef.IsBackspaceDel;
    LocalEcho      = TermDef.LocalEcho;
    AutoWrap       = TermDef.WrapOn80;
  }

/*-------------------------------------------------------------------- */

void InitTerminal( tTermDef *TermDef1 )

  {
    TermDef = *TermDef1;
    ResetTerm();
  }

/*-------------------------------------------------------------------- */

uint16 Connect( void )

  {
    register int16 Ch;
    static char Buffer[ 2000 ];
    register uint16 I, BytesRead;
    uint16 OldCursor;

    OldCursor = SetCursorSize( CursorType == CursorBlock ? 8 : 2 );
    AdjustCursor();

    while ( 1 )
      {
	BytesRead = ReadSerial( Buffer, sizeof( Buffer ) - 2 );
	for ( I = 0; I < BytesRead; I++ )
	  TermPutCh( Buffer[ I ] );
	if ( ( Ch = VT52GetCh() ) != -1 && Ch != 255 )
	  {
	    if ( LocalEcho )
	      TermPutCh( Ch );
	    PutSerialCh( Ch );
	  }
	if ( Ch == 255 )
          break;
      }
    RestoreCursor( OldCursor );
    return( GetCh() ); /* Last char was ungot to the buffer */
  }

/*-------------------------------------------------------------------- */
