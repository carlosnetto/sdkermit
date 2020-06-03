/* -------------------------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>

#include <types.e>

#include "malloc.e"

/* -------------------------------------------------------------------- */

static void (*UserHandler)( void ) = NULL;

/* -------------------------------------------------------------------- */

void *Malloc( size_t Size )

  {
    register void *Memory = malloc( Size );

    if ( Memory == NULL && UserHandler != NULL )
      (*UserHandler)();
    return( Memory );
  }

/* -------------------------------------------------------------------- */

void SetNoMemoryHandler( void (*Handler)( void ) )

  {
    UserHandler = Handler;
  }

/* ---------------------------------------------------------------------- */
