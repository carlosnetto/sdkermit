/* ------------------------------------------------------------------- */

#ifndef _PROTOCOL_E_
#define _PROTOCOL_E_

/* ------------------------------------------------------------------- */

#ifndef _TYPES_E_
#include <types.e>
#endif

/* ------------------------------------------------------------------- */

bool SendFiles( int NumberOfFiles, char **Files, bool Server );
bool ReceiveFiles( char *FileMask, bool Server );
void Server( void );
bool Finish( void );
bool Logout( void );

/* ------------------------------------------------------------------- */

#endif _PROTOCOL_E_

/* ------------------------------------------------------------------- */
