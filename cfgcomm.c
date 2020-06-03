/* ------------------------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <types.e>
#include <scrker.e>
#include <newmenu.e>

#include "config.e"
#include "cfgcomm.e"

/* ------------------------------------------------------------------- */

static char *CommOpts[] = { "Linha",
			    "Velocidade",
			    "Tamanho",
			    "Paridade",
			    "Bits de parada",
			    "Controle de fluxo",
                            "" };

static tMenu SerialMenu = { "",
			    35, 5, 0, 0,
                            1,
                            0,
                            CommOpts,
			    1,
			    NULL,
                            NULL };

static char *LineOpts[] = { "A COM1",
                            "B COM2",
                            "" };

static tMenu LineMenu   = { "",
			    37, 7, 0, 0,
                            1,
                            0,
                            LineOpts,
			    1,
			    NULL,
                            NULL };

static char *BaudOpts[] = { "A 300",
                            "B 600",
                            "C 1200",
                            "D 2400",
                            "E 4800",
                            "F 9600",
                            "G 19200",
                            "H 38400",
                            "I 57600",
                            "" };

static tMenu BaudMenu =   { "",
			    37, 8, 0, 0,
                            1,
                            0,
                            BaudOpts,
			    1,
			    NULL,
                            NULL };

static char *BitsOpts[] = { "A 7 Bits",
                            "B 8 Bits",
                            "" };

static tMenu BitsMenu   = { "",
			    37, 9, 0, 0,
                            1,
                            0,
                            BitsOpts,
			    1,
			    NULL,
                            NULL };

static char *ParityOpts[] = { "Nenhuma",
			      "Par",
			      "Impar",
                              "" };

static tMenu ParityMenu   = { "",
			      37, 10, 0, 0,
                              1,
                              0,
                              ParityOpts,
			      1,
			      NULL,
                              NULL };

static char *StopOpts[] = { "A - 1 bit",
                            "B - 2 bits",
                            "" };

static tMenu StopMenu   = { "",
			    37, 11, 0, 0,
                            1,
                            0,
                            StopOpts,
			    1,
			    NULL,
                            NULL };

static char *AutoOpts[] = { "Sim",
			    "Nao",
                            "" };

static tMenu AutoMenu   = { "",
			    37, 12, 0, 0,
                            1,
                            0,
                            AutoOpts,
			    1,
			    NULL,
                            NULL };

/* ------------------------------------------------------------------- */

void ConfigSerial( void )

  {
    int Opt;
    static enum eBaudRate BaudTab[] = { B300, B600, B1200, B2400,
					B4800, B9600, B19200,
					B38400, B57600 };

    CloseSerial();

    LineMenu.CurrentOption = ( ProgCfg.SerialDef.Port == COM1 ) ? 1 : 2;
    BitsMenu.CurrentOption = ProgCfg.SerialDef.Size - 6;
    ParityMenu.CurrentOption = ProgCfg.SerialDef.Parity + 1;
    StopMenu.CurrentOption = ProgCfg.SerialDef.StopBits;
    AutoMenu.CurrentOption = ProgCfg.SerialDef.XonXoff ? 1 : 2;
    Opt = 0;
    do
      if ( BaudTab[ Opt++ ] == ProgCfg.SerialDef.BaudRate )
	{
	  BaudMenu.CurrentOption = Opt;
	  break;
	}
    while ( Opt < sizeof( BaudTab ) );

    while ( ( Opt = ReadMenu( &SerialMenu ) ) != 0 )
      switch ( Opt )
	{
	  case 1 :  /* Line */
	    ReadMenu( &LineMenu );
	    EraseMenu( &LineMenu );
	    break;

	  case 2 :  /* Baud Rate */
	    ReadMenu( &BaudMenu );
	    EraseMenu( &BaudMenu );
	    break;

	  case 3 :  /* Bits */
	    ReadMenu( &BitsMenu );
	    EraseMenu( &BitsMenu );
	    break;

	  case 4 :  /* Parity */
	    ReadMenu( &ParityMenu );
	    EraseMenu( &ParityMenu );
	    break;

	  case 5 :  /* Stop Bits */
	    ReadMenu( &StopMenu );
	    EraseMenu( &StopMenu );
	    break;

	  case 6 :  /* Xon/Xoff */
	    ReadMenu( &AutoMenu );
	    EraseMenu( &AutoMenu );
	    break;

	  default :
	    assert( False );
      }
    EraseMenu( &SerialMenu );

    ProgCfg.SerialDef.Port = LineMenu.CurrentOption == 1 ? COM1 : COM2;
    ProgCfg.SerialDef.Size = BitsMenu.CurrentOption + 6;
    ProgCfg.SerialDef.Parity = ParityMenu.CurrentOption - 1;
    ProgCfg.SerialDef.StopBits = StopMenu.CurrentOption;
    ProgCfg.SerialDef.XonXoff = AutoMenu.CurrentOption == 1 ? True : False;
    ProgCfg.SerialDef.BaudRate = BaudTab[ BaudMenu.CurrentOption - 1 ];

    OpenSerial( &ProgCfg.SerialDef );
  }

/* ------------------------------------------------------------------- */
