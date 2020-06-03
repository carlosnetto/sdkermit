/* ------------------------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <types.e>
#include <scrker.e>
#include <newmenu.e>

#include "config.e"
#include "cfgterm.e"

/* ------------------------------------------------------------------- */

static char *TermOpts[] = { "Eco local",
			    "Retrocesso",
                            "Cursor",
			    "Quebra linha",
			    "Tipo",
                            "" };

static tMenu TermMenu = { "",
			  35, 4, 0, 0,
                          1,
                          0,
                          TermOpts,
			  1,
			  NULL,
                          NULL };

static char *LocalOpts[] = { "Sim",
			     "Nao",
                             "" };

static tMenu LocalMenu = { "",
			  37, 6, 0, 0,
                          1,
                          0,
                          LocalOpts,
			  1,
			  NULL,
                          NULL };

static char *BackspaceOpts[] = { "Del",
				 "Backspace",
                                 "" };

static tMenu BackspaceMenu = { "",
			       37, 7, 0, 0,
                               1,
                               0,
                               BackspaceOpts,
			       1,
			       NULL,
                               NULL };

static char *CursorOpts[] = { "Linha",
			      "Bloco",
                              "" };

static tMenu CursorMenu = { "",
			    37, 8, 0, 0,
                            1,
                            0,
                            CursorOpts,
			    1,
			    NULL,
                            NULL };

static tMenu WrapMenu = { "",
			  37, 9, 0, 0,
                          1,
                          0,
                          LocalOpts,
			  1,
			  NULL,
                          NULL };

static char *TermTypeOpts[] = { "A VT52",
				"B VT100",
				"" };

static tMenu TermTypeMenu = { "",
			      37, 10, 0, 0,
			      1,
			      0,
			      TermTypeOpts,
			      1,
			      NULL,
			      NULL };

/* ------------------------------------------------------------------- */

void ConfigTerminal( void )

  {
    int Opt;

    LocalMenu.CurrentOption = ProgCfg.TermDef.LocalEcho ? 1 : 2;
    BackspaceMenu.CurrentOption = ProgCfg.TermDef.IsBackspaceDel ? 1 : 2;
    CursorMenu.CurrentOption = ProgCfg.TermDef.CursorType + 1;
    WrapMenu.CurrentOption = ProgCfg.TermDef.WrapOn80 ? 1 : 2;
    TermTypeMenu.CurrentOption = ProgCfg.TermDef.TermType == TermVT52 ? 1 : 2;

    while ( ( Opt = ReadMenu( &TermMenu ) ) != 0 )
      switch ( Opt )
	{
	  case 1 :  /* Local On/Off */
	    ReadMenu( &LocalMenu );
	    EraseMenu( &LocalMenu );
	    break;

	  case 2 :  /* Backspace */
	    ReadMenu( &BackspaceMenu );
	    EraseMenu( &BackspaceMenu );
	    break;

	  case 3 :  /* Cursor Menu */
	    ReadMenu( &CursorMenu );
	    EraseMenu( &CursorMenu );
	    break;

	  case 4 :  /* Wrap */
	    ReadMenu( &WrapMenu );
	    EraseMenu( &WrapMenu );
	    break;

	  case 5 :  /* TermType */
	    ReadMenu( &TermTypeMenu );
	    EraseMenu( &TermTypeMenu );
	    break;

	  default :
	    assert( False );
      }
    EraseMenu( &TermMenu );

    ProgCfg.TermDef.LocalEcho = LocalMenu.CurrentOption == 1 ? True : False;
    ProgCfg.TermDef.IsBackspaceDel = BackspaceMenu.CurrentOption == 1 ? True : False;
    ProgCfg.TermDef.CursorType = CursorMenu.CurrentOption == 1 ? CursorUnderline : CursorBlock;
    ProgCfg.TermDef.WrapOn80 = WrapMenu.CurrentOption == 1 ? True : False;
    ProgCfg.TermDef.TermType = TermTypeMenu.CurrentOption == 1 ? TermVT52 : TermVT100;

    ResetTerminal( &ProgCfg.TermDef );
  }

/* ------------------------------------------------------------------- */
