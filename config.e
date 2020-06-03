/* ---------------------------------------------------------------------- */

#ifndef _CONFIG_E_
#define _CONFIG_E_

#ifndef _TYPES_E_
#include <types.e>
#endif

#ifndef _SERIAL_E_
#include <serial.e>
#endif

#ifndef _TERMINAL_E_
#include <terminal.e>
#endif

#ifndef _CFGSCR_E_
#include <cfgscr.e>
#endif

/* ---------------------------------------------------------------------- */
/*
/* Global constants
/*
/* ---------------------------------------------------------------------- */

#define BufSize 3000   /* Size of the serial-buffer */


/* ---------------------------------------------------------------------- */
/*
/* The configuration struct
/*
/* ---------------------------------------------------------------------- */

typedef struct
  {
    char Key[ 30 ];
    long Position;
    tSerialDef SerialDef;
    tTermDef TermDef;
    tScreenDef ScreenDef;
  } tProgramConfig;

extern tProgramConfig ProgCfg;
extern uint16 OriginalCursor;

/* ---------------------------------------------------------------------- */

void InitAll( int Argc, char *Argv[] );
char *SaveConfig( void );

/* ---------------------------------------------------------------------- */

#endif

/* ---------------------------------------------------------------------- */

