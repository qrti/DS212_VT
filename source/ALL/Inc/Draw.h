/********************* (C) COPYRIGHT 2017 e-Design Co.,Ltd. *******************
 File Name : Draw.h
 Version   : DS212                                                Author : bure
******************************************************************************/
#ifndef __DRAW_H
#define __DRAW_H

#include "STM32F30x.h"

//========== Waveform display window related definitions ===========
#define X_SIZE     300
#define Y_SIZE     200

//================= Display method =================
#define PRN        0x00
#define INV        0x10   /* Bit4: 0/1 = NORM/INV   normal/contrast*/
#define SYMB       0x20   /* Bit5: O/1 = CHAR/SYMB  character/blocks*/
#define VOID       0x40   /* Bit6: 0/1 = REAL/VOID  solid line/dotted line*/

//============= Palette color definition ===============
#define CYN        0xFFE0  /* 0*/
#define PUR        0xF81F  /* 1*/
#define YEL        0x07FF  /* 2*/
#define GRN        0x07E0  /* 3*/
#define CYN_       0xBDE0  /* 4*/
#define PUR_       0xB817  /* 5*/
#define YEL_       0x05F7  /* 6*/
#define GRN_       0x05E0  /* 7*/
#define ORN        0x051F  /* 8*/
#define BLK        0x0000  /* 9*/
#define WHT        0xFFFF  /* 10*/
#define BLU        0xFC10  /* 11*/
#define RED        0x001F  /* 12*/
#define GRY        0x7BEF  /* 13*/
#define LGN        0x27E4  /* 14*/
#define DAR        0x39E7  /* 15*/
#define GRAY       0x7BEF
#define RED_       0x631F

//=========== Display window flag definition =============
#define SHOW        0x00       /* All Show*/
#define D_HID       0x01       /* Endp Hide*/
#define L_HID       0x02       /* Line Hide*/
#define W_HID       0x04       /* Wave Hide*/
#define B_HID       0x06       /* Line & Wave Hide*/
#define A_HID       0x07       /* End Dot & Line & Wave Hide*/

//=========== Pop-up subwindow mark definition =============
#define P_HID       0x01       /* Pop Hide*/

//============== Display parameter definition ==================
#define CCM_ADDR   0x10000000
#define TR1_pBUF   500
#define TAB_PTR    410

extern u16  Background, Foreground, Sx, Sy;
extern uc16 CHAR10x14[], CHAR8x14[], CHAR8x9[];
extern uc8  CHAR6x8[];

void __DrawWindow(u8* VRAM_Addr);
void SetColor(u16 Board_Color, u16 Text_Color);
void DispStr(u16 x0,  u16 y0,  u8 Mode, u8 *Str);
void DispChar(u8 Mode, u8 Code);
void DispChar10x14(u8 Mode, u8 Code);
void DispChar8x9(u8 Mode, u8 Code);
void DispStr10x14(u16 x0,  u16 y0,  u8 Mode, u8 *Str);
void DispChar6x8(u8 Mode, u8 Code);
void DispStr8x9(u16 x0, u16 y0, u8 Mode, u8 *Str);
u16  Get_TAB_8x9(u8 Code, u16 Row);
u16  Get_TAB_6x8(u8 Code, u16 Row);
u16  Get_TAB_10x14(u8 Code, u16 Row);
void DispStr6x8(u16 x0, u16 y0, u8 Mode, u8 *Str);
void PrintStr6x8(u8 Mode, u8 *Str);

void Draw_Circle_S(u16 Col, u16 Posi_x, u16 Posi_y, u16 High, u16 Width);
void Draw_Circle_D(u16 Col, u16 Posi_x, u16 Posi_y, u16 High, u16 Width, u16 Distance);
void Draw_Rectangle(u16 Col, u16 Posi_x, u16 Posi_y, u16 High, u16 Width);
void Draw_RECT(u16 Col, u16 Posi_x, u16 Posi_y, u16 High, u16 width, u8 R);

#endif
/********************************* END OF FILE ********************************/
