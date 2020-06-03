/* ---------------------------------------------------------------------- */

#ifndef _TERMINAL_E_
#define _TERMINAL_E_

#ifndef _TYPES_E_
#include <types.e>
#endif

/* ---------------------------------------------------------------------- */

typedef enum
  {
    TermVT52,
    TermVT100
  } tTermType;

typedef enum
  {
    CursorUnderline,
    CursorBlock
  } tCursorType;

typedef struct
  {
    tTermType TermType;
    tCursorType CursorType;
    bool WrapOn80;
    bool IsBackspaceDel;
    bool LocalEcho;
  } tTermDef;

/* ---------------------------------------------------------------------- */

void InitTerminal( tTermDef *TermDef );
void ResetTerminal( tTermDef *TermDef );
uint16 Connect( void );  /* Returns the Key used bye the user to exit from Connect */

/* ---------------------------------------------------------------------- */

#endif

/* ---------------------------------------------------------------------- */

