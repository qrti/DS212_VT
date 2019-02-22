/******************** (C) COPYRIGHT 2017 e-Design Co.,Ltd. ********************
 File Name : Process.h
 Version   : DS202 Ver 1.0                                        Author : bure
******************************************************************************/
#ifndef __PROEESS_H
#define __PROEESS_H

#include "STM32F30x.h"

extern u16  Smpl[];
extern uc8  GK[];
extern s16  *KpA, *KpB, *KgA, *KgB, Kpg[];
extern u8*  V_Buf;

void Zero_Align(void);
void initChannels(u8, u8, u8, u8);

#endif /*__PROEESS_H*/

/******************************** END OF FILE *********************************/
