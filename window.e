/* -------------------------------------------------------------------- */

#ifndef _WINDOW_E_
#define _WINDOW_E_

/* -------------------------------------------------------------------- */

typedef struct
  {
    struct text_info TextInfo;
    int Left, Upper, Right, Lower;
    void *SavedScreen;
  } tWindowInfo;

/* -------------------------------------------------------------------- */

tWindowInfo *OpenWindow( int Left, int Upper, int Right, int Lower,
			 int BoxKind, char *Title,
			 char *BoxAttr, char *BordAttr, char *TitleAttr );

void CloseWindow( tWindowInfo *WindowInfo );

/* -------------------------------------------------------------------- */

#endif

/* -------------------------------------------------------------------- */
