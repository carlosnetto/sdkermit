/*#define TRACE_PROTOCOL*/

/* ------------------------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <conio.h>
#include <io.h>
#include <assert.h>
#include <setjmp.h>
#include <time.h>

#include <types.e>
#include <keyboard.e>
#include <malloc.e>
#include <scrker.e>
#include <expfile.e>
#include <writemsg.e>

#include "serial.e"
#include "config.e"
#include "protocol.e"

/* ------------------------------------------------------------------- */

#define ToChar( Ch )  ( ( Ch ) + ' ' )
#define UnChar( Ch )  ( ( Ch ) - ' ' )
#define Control( Ch ) ( ( Ch ) ^ 64  )

/* ------------------------------------------------------------------- */

#define SOH	      1

#define MyQuoteChar   '#'
#define MyQuoteBin    '&'
#define MyPadChar     '\0'
#define MyPad         0
#define MyEol         '\r'
#define MyTimeout     4

#define MaxPackSize   94
#define MaxTries      31000

/* ------------------------------------------------------------------- */

static jmp_buf TimeOutJmp;  	   /* JmpBuf for longjmp when timeout */

static char **FileList;            /* List of files to be sent */
static int FileCount;              /* Number of files yet to send */
static char *FileName;             /* Current file name */

static char State;		   /* Present state of the automaton */
static int PacketNumber;           /* PacketNumber */
static int NumTries;               /* Times this packet retried */
static int OldTry;                 /* Times previous packet retried */
static int Size;		   /* Size of present data */

static int Pad;                    /* How much pad to send */
static char PadChar;               /* Padding character to send */

static char EndOfLine;             /* End-Of-Line character to send */
static char QuoteChar;             /* Quote character in incoming data */

static char QuoteBin;		   /* Binary quote character - 0 if none */
static char SendQuoteBin;          /* Binary quote character for Send-Init */
static char SentQuoteBin;	   /* Binary Quote character was sent */
static bool HandleBinaryFiles;     /* Can I transmit/receive binary files ? */

static FILE *CurrentFile;          /* Current File */
static char CurrFileName[ MaxPackSize ];   /* It's name, when creating it */

static bool ServerMode;	           /* Is SDKermit in server mode ? */

static int SendPackSize;	   /* Maximum send packet size */
static int Timeout;		   /* Timeout for foreign host on sends */
static clock_t Clickout;	   /* Timeout for foreing host on sends in clicks, not seconds */
static char ReceivedPacket[ MaxPackSize + 4 ];  /* Receive packet buffer */
static char Packet[ MaxPackSize + 4 ];          /* Packet buffer */

/* ------------------------------------------------------------------------ */

static void *SavedScreen;	/* Saves the box for writing messages to the user */

static unsigned int PacketNo;   /* Packet number for writing on the screen */
static unsigned int RetrieNo;   /* Retrie number for showing to the user */
static bool ShowPacketNo;	/* Must show the packet number to the user */
static unsigned int LastRetrieNo;

/* ------------------------------------------------------------------------ */

/*

  FlushInput:
    Flushs all pending characters.

*/

static void FlushInput( void )

  {
    while ( ReadSerial( ReceivedPacket, sizeof( ReceivedPacket ) - 3 ) > 0 );
  }


/* ------------------------------------------------------------------------ */

/*

  GetNextFile:
    Sets FileName to the next File. Returns False if it does not exist.

*/

static bool GetNextFile( void )

  {
    if ( FileCount == 0 )
      return( False );
    else
      {
	FileCount--;
        FileName = *( FileList++ );
        return( True );
      }
  }


/* ------------------------------------------------------------------------ */

/*

  ShowDoing
    Shows what is going on

*/

static void ShowDoing( char FirstState )

  {
    PutAttrStr( "#2" );
    gotoxy( 26, 10 );
    switch ( FirstState )
      {
	case 'S' :
	  cprintf( " Enviando:" );
	  break;

	case 'R' :
	  cprintf( "Recebendo:" );
	  break;

	case 'V' :
	  cprintf( " Servindo:%13s", "" );
	  break;
      }
    gotoxy( 26, 11 );
    cprintf( "  Pacotes: 0    " );
    gotoxy( 26, 12 );
    cprintf( "   Falhas: 0    " );
  }


/* ------------------------------------------------------------------------ */

/*

  InitKermit:
    Initializes all necessary variables.

*/

static void InitKermit( char FirstState, bool MustDrawBox )

  {
    FlushInput();

    CurrentFile = NULL;

    State = FirstState;

    SendPackSize = MaxPackSize;
    Timeout = MyTimeout;
    Clickout = MyTimeout * ( int ) CLK_TCK;
    Pad = MyPad;
    PadChar = MyPadChar;
    EndOfLine = MyEol;
    QuoteChar = MyQuoteChar;

    HandleBinaryFiles = True;
    if ( ProgCfg.SerialDef.Parity == None && ProgCfg.SerialDef.Size == 8 )
      {
	SendQuoteBin = 'Y';
	QuoteBin = 0;
      }
    else
      SendQuoteBin = QuoteBin = MyQuoteBin;
    SentQuoteBin = 0;

    PacketNumber = 0;
    NumTries = 0;

    ServerMode = ( FirstState == 'V' );

    if ( MustDrawBox )
      {
	SavedScreen = Malloc( 34 * 8 * 2 + 10 );   /* 10 = medo */
	gettext( 24, 8, 57, 15, SavedScreen );
	PutAttrStr( "#2" );
	ClearBox( 24, 8, 57, 15 );
	DrawBox( 24, 8, 57, 15, 3 );
	gotoxy( 25, 14 );
	PutAttrStr( "#1        <Ctrl-C> Cancela        " );
      }

    if ( strchr( "SRV", FirstState ) != NULL )
      {
	RetrieNo = LastRetrieNo = PacketNo = 0;
	ShowPacketNo = True;
	ShowDoing( FirstState );
      }
    else
      ShowPacketNo = False;
  }


/* ------------------------------------------------------------------------ */

/*

  ErrorMsg
    Prints an error message

*/


static void ErrorMsg( char *Message, ... );


/* ------------------------------------------------------------------------ */

/*

  DiskFull
    Tells the user that the disk is full, and erases the current file

*/


static void DiskFull( void )

  {
    if ( CurrentFile != NULL )
      {
	fclose( CurrentFile );
	CurrentFile = NULL;
      }
    unlink( CurrFileName );
    ErrorMsg( "Disco cheio. \"%s\" apagado", CurrFileName );
  }



/* ------------------------------------------------------------------------ */

/*

  AbortMessage
    Tells the user that he aborted the file transferation

*/


static void AbortMessage( void )

  {
    ErrorMsg( "Transmissao cancelada pelo usuario" );
  }



/* ------------------------------------------------------------------------ */

/*

  FinishKermit:

*/

static void FinishKermit( bool Success )

  {
    if ( CurrentFile != NULL )
      fclose( CurrentFile );
    if ( Success )
      WriteMsg( 40, 13, "Aviso", "Transmissao bem sucedida" );
    puttext( 24, 8, 57, 15, SavedScreen );
    free( SavedScreen );
  }


/* ------------------------------------------------------------------------ */

/*

  SetPar:
    Fill the data array with my send-init parameters.

*/

static void SetPars( char *Data )

  {
    Data[ 0 ] = ToChar( MaxPackSize );	   			/* Biggest packet I can receive */
    Data[ 1 ] = ToChar( MyTimeout );				/* When I want to be timed out */
    Data[ 2 ] = ToChar( MyPad );				/* How much padding I need */
    Data[ 3 ] = Control( MyPadChar );				/* Padding character I want */
    Data[ 4 ] = ToChar( MyEol );				/* End-Of-Line character I want */
    Data[ 5 ] = MyQuoteChar;					/* Control-Quote character I send */
    Data[ 6 ] = SentQuoteBin = SendQuoteBin;			/* Eight-Bit-Quote character */
  }


/* ------------------------------------------------------------------------ */

/*

  GetPars:
    Get the other host's send-init parameters

*/

static void GetPars( char *Data, int Length )

  {
    if ( Length > 0 )
      SendPackSize = UnChar( Data[ 0 ] );			/* Maximum send packet size */
    else
      SendPackSize = MaxPackSize;

    if ( Length > 1 )
      Timeout = UnChar( Data[ 1 ] );				/* When I should time out */
    else
      Timeout = MyTimeout;
    Clickout = Timeout * ( int ) CLK_TCK;

    if ( Length > 2 )
      Pad = UnChar( Data[ 2 ] );				/* Number of pads to send */
    else
      Pad = MyPad;

    if ( Length > 3 )
      PadChar = Control( Data[ 3 ] );				/* Padding character to send */
    else
      PadChar = MyPadChar;

    if ( Length > 4 )
      EndOfLine = UnChar( Data[ 4 ] );				/* EOL character I must send */
    else
      EndOfLine = MyEol;

    if ( Length > 5 )
      QuoteChar = Data[ 5 ];					/* Incoming data quote character */
    else
      QuoteChar = MyQuoteChar;

    if ( Length > 6 )						/* Incoming data bin-quote character */
      {
	char RemoteQuoteBin = Data[ 6 ];

	if ( ( RemoteQuoteBin > 32 && RemoteQuoteBin < 63 ) ||
	     ( RemoteQuoteBin > 95 && RemoteQuoteBin < 127 ) )
	  {
	    if ( SentQuoteBin )   /* I sent a quote bin */
	      {
		if ( SentQuoteBin == 'Y' || SentQuoteBin == RemoteQuoteBin )
		  QuoteBin = RemoteQuoteBin;
		else  /* I sent a QuoteBin different from RemoteQuoteBin */
	          {
		    HandleBinaryFiles = False;
		    QuoteBin = 0;
		  }
	      }
	    else /* I didn't send a quote bin yet */
	      SendQuoteBin = QuoteBin = RemoteQuoteBin;
	  }
	else if ( RemoteQuoteBin == 'Y' )
	  /* Ok - nothing to do */;
	else  /* A invalid RemoteQuoteBin or 'N' */
	  {
	    HandleBinaryFiles = False;
	    QuoteBin = 0;
	    SendQuoteBin = 'N';
	  }
      }
    else
      {
	HandleBinaryFiles = False;
	QuoteBin = 0;
	SendQuoteBin = 'N';
      }
  }


/* ------------------------------------------------------------------------ */

/*

  SendPack:
    Send a Packet

*/

static void SendPack( char Type, int Number, int Length, char *Data )

  {
    int I;							/* Character loop counter */
    char CheckSum;						/* CheckSum */

#ifdef TRACE_PROTOCOL
    Data[ Length ] = '\0';
    printf( "\rSend: [%c,%2d] %s\n", Type, Number, Data );
#endif

    if ( ShowPacketNo )
      {
	if ( ( ++PacketNo % 8 ) == 0 )
	  {
	    gotoxy( 37, 11 );
	    cprintf( "%u", PacketNo );
	  }
	if ( RetrieNo != LastRetrieNo )
	  {
	    LastRetrieNo = RetrieNo;
	    gotoxy( 37, 12 );
	    cprintf( "%u", RetrieNo );
	  }
      }

    for ( I = 1; I <= Pad; I++ )
      PutSerialCh( PadChar ); 					/* Issue any padding */

    PutSerialCh( SOH );						/* Packet marker, ASCII 1 (SOH) */
    PutSerialCh( ToChar( Length + 3 ) );			/* Send the character count */
    CheckSum     = ToChar( Length + 3 );			/* Initialize the CheckSum */
    PutSerialCh( ToChar( Number ) );				/* Packet number */
    CheckSum    += ToChar( Number );				/* Update CheckSum */
    PutSerialCh( Type );					/* Packet type */
    CheckSum    += Type;					/* Update CheckSum */

    for ( I = 0; I < Length; I++ )				/* Loop for all data characters */
      {
	PutSerialCh( Data[ I ] );				/* Get a character */
	CheckSum += Data[ I ];					/* Update CheckSum */
      }

    CheckSum = ( ( ( CheckSum & 0300 ) >> 6 ) + CheckSum ) & 077; /* Compute final CheckSum */
    PutSerialCh( ToChar( CheckSum ) );				/* Put it in the packet */
    PutSerialCh( EndOfLine );					/* Extra-packet line terminator */
  }

/* ------------------------------------------------------------------------ */

/*

  NextCh:
    Returns one more character.

*/

static char NextCh( void )

  {
    static char Buffer[ 150 ];
    static int BufPos;
    static int NumOfChars = 0;
    static clock_t Weakeup;

    if ( NumOfChars == 0 )
      {
	NumOfChars = ReadSerial( Buffer, sizeof( Buffer ) - 1 );
	if ( NumOfChars == 0 )
	  {
	    Weakeup = clock() + Clickout;
	    while ( True )
	      {
		NumOfChars = ReadSerial( Buffer, sizeof( Buffer ) - 1 );
		if ( NumOfChars )
		  break;
		if ( KeyPressed() )
		  {
		    uint16 Key = GetCh();
		    switch ( Key )
		      {
			case KeyCtrlM :
			  longjmp( TimeOutJmp, KeyCtrlM );
			  break;

			case KeyCtrlC :
			  longjmp( TimeOutJmp, KeyCtrlC );
			  break;
		      }
		  }
		NumOfChars = ReadSerial( Buffer, sizeof( Buffer ) - 1 );
		if ( NumOfChars )
		  break;
		if ( clock() >= Weakeup )
		  longjmp( TimeOutJmp, KeyCtrlM );
	      }
	  }
        BufPos = 0;
      }
    NumOfChars--;
    return( Buffer[ BufPos++ ] );
  }


/* ------------------------------------------------------------------------ */

/*

  ReceivePack:
    Receive a Packet

*/

static char ReceivePack( int *Length, int *Number, char *Data )

  {
    register char Ch;						/* Character read */
    int Len;                                            	/* Packet Length */
    int Num;							/* Packet Number */
    int Type;							/* Packet Type */
    register int ComputedCheckSum;				/* Computed CheckSum */
    int ReceivedCheckSum;                                       /* Received CheckSum */
    int I;

    switch ( setjmp( TimeOutJmp ) )
      {
	case 0 :
	  break;

	case KeyCtrlM :   /* Time Out */
	  return( False );

	case KeyCtrlC :   /* Abort */
	  return( 'A' );
      }

    do
      Ch = NextCh();
    while ( Ch != SOH );

    while ( 1 )			                        	/* Loop to get a packet */
      {
	Ch = NextCh();                  			/* Get character */
	if ( Ch == SOH )                                        /* Resynchronize if SOH */
          continue;
	ComputedCheckSum = Ch;          			/* Start the CheckSum */
	Len = UnChar( Ch ) - 3;					/* Character count */

	Ch = NextCh();						/* Get character */
	if ( Ch == SOH )					/* Resynchronize if SOH */
	  continue;
	ComputedCheckSum += Ch;					/* Update CheckSum */
	Num = UnChar( Ch );					/* Packet number */

	Ch = NextCh();						/* Get character */
	if ( Ch == SOH )					/* Resynchronize if SOH */
	  continue;
	ComputedCheckSum += Ch;					/* Update CheckSum */
	Type = Ch;						/* Packet type */

	for ( I = 0; I < Len; I++ )				/* The data itself, if any */
	  {							/* Loop for character count */
	    Ch = NextCh();					/* Get character */
	    if ( Ch == SOH )					/* Resynchronize if SOH */
	      continue;
	    ComputedCheckSum += Ch;				/* Update CheckSum */
	    Data[ I ] = Ch;					/* Put it in the data buffer */
	  }
	Data[ Len ] = 0;					/* Mark the end of the data */

	Ch = NextCh();						/* Get last character (CheckSum) */
	ReceivedCheckSum = UnChar( Ch );			/* Convert to numeric */
	Ch = NextCh();						/* get EOL character and toss it */
	if ( Ch == SOH )					/* Resynchronize if SOH */
	  continue;
	break;							/* Got CheckSum, done */
      }

    ComputedCheckSum = ( ( ( ComputedCheckSum & 0300 ) >> 6 ) +
		       ComputedCheckSum ) & 077; 		/* final CheckSum */

    *Length = Len;
    *Number = Num;
    if ( ComputedCheckSum != ReceivedCheckSum )
      return( False );

#ifdef TRACE_PROTOCOL
    Data[ Len ] = '\0';
    printf( "\rRece: [%c,%2d] %s\n", Type, Num, Data );
#endif

    return( Type );						/* All OK, return packet type */
  }




/* ------------------------------------------------------------------------ */

/*

  PrintErrorPacket
    Prints the contents of a ErrorPacket.

*/


static void PrintErrorPacket( char *Data )

  {
    if ( ! ServerMode )
      WriteMsg( 10, 10, "Erro", "%s", Data );
  }

/* ------------------------------------------------------------------------ */

/*

  ErrorMsg
    Prints an error message

*/


static void ErrorMsg( char *Message, ... )

  {
    va_list ArgPtr;
    char Msg[ 130 ];

    va_start( ArgPtr, Message );
    vsprintf( Msg, Message, ArgPtr );
    va_end( ArgPtr );

    SendPack( 'E', PacketNumber, strlen( Msg ), Msg );
    SendPack( 'E', PacketNumber, strlen( Msg ), Msg );  /* If the 1st got lost ... */
    if ( ! ServerMode )
      WriteMsg( 10, 10, "Erro", Msg );
  }


/* ------------------------------------------------------------------------ */

/*

  TooManyTries
    Prints an error message for too many retries

*/


static void TooManyTries( void )

  {
    ErrorMsg( "Numero maximo de falhas excedido" );
  }


/* ------------------------------------------------------------------------ */

/*

  ProtocolError
    Prints an error message for any protocol error

*/


static void ProtocolError( void )

  {
    ErrorMsg( "Falha grave na transmissao" );
  }



/* ------------------------------------------------------------------------ */

/*

  FillBuffer:
    Get a bufferful of data from the file that's being sent.
    Only control-quoting and 8-bit are done; repeat count prefix are
    not handled.

*/

static int FillBuffer( char *Buffer )

  {
    register int I, Ch, Ch7;

    I = 0;
    while ( ( Ch = getc( CurrentFile ) ) != EOF )
      {
	Ch7 = Ch & 0x7f;
	if ( Ch > 127 )
	  if ( HandleBinaryFiles )
	    {
	      if ( QuoteBin )
		{
		  Buffer[ I++ ] = QuoteBin;
		  Ch = Ch7;
		}
	    }
	  else
	    return( False );

	if ( Ch7 < ' ' || Ch7 == 127 || Ch7 == QuoteChar || Ch7 == QuoteBin )
	  {
	    Buffer[ I++ ] = QuoteChar;				/* Quote the character */
	    if ( Ch7 < ' ' || Ch7 == 127 )
	      Ch = Control( Ch );				/* and uncontrolify */
	  }
	Buffer[ I++ ] = Ch;					/* Deposit the character itself */

	if ( I >= SendPackSize - 11 )				/* Check length */
	  return( I );
      }
    if ( I == 0 )                                       	/* Wind up here only on EOF */
      return( EOF );

    return( I );						/* Handle partial buffer */
  }


/* ------------------------------------------------------------------------ */

/*

  WritePack:
    Put data from a incoming packet into a file.

*/

static bool WritePack( char *Buffer, int Length )

  {
    register int I;						/* Counter */
    register char Ch, Ch7;						/* Character holder */
    char Mask;							/* 8-bit mask */

    for ( I = 0; I < Length; I++ )				/* Loop thru the data field */
      {
	Ch = Buffer[ I ];					/* Get character */
	Ch7 = Ch & 0x7f;
	if ( QuoteBin && Ch7 == QuoteBin )
	  {
	    Mask = 0x80;
	    Ch = Buffer[ ++I ];
	    Ch7 = Ch & 0x7f;
	  }
	else
	  Mask = 0;
	if ( Ch7 == MyQuoteChar )				/* Control quote? */
	  {							/* Yes */
	    Ch = Buffer[ ++I ];					/* Get the quoted character */
	    Ch7 = Ch & 0x7f;
	    if ( Ch7 != MyQuoteChar && !( QuoteBin && Ch7 == QuoteBin ) )	/* Low order bits match quote char? */
	      Ch = Control( Ch );				/* No, uncontrollify it */
	  }
	if ( putc( Ch | Mask, CurrentFile ) == EOF )
	  return( False );
      }
    return( True );
  }


/* ------------------------------------------------------------------------ */

/*

  SendInit:
    Send this host's parameters and get other side's back.

*/

static char SendInit( void )

  {
    int Number, Length;

    if ( NumTries++ > MaxTries )				/* If too many tries, give up */
      {
	TooManyTries();
	return( 'A' );
      }
    SetPars( Packet );						/* Fill up init info packet */
    FlushInput();						/* Flush pending input */

    SendPack( 'S', PacketNumber, 7, Packet );			/* Send an S packet */
    switch( ReceivePack( &Length, &Number, ReceivedPacket ) )
      {
	case 'N' :
	  RetrieNo++;
	  return( State );					/* NAK, try it again */

	case 'Y' :						/* ACK */
	  if ( Number != PacketNumber )  			/* If wrong ACK, stay in S state ... */
	    {
	      RetrieNo++;
	      return( State );		      			/* and try again */
	    }
	  GetPars( ReceivedPacket, Length );          		/* Get other side's init info */
	  NumTries = 0;
	  PacketNumber = ( PacketNumber + 1 ) % 64;
	  return( 'F' );		      			/* OK, switch state to F */

	case 'E' :						/* Error packet received */
	  PrintErrorPacket( ReceivedPacket );
	  return( 'A' );

	case 'A' :
	  AbortMessage();
	  return( 'A' );

	case False :	        				/* Receive failure, try again */
	   RetrieNo++;
	   return( State );

	default :						/* Anything else, just "abort" */
	  ProtocolError();
	  return( 'A' );
      }
  }


/* ------------------------------------------------------------------------ */

/*

  SendFile:
    Send File Header.

*/

static char SendFile( void )

  {
    int Number, Length;
    char ConvFileName[ 50 ];
    char *NewFileName;						/* Pointer to file name to send */

    if ( NumTries++ > MaxTries )
      {
	TooManyTries();
	return( 'A' );
      }

    if ( CurrentFile == NULL )					/* If not already open, */
      {
	CurrentFile = fopen( FileName, "rb" );
	if ( CurrentFile == NULL || isatty( fileno( CurrentFile ) ) )
	  {
	    ErrorMsg( "\"%s\" nao encontrado", FileName );
	    if ( CurrentFile != NULL )
	      {
		fclose( CurrentFile );
		CurrentFile = NULL;
	      }
	    return( 'A' );
	  }
      }

    strcpy( ConvFileName, FileName );
    NewFileName = strpbrk( ConvFileName, ":\\" );               /* Strip the path */
    if ( NewFileName != NULL )
      {
	char *Aux;

	NewFileName++;
	if ( ( Aux = strrchr( NewFileName, '\\' ) ) != NULL )
	  NewFileName = Aux + 1;
      }
    else
      NewFileName = ConvFileName;

    Length = strlen( NewFileName );				/* Compute length of new filename */

    gotoxy( 37, 10 );
    cprintf( "%-12s", NewFileName );

    SendPack( 'F', PacketNumber, Length, NewFileName );  	/* Send an F packet */
    switch( ReceivePack( &Length, &Number, ReceivedPacket ) ) 	/* What was the reply? */
      {
	case 'N' :						/* NAK, just stay in this state, */
	  Number = ( --Number < 0 ? 63 : Number );		/* unless it's NAK for next packet */
	  if ( PacketNumber != Number )				/* which is just like an ACK for */
	    {
	      RetrieNo++;
	      return( State );					/* this packet so fall thru to... */
	    }

	case 'Y' :						/* ACK */
	  if ( PacketNumber != Number )                         /* If wrong ACK, stay in F state */
	    {
	      RetrieNo++;
	      return( State );
	    }
	  NumTries = 0;						/* Reset try counter */
	  PacketNumber = ( PacketNumber + 1 ) % 64;		/* Bump packet count */
	  if ( ( Size = FillBuffer( Packet ) ) == False )	/* Get first data from file */
	    {
	      ErrorMsg( "Kermit remoto nao aceita arquivo binario" );
	      return( 'A' );
	    }
	  return( 'D' );					/* Switch state to D */

	case 'E' :						/* Error packet received */
	  PrintErrorPacket( ReceivedPacket );			/* Print it out and */
	  return( 'A' );					/* abort */

	case 'A' :
	  AbortMessage();
	  return( 'A' );

	case False :                                            /* Receive failure, stay in F state */
	  RetrieNo++;
	  return( State );

	default :                                               /* Something else, just "abort" */
	  ProtocolError();
	  return( 'A' );
      }
  }


/* ------------------------------------------------------------------------ */

/*

  SendData:
    Send File Data.

*/

static char SendData( void )

  {
    int Number, Length;						/* Packet number, length */

    if ( NumTries++ > MaxTries )                    		/* If too many tries, give up */
      {
	TooManyTries();
	return( 'A' );
      }

    SendPack( 'D', PacketNumber, Size, Packet );		/* Send a D packet */
    switch( ReceivePack( &Length, &Number, ReceivedPacket ) )	/* What was the reply? */
      {
	case 'N' :						/* NAK, just stay in this state, */
	  Number = ( --Number < 0 ? 63 : Number );		/* unless it's NAK for next packet */
	  if ( PacketNumber != Number )				/* which is just like an ACK for */
	    {
	      RetrieNo++;
	      return( State );					/* this packet so fall thru to... */
	    }

	case 'Y' :						/* ACK */
	  if ( PacketNumber != Number )               		/* If wrong ACK, fail */
	    {
	      RetrieNo++;
	      return( State );
	    }
	  NumTries = 0;					        /* Reset try counter */
	  PacketNumber = ( PacketNumber + 1 ) % 64;		/* Bump packet count */
	  if ( ( Size = FillBuffer( Packet ) ) == EOF ) 	/* Get data from file */
	    {
	      if ( ferror( CurrentFile ) )
		{
		  ErrorMsg( "\"%s\" nao pode ser lido", FileName );
		  return( 'A' );
		}
	      else
		return( 'Z' );					/* If EOF set state to that */
	    }
          else if ( Size == False )
	    {
	      ErrorMsg( "Kermit remoto nao aceita arquivo binario" );
	      return( 'A' );
	    }
	  else
	    return( 'D' );					/* Got data, stay in state D */

	case 'E' :						/* Error packet received */
	  PrintErrorPacket( ReceivedPacket );			/* Print it out and */
	  return( 'A' );					/* abort */

	case 'A' :
	  AbortMessage();
	  return( 'A' );

	case False :                                            /* Receive failure, stay in D */
	  RetrieNo++;
	  return( State );

	default :                                               /* Anything else, "abort" */
	  ProtocolError();
	  return( 'A' );
      }
  }


/* ------------------------------------------------------------------------ */

/*

  SendEof:
    Send End-Of-File.

*/

static char SendEof( void )

  {
    int Number, Length;						/* Packet number, length */
    if ( NumTries++ > MaxTries )                                /* If too many tries, "abort" */
      {
	TooManyTries();
	return( 'A' );
      }

    SendPack( 'Z', PacketNumber, 0, NULL );			/* Send a 'Z' packet */
    switch( ReceivePack( &Length, &Number, ReceivedPacket ) )	/* What was the reply? */
      {
	case 'N' :						/* NAK, just stay in this state, */
	  Number = ( --Number < 0 ? 63 : Number );		/* unless it's NAK for next packet, */
	  if ( PacketNumber != Number )				/* which is just like an ACK for */
	    {
	      RetrieNo++;
	      return( State );					/* this packet so fall thru to... */
	    }

	case 'Y' :						/* ACK */
	  if ( PacketNumber != Number )                         /* If wrong ACK, hold out */
	    {
	      RetrieNo++;
	      return( State );
	    }
	  NumTries = 0;						/* Reset try counter */
	  PacketNumber = ( PacketNumber + 1 ) % 64;		/* and bump packet count */
	  fclose( CurrentFile );				/* Close the input file */
	  CurrentFile = NULL;					/* Set flag indicating no file open */

	  if ( GetNextFile() == False )				/* No more files go? */
	    return( 'B' );					/* if not, break, EOT, all done */
	  return( 'F' );					/* More files, switch state to F */

	case 'E' :						/* Error packet received */
	  PrintErrorPacket( ReceivedPacket );			/* Print it out and */
	  return( 'A' );					/* abort */

	case 'A' :
	  AbortMessage();
	  return( 'A' );

	case False :                                            /* Receive failure, stay in Z */
	  RetrieNo++;
	  return( State );

	default :                                               /* Something else, "abort" */
	  ProtocolError();
	  return( 'A' );
      }
  }


/* ------------------------------------------------------------------------ */

/*

  SendBreak:
    Send Break (EOT)

*/

static char SendBreak( void )

  {
    int Number, Length;						/* Packet number, length */
    if ( NumTries++ > MaxTries )                                /* If too many tries "abort" */
      {
	TooManyTries();
	return( 'A' );
      }

    SendPack( 'B', PacketNumber, 0, NULL ); 			/* Send a B packet */
    switch ( ReceivePack( &Length, &Number, ReceivedPacket ) )	/* What was the reply? */
      {
	case 'N' :						/* NAK, just stay in this state, */
	  Number = ( --Number < 0 ? 63 : Number );		/* unless NAK for previous packet, */
	  if ( PacketNumber != Number )				/* which is just like an ACK for */
	    {
	      RetrieNo++;
	      return( State );					/* this packet so fall thru to... */
	    }

	case 'Y' :						/* ACK */
	  if ( PacketNumber != Number )                         /* If wrong ACK, fail */
	    {
	      RetrieNo++;
	      return( State );
	    }
	  NumTries = 0;						/* Reset try counter */
	  PacketNumber = ( PacketNumber + 1 ) % 64;		/* and bump packet count */
	  return( 'C' );					/* Switch state to Complete */

	case 'E' :						/* Error packet received */
	  PrintErrorPacket( ReceivedPacket );			/* Print it out and */
	  return( 'A' );					/* abort */

	case 'A' :
	  AbortMessage();
	  return( 'A' );

	case False :                                            /* Receive failure, stay in B */
	  RetrieNo++;
	  return( State );

	default :                                               /* Other, "abort" */
	  ProtocolError();
	  return( 'A' );
      }
  }

/* ------------------------------------------------------------------------ */

/*

  SendFiles:
    Send the files.

*/


bool SendFiles( int NumberOfFiles, char **Files, bool Server )

  {
    if ( ! Server )
      InitKermit( 'S', True );

    FileCount = NumberOfFiles;
    FileList = Files;
    GetNextFile();

    while ( True )
      {
	switch( State )
	  {
	    case 'S' :
	      State = SendInit();
	      break;

	    case 'F' :
	      State = SendFile();
	      break;

	    case 'D' :
	      State = SendData();
	      break;

	    case 'Z' :
	      State = SendEof();
	      break;

	    case 'B' :
	      State = SendBreak();
	      break;

	    case 'C' :						/* Complete */
	      if ( ! Server )
		FinishKermit( True );
	      return( True );

	    case 'A' :  					/* Abort */
	      if ( ! Server )
		FinishKermit( False );
	      return( False );

	    default :   					/* Unknown, so fail */
	      if ( ! Server )
		FinishKermit( False );
	      return( False );
	  }
      }
    assert( False );
    return( True );  						/* Unreachable code */
  }


/* ------------------------------------------------------------------------ */

/*

  ReceiveInit:
    Receive Initialization

*/

static char ReceiveInit( void )

  {
    int Number, Length;

    if ( NumTries++ > MaxTries )        			/* If too many tries, "abort" */
      {
	TooManyTries();
	return( 'A' );
      }

    switch( ReceivePack( &Length, &Number, ReceivedPacket ) )
      {
	case 'S' :			                        /* Send-Init */
          GetPars( ReceivedPacket, Length );            	/* Get the other side's init data */
          SetPars( Packet );                            	/* Fill up packet with my init info */
	  SendPack( 'Y', PacketNumber, 7, Packet );     	/* ACK with my parameters */
          OldTry = NumTries;                                    /* Save old try count */
          NumTries = 0;                                         /* Start a new counter */
          PacketNumber = ( PacketNumber + 1 ) % 64;             /* Bump packet number, mod 64 */
          return( 'F' );                                        /* Enter File-Receive state */

	case 'E' :			                        /* Error packet received */
	  PrintErrorPacket( ReceivedPacket );		        /* Print it out and */
	  return( 'A' );		                        /* ... abort */

	case 'A' :
	  AbortMessage();
	  return( 'A' );

	case False :			                        /* Didn't get packet */
	  RetrieNo++;
	  SendPack( 'N', PacketNumber, 0, NULL );		/* Return a NAK */
	  return( State );		                        /* Keep trying */

	default :       	                                /* Some other packet type, "abort" */
	  ProtocolError();
	  return( 'A' );
      }
  }


/* ------------------------------------------------------------------------ */

/*

  ReceiveFile:
    Receive File Header

*/

static char ReceiveFile( void )

  {
    int Number, Length;

    if ( NumTries++ > MaxTries )				/* "abort" if too many tries */
      {
	TooManyTries();
	return( 'A' );
      }

    switch( ReceivePack( &Length, &Number, ReceivedPacket ) )
      {
	case 'S' :  						/* Send-Init, maybe our ACK lost */
	  if ( OldTry++ > MaxTries )   				/* If too many tries "abort" */
	    {
	      TooManyTries();
	      return( 'A' );
	    }
	  if ( Number == ( ( PacketNumber == 0 ) ? 63 : PacketNumber - 1 ) )
	    {							/* Yes, ACK it again with  */
	      SetPars( Packet );				/* our Send-Init parameters */
	      SendPack( 'Y', Number, 7, Packet );
	      NumTries = 0;					/* Reset try counter */
	      return( State );					/* Stay in this state */
	    }
	  else
	    {
	      ProtocolError();
	      return('A');					/* Not previous packet, "abort" */
	    }

	case 'Z' :						/* End-Of-File */
	  if ( OldTry++ > MaxTries )   				/* If too many tries "abort" */
	    {
	      TooManyTries();
	      return( 'A' );
	    }
	  if ( Number == ( ( PacketNumber == 0 ) ? 63 : PacketNumber - 1 ) )  /* Previous packet ? */
	    {							/* Yes, ACK it again. */
	      SendPack( 'Y', Number, 0, NULL );
	      NumTries = 0;
	      return( State );					/* Stay in this state */
	    }
	  else
	    {
	      ProtocolError();
	      return( 'A' );					/* Not previous packet, "abort" */
	    }

	case 'F' :						/* File Header (just what we want) */
	  if ( Number != PacketNumber )                 	/* The packet number must be right */
	    {
	      ProtocolError();
	      return( 'A' );
	    }

	  if ( ( CurrentFile = fopen( ReceivedPacket, "rb" ) ) != NULL )  /* Check for the existance of the file */
	    {
	      ErrorMsg( "\"%s\" ja existe", ReceivedPacket );
	      fclose( CurrentFile );
	      CurrentFile = NULL;
	      return( 'A' );
	    }
	  else if ( ( CurrentFile = fopen( ReceivedPacket, "wb" ) ) == NULL )  /* Try to open a new file */
            {
	      ErrorMsg( "\"%s\" nao pode ser criado", ReceivedPacket );      /* Give up if can't */
	      return( 'A' );
	    }
	  else							/* OK, give message */
	    {
	      strcpy( CurrFileName, ReceivedPacket );
	      gotoxy( 37, 10 );
	      cprintf( "%-12.12s", ReceivedPacket );
	    }

	  SendPack( 'Y', PacketNumber, 0, NULL );		/* Acknowledge the file header */
	  OldTry = NumTries;					/* Reset try counters */
	  NumTries = 0;						/* ... */
	  PacketNumber = ( PacketNumber + 1 ) % 64;		/* Bump packet number, mod 64 */
	  return( 'D' );					/* Switch to Data state */

	case 'B' :						/* Break transmission (EOT) */
	  if ( Number != PacketNumber )                 	/* Need right packet number here */
	    {
	      ProtocolError();
	      return( 'A' );
	    }
	  SendPack( 'Y', PacketNumber, 0, NULL );		/* Say OK */
	  return( 'C' );					/* Go to complete state */

	case 'E' :						/* Error packet received */
	  PrintErrorPacket( ReceivedPacket );			/* Print it out and */
	  return( 'A' );					/* abort */

	case 'A' :
	  AbortMessage();
	  return( 'A' );

	case False :						/* Didn't get packet */
	  RetrieNo++;
	  SendPack( 'N', PacketNumber, 0, NULL );		/* Return a NAK */
	  return( State );					/* Keep trying */

	default :
	  ProtocolError();
	  return( 'A' );					/* Some other packet, "abort" */
      }
  }


/* ------------------------------------------------------------------------ */

/*

  ReceiveData:
    Receive Data

*/


static char ReceiveData( void )

  {
    int Number, Length;

    if ( NumTries++ > MaxTries )				/* If too many tries, give up */
      {
	TooManyTries();
	return( 'A' );
      }

    switch( ReceivePack( &Length, &Number, ReceivedPacket ) )
      {
	case 'D' :						/* Got Data packet */
	  if ( Number != PacketNumber )				/* Right packet? */
	    {							/* No */
	      if ( OldTry++ > MaxTries )
		{
		  TooManyTries();
		  return( 'A' );
		}
              if ( Number == ( ( PacketNumber == 0 ) ? 63 : PacketNumber - 1 ) )   /* Else check packet number */
		{						/* Previous packet again? */
		  SendPack( 'Y', Number, 0, NULL );   		/* Yes, re-ACK it */
		  NumTries = 0;					/* Reset try counter */
		  return( State );				/* Don't write out data! */
		}
	      else
		{
		  ProtocolError();
		  return( 'A' );					/* sorry, wrong number */
		}
	    }
								/* Got data with right packet number */
	  if ( ! WritePack( ReceivedPacket, Length ) )		/* Write the data to the file */
	    {
	      DiskFull();
	      return( 'A' );
	    }
	  SendPack( 'Y', PacketNumber, 0, NULL );		/* Acknowledge the packet */
	  OldTry = NumTries;					/* Reset the try counters */
	  NumTries = 0;						/* ... */
	  PacketNumber = ( PacketNumber + 1 ) % 64;		/* Bump packet number, mod 64 */
	  return( 'D' );					/* Remain in data state */

	case 'F' :						/* Got a File Header */
	  if ( OldTry++ > MaxTries )
	    {
	      TooManyTries();
	      return( 'A' );
	    }
          if ( Number == ( ( PacketNumber == 0 ) ? 63 : PacketNumber - 1 ) )  /* Else check packet number */
	    {							/* It was the previous one */
	      SendPack( 'Y', Number, 0, NULL );			/* ACK it again */
	      NumTries = 0;					/* Reset try counter */
	      return( State );					/* Stay in Data state */
	    }
	  else
	    {
	      ProtocolError();
	      return( 'A' );					/* Not previous packet, "abort" */
	    }

	case 'Z' :						/* End-Of-File */
	  if ( Number != PacketNumber )                    	/* Must have right packet number */
	    {
	      ProtocolError();
	      return( 'A' );
	    }
	  if ( fclose( CurrentFile ) == EOF )			/* Close the file */
	    {
	      CurrentFile = NULL;
	      DiskFull();
	      return( 'A' );
	    }
	  SendPack( 'Y', PacketNumber, 0, NULL );		/* OK, ACK it. */
	  CurrentFile = NULL;
	  PacketNumber = ( PacketNumber + 1 ) % 64;		/* Bump packet number */
	  return( 'F' );					/* Go back to Receive File state */

	case 'E' :						/* Error packet received */
	  PrintErrorPacket( ReceivedPacket );			/* Print it out and */
	  return( 'A' );					/* abort */

	case 'A' :
	  AbortMessage();
	  return( 'A' );

	case False :						/* Didn't get packet */
	  RetrieNo++;
	  SendPack( 'N', PacketNumber, 0, NULL );		/* Return a NAK */
	  return( State );					/* Keep trying */

	default :
	  ProtocolError();
	  return( 'A' );					/* Some other packet, "abort" */
      }
  }


/* ------------------------------------------------------------------------ */

/*

  ReceiveFiles:
    Receive the files.

*/

bool ReceiveFiles( char *FileMask, bool Server )

  {
    if ( ! Server )
      InitKermit( 'R', True );

    while( True )
      {
	switch( State )						/* Do until done */
	  {
	    case 'R' :   					/* Receive-Init */
	      if ( FileMask != NULL )
		SendPack( 'R', 0, strlen( FileMask ), FileMask );
	      State = ReceiveInit();
	      break;

	    case 'F' :   					/* Receive-File */
	      State = ReceiveFile();
	      break;

	    case 'D' :   					/* Receive-Data */
	      State = ReceiveData();
	      break;

	    case 'C' :   					/* Complete state */
	      if ( ! Server )
		FinishKermit( True );
	      return( True );

	    case 'A' :   					/* "Abort" state */
	      if ( CurrentFile != NULL )
		{
		  fclose( CurrentFile );
		  CurrentFile = NULL;
		  unlink( CurrFileName );
		}
	      if ( ! Server )
		FinishKermit( False );
	      return( False );
	  }
      }
    assert( False );
    return( True );						/* Unreachable code */
  }



/* ------------------------------------------------------------------------ */

/*

  Server:
    Puts the SDKermit into server-mode

*/

void Server( void )

  {
    int Number, Length;
    char **Files;
    int NoOfFiles;

    InitKermit( 'V', True );
    State = 'R';

    while( True )
      {
	switch( State )						/* Do until done */
	  {
	    case 'R' :
	      switch( ReceivePack( &Length, &Number, ReceivedPacket ) )
		{
		  case 'S' :			                /* Send-Init */
		    GetPars( ReceivedPacket, Length );          /* Get the other side's init data */
		    SetPars( Packet );                          /* Fill up packet with my init info */
		    SendPack( 'Y', PacketNumber, 7, Packet );   /* ACK with my parameters */
		    OldTry = NumTries;                          /* Save old try count */
		    NumTries = 0;                               /* Start a new counter */
		    PacketNumber = ( PacketNumber + 1 ) % 64;   /* Bump packet number, mod 64 */
		    ShowDoing( 'R' );
		    State = 'F';
		    ReceiveFiles( NULL, True );
		    State = 'C';
		    break;

		  case 'R' :
		    if ( ( Files = ExpandFile( ReceivedPacket, &NoOfFiles ) ) == NULL )
		      ErrorMsg( "\"%s\" nao encontrado", ReceivedPacket );
		    else
		      {
			ShowDoing( 'S' );
			State = 'S';
			SendFiles( NoOfFiles, Files, True );
			do
			  free( Files[ --NoOfFiles ] );
			while( NoOfFiles != 0 );
			free( Files );
		      }
		    State = 'C';
		    break;

		  case 'G' :
		    if ( strchr( "FL", *ReceivedPacket ) == NULL )
		      ErrorMsg( "Comando nao implementado" );
		    else
		      {
			SendPack( 'Y', Number, 0, NULL ); /* ACK it */
			State = 'L';
		      }
		    break;

		  case 'E':
		  case False :
		    break;

		  case 'A' :
		    WriteMsg( 10, 10, "Aviso", "Servidor terminado pelo usuario" );
		    State = 'A';
		    break;

		  default :
		    ErrorMsg( "Comando remoto nao implementado" );
		    break;
		}
	      break;

	    case 'C' :   					/* Complete state */
	      InitKermit( 'V', False );
	      State = 'R';
	      break;

	    case 'L' :						/* Logout state */
	      WriteMsg( 10, 10, "Aviso", "Servidor terminado pelo kermit remoto" );
	      FinishKermit( False );
	      return;

	    case 'A' :
	      FinishKermit( False );
	      return;
	  }
      }
    assert( False );
  }



/* ------------------------------------------------------------------------ */

/*

  Finish:
    Turns off the server

*/

bool Finish( void )

  {
    int Length, Number;

    InitKermit( 'G', False );
    SendPack( 'G', 0, 1, "F" );
    return( ReceivePack( &Length, &Number, ReceivedPacket ) == 'Y' );
  }



/* ------------------------------------------------------------------------ */

/*

  Logout:
    Tells "BYE...." to the server

*/

bool Logout( void )

  {
    int Length, Number;

    InitKermit( 'G', False );
    SendPack( 'G', 0, 1, "L" );
    return( ReceivePack( &Length, &Number, ReceivedPacket ) == 'Y' );
  }

/* ------------------------------------------------------------------------ */
