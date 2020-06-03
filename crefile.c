#include <stdio.h>

main()

  {
    FILE *fp = fopen( "DATAFILE.DTA", "wb" );
    int i;

    for ( i = 0; i <= 255; i++ )
      fputc( i, fp );
    fclose( fp );
  }
