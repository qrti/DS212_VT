/********************* (C) COPYRIGHT 2017 e-Design Co.,Ltd. ********************
 File Name : file.h  
 Version   : DS212                                                Author : bure
*******************************************************************************/
#ifndef __FILE_H
#define __FILE_H

#include "STM32F30x.h"

extern u8 Cal_Flag;

enum { MAIN, RTIM, MODE, VOLA, VOLB, COUP };                        // main, timebase, mode, volt A, B, coupling
enum { TIME, NSCR, VSCR, OFFA, OFFB, MAGN, VCHN, BRIG };            // time, number of screens, visible screen, offset, magnification, visible channel, brightness

#define MAXITEM     8

typedef struct control{
    u8* cap[MAXITEM];
    s16 def[MAXITEM];
    s16 val[MAXITEM];
    s16 min[MAXITEM];
    s16 max[MAXITEM];
    s16 cha[MAXITEM];
    s8 numItm;
    s8 sel;
    u8** itx[MAXITEM];
}CONTROL;

void Read_CalFlag(void);
u8 Save_Kpg(void);
void Read_Kpg(void);
u8 Save_Param(void);
u8 Load_Param(void);
u8 Save_Bmp(void);
u8 colorNum(u16);

#endif

/********************************* END OF FILE ********************************/
