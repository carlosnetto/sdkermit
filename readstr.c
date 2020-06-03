/* ------------------------------------------------------------------- */

#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <assert.h>

#include <types.e>
#include <keyboard.e>
#include <malloc.e>
#include <scrker.e>

#include "readstr.e"

/* ------------------------------------------------------------------- */

static int X1, X, X2, Y;
static int I, Length, Offset;

/* ------------------------------------------------------------------- */

static void UpdateString( char *String )

  {
    int J = Offset;

    TurnOffCursor();
    gotoxy( X = X1, Y );
    while ( X <= X2 && String[ J ] )
      {
        PutCh( String[ J++ ] );
        X++;
      }
    while ( X <= X2 )
      {
        PutCh( ' ' );
        X++;
      }
    X = X1 + I - Offset;
    gotoxy( X, Y );
    TurnOnCursor();
  }

/* ------------------------------------------------------------------- */

bool ReadBoxStr( char *Title, char *String, int MaxLen, int Xpos, int Ypos, int Width )

  {
    void *SavedBox = Malloc( Width * 3 * 2 );
    uint16 Key;
    uint16 OldCursor;

    assert( Width > 6 );

    OldCursor = TurnOffCursor();

    X2 = Xpos + Width - 1; 			/* Posicao direita da caixa */

    gettext( Xpos, Ypos, X2, Ypos + 2, SavedBox );  /* Desenha a caixa e titulo */
    PutAttrStr( "#2" );
    ClearBox( Xpos, Ypos, X2, Ypos + 2 );
    DrawBox( Xpos, Ypos, X2, Ypos + 2, 0 );
    if ( Title != NULL )
      {
	gotoxy( Xpos - ( strlen( Title ) - Width ) / 2, Ypos );
        cprintf( " %s ", Title );
      }

    X1 = Xpos + 2;   	       /* Primeira posicao util para string */
    X2 = X2 - 2;     	       /* Ultima posicao util para string */
    Y = Ypos + 1;              /* Linha para leitura */
    Length = strlen( String );      /* Tamanho da linha em edicao */
    Offset = 0;                     /* Offset da linha */
    if ( X1 + Length < X2 )
      I = Length;
    else
      I = X2 - X1;

    PutAttrStr( "#3" );
    UpdateString( String );
    PutAttrStr( "#2" );

    TurnOnCursor();

    Key = GetCh();
    UnGetCh( Key );
    switch ( Key )
      {
        case KeyEsc :
          break;

        case KeyCtrlM :
        case KeyCtrlH :
        case KeyLeft :
	case KeyRight :
	case KeyCtrlS :
	case KeyCtrlD :
          UpdateString( String );
          break;

        default :
          String[ 0 ] = '\0';
          I = Length = 0;
          UpdateString( String );
          break;
      }

    while ( 1 )
      {
	gotoxy( X, Y );
	Key = GetCh();
	if ( Key == KeyEsc || Key == KeyCtrlM )
	  break;
        switch ( Key )
          {
	    case KeyLeft :
	    case KeyCtrlS :
              if ( I )
                {
                  I--;
                  if ( X > X1 )
                    X--;
                  else
                    {
                      Offset--;
                      UpdateString( String );
                    }
                }
              break;

	    case KeyRight :
	    case KeyCtrlD :
              if ( I < Length )
                {
                  I++;
                  if ( X < X2 )
                    X++;
                  else
                    {
                      Offset++;
                      UpdateString( String );
                    }
                }
              break;

            case KeyCtrlH :
              if ( I )
                {
                  int J = I;

                  Length--;
                  I--;
                  while ( String[ J ] )
                    {
                      String[ J - 1 ] = String[ J ];
                      J++;
                    }
                  String[ J - 1 ] = '\0';
                  if ( X > X1 )
                    X--;
                  else
                    Offset--;
                  UpdateString( String );
                }
              break;

            default :
              if ( Key >= ' ' && Key <= '~' && I < MaxLen - 1 )
                {
                  int J;

                  J = Length = Length + 1;
                  while ( J > I )
                    {
                      String[ J ] = String[ J - 1 ];
                      J--;
                    }
                  String[ I++ ] = Key;
                  if ( X < X2 )
                    X++;
                  else
                    Offset++;
                  UpdateString( String );
                }
              break;
          }
      }
    RestoreCursor( OldCursor );

    puttext( Xpos, Ypos, X2 + 2, Ypos + 2, SavedBox );
    return( Key == KeyCtrlM );
  }

/* ------------------------------------------------------------------- */

