/* ---------------------------------------------------------------------- */

#ifndef _NEWMENU_E_
#define _NEWMENU_E_

#ifndef _TYPES_E_
#include <types.e>
#endif

/* ---------------------------------------------------------------------- */

typedef enum
  {
    MenuHorizontal = 0x0001,
    MenuDrawRule   = 0x0002,
    MenuEraseRule  = 0x0004
  } tMenuFlags;

typedef struct
  {
    char *Name;
    unsigned char Xpos, Ypos, Width, Heigth;
    char BoxKind;
    tMenuFlags Flags;
    char **Options;
    char CurrentOption;
    uint16 *ExitKeys;
    void *MemToSave;
    uint16 LastKey;
    unsigned char Xopt, Yopt;
    char LastOption;
    char *OptKeys;
    uint8 InternalFlags;
  } tMenu;

/* ---------------------------------------------------------------------- */

int ReadMenu( tMenu *Menu );
void EraseMenu( tMenu *Menu );
void RedrawMenu( tMenu *Menu );

/* ---------------------------------------------------------------------- */

#endif

/* ---------------------------------------------------------------------- */
