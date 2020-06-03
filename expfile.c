/* ------------------------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <dos.h>
#include <dir.h>

#include <types.e>
#include <strtool.e>
#include <malloc.e>

#include "expfile.e"

/* ------------------------------------------------------------------- */

char **ExpandFile( char *FileMask, int *NoOfFiles )

  {
    struct ffblk FfBlk;
    int FileNo = 0;
    char **FileTab;
    char **AuxFileTab;
    int TabSize = 20;
    char *DirPrefix;
    char *FileNamePlace;

    if ( strpbrk( FileMask, ":\\" ) == NULL )
      DirPrefix = NULL;
    else
      {
	char *Aux;

	DirPrefix = Malloc( strlen( FileMask ) + 15 ); /* 15 bytes for the file name */
	strcpy( DirPrefix, FileMask );
	FileNamePlace = DirPrefix;
	while ( ( Aux = strpbrk( FileNamePlace, ":\\" ) ) != NULL )
	  FileNamePlace = Aux + 1;
      }

    FileTab = Malloc( TabSize * sizeof( char * ) );
    if ( findfirst( FileMask, &FfBlk, 0 ) == 0 )
      do
        {
          if ( FileNo == TabSize )
            {
              AuxFileTab = Malloc( 2 * TabSize * sizeof( char * ) );
              memcpy( AuxFileTab, FileTab, TabSize * sizeof( char * ) );
              free( FileTab );
              FileTab = AuxFileTab;
              TabSize *= 2;
            }
	  if ( DirPrefix == NULL )
	    FileTab[ FileNo++ ] = StrSave( FfBlk.ff_name );
	  else
	    {
	      strcpy( FileNamePlace, FfBlk.ff_name );
	      FileTab[ FileNo++ ] = StrSave( DirPrefix );
	    }
        }
      while ( findnext( &FfBlk ) == 0 );
    if ( FileNo == 0 )
      {
        free( FileTab );
        return( NULL );
      }
    else
      {
        if ( FileNo < TabSize )
          {
	    AuxFileTab = Malloc( FileNo * sizeof( char * ) );
            memcpy( AuxFileTab, FileTab, FileNo * sizeof( char * ) );
            free( FileTab );
            FileTab = AuxFileTab;
          }
        *NoOfFiles = FileNo;
	return( FileTab );
      }
  }

/* ------------------------------------------------------------------- */
