/* ------------------------------------------------------------------- */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <types.e>
#include <malloc.e>

#include "strtool.e"

/* ------------------------------------------------------------------- */

char *StrSave( char *String )

  {
    char *Saved;

    if ( ( Saved = Malloc( strlen( String ) + 1 ) ) == NULL )
      return( NULL );
    else
      {
        strcpy( Saved, String );
        return( Saved );
      }
  }

/* ------------------------------------------------------------------- */

char *LeftTrim( register char *String )

  {
    while ( *String == ' ' )
      String++;
    return( String );
  }

/* ------------------------------------------------------------------- */
