/* ---------------------------------------------------------------------- */

#ifndef _SCRKER_E_
#define _SCRKER_E_

#ifndef _TYPES_E
#include <types.e>
#endif

/* ---------------------------------------------------------------------- */

void TurnOffVideo( void );
void TurnOnVideo( void );
uint16 TurnOffCursor( void );
uint16 TurnOnCursor( void );
uint16 SetCursorSize( int NewSize );
uint16 RestoreCursor( uint16 NewValue );
void ClearBox( int Left, int Upper, int Right, int Lower );
void ScrollUpBox( int Left, int Upper, int Right, int Lower, int Nlines );
void ScrollDownBox( int Left, int Upper, int Right, int Lower, int Nlines );
void PutCh( char Char );
void PutChs( char Char, int NumberOfChs );
void DrawBox( int Left, int Upper, int Right, int Lower, int BoxKind  );
void PutAttrStr( register char *Str );
int AttrStrLen( register char *Str );
void SetAttrTab( int Normal, int HiLight, int Reverse, int ReverseHiLight );

/* ---------------------------------------------------------------------- */

#endif

/* ---------------------------------------------------------------------- */

