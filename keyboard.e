/* ---------------------------------------------------------------------- */

#ifndef _KEYBOARD_E_
#define _KEYBOARD_E_

#ifndef _TYPES_E_
#include <types.e>
#endif

/* ---------------------------------------------------------------------- */

/* KeyNames */

#define KeyHome        (71 << 8)
#define KeyUp          (72 << 8)
#define KeyPgUp        (73 << 8)
#define KeyLeft        (75 << 8)
#define KeyRight       (77 << 8)
#define KeyEnd         (79 << 8)
#define KeyDown        (80 << 8)
#define KeyPgDn        (81 << 8)

#define KeyCtrlHome    (119 << 8)
#define KeyCtrlPgUp    (132 << 8)
#define KeyCtrlLeft    (115 << 8)
#define KeyCtrlRight   (116 << 8)
#define KeyCtrlEnd     (117 << 8)
#define KeyCtrlPgdn    (118 << 8)

#define KeyF1  (59 << 8)
#define KeyF2  (60 << 8)
#define KeyF3  (61 << 8)
#define KeyF4  (62 << 8)
#define KeyF5  (63 << 8)
#define KeyF6  (64 << 8)
#define KeyF7  (65 << 8)
#define KeyF8  (66 << 8)
#define KeyF9  (67 << 8)
#define KeyF10 (68 << 8)

#define KeyCtrlF1  (94 << 8)
#define KeyCtrlF2  (95 << 8)
#define KeyCtrlF3  (96 << 8)
#define KeyCtrlF4  (97 << 8)
#define KeyCtrlF5  (98 << 8)
#define KeyCtrlF6  (99 << 8)
#define KeyCtrlF7  (100 << 8)
#define KeyCtrlF8  (101 << 8)
#define KeyCtrlF9  (102 << 8)
#define KeyCtrlF10 (103 << 8)

#define KeyAltF1  (104 << 8)
#define KeyAltF2  (105 << 8)
#define KeyAltF3  (106 << 8)
#define KeyAltF4  (107 << 8)
#define KeyAltF5  (108 << 8)
#define KeyAltF6  (109 << 8)
#define KeyAltF7  (110 << 8)
#define KeyAltF8  (111 << 8)
#define KeyAltF9  (112 << 8)
#define KeyAltF10 (113 << 8)

#define KeyCtrlA 1
#define KeyCtrlB 2
#define KeyCtrlC 3
#define KeyCtrlD 4
#define KeyCtrlE 5
#define KeyCtrlF 6
#define KeyCtrlG 7
#define KeyCtrlH 8
#define KeyCtrlI 9
#define KeyCtrlJ 10
#define KeyCtrlK 11
#define KeyCtrlL 12
#define KeyCtrlM 13
#define KeyCtrlN 14
#define KeyCtrlO 15
#define KeyCtrlP 16
#define KeyCtrlQ 17
#define KeyCtrlR 18
#define KeyCtrlS 19
#define KeyCtrlT 20
#define KeyCtrlU 21
#define KeyCtrlV 22
#define KeyCtrlW 23
#define KeyCtrlX 24
#define KeyCtrlY 25
#define KeyCtrlZ 26

#define KeyEsc   27

#define KeyAltA  0x1e00
#define KeyAltB  0x3000
#define KeyAltC  0x2e00
#define KeyAltD  0x2000
#define KeyAltE  0x1200
#define KeyAltF  0x2100
#define KeyAltG  0x2200
#define KeyAltH  0x2300
#define KeyAltI  0x1700
#define KeyAltJ  0x2400
#define KeyAltK  0x2500
#define KeyAltL  0x2600
#define KeyAltO  0x4f00
#define KeyAltP  0x1900
#define KeyAltQ  0x1000
#define KeyAltR  0x1300
#define KeyAltS  0x1F00
#define KeyAltT  0x1400
#define KeyAltU  0x1600
#define KeyAltV  0x2f00
#define KeyAltW  0x1100
#define KeyAltX  0x2d00
#define KeyAltY  0x1500
#define KeyAltZ  0x2c00

/* ---------------------------------------------------------------------- */

void InitKeyboard( void );
void CloseKeyboard( void );

void ( *SetCtrlBreakHandler( void ( *OldHandler )( void ) ) ) ( void );
uint16 KeyPressed( void );
void UnGetCh( uint16 Key );
void UnGetChs( uint16 *Keys );
uint16 GetCh( void );
uint16 GetChScan( void );

/* ---------------------------------------------------------------------- */

#endif

/* ---------------------------------------------------------------------- */
