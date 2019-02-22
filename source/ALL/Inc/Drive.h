/********************* (C) COPYRIGHT 2017 e-Design Co.,Ltd. *******************
 Brief   : Low-level hardware configuration                     Author : bure
******************************************************************************/
#ifndef __DRIVE_H
#define __DRIVE_H

#include "STM32F30x.h"

//============================================================================//

extern u8 StateA, StateB, CouplA, CouplB, GainA, GainB, KindA, KindB;
extern u16 KeyIn, Vbat, Ichg, Key_Exit;
extern u8 Key_UPD;
extern u16 Key_S_Time, Vb_Sum, Vbattrey;
extern u8 Battery;

#define OFF             0
#define ON              1

//----------------------------- key code define ------------------------------//

#define K_RUN           0x02     //       RUN key             
#define K_M             0x04     // Bit2    M key
#define K_UP            0x08     // Bit3    U key
#define K_DOWN          0x10     // Bit4    D key
#define K_LEFT          0x20     // Bit5    L key
#define K_RIGHT         0x40     // Bit6    R key
#define K_S             0x80     // Bit7    S key

#define KEY_DOUBLE_M    0x4000   // M double click
#define KEY_DOUBLE_S    0x8000   // S double click

#define R_HOLD          0x0200   // RUN hold
#define S_HOLD          0x2000   // M press  
#define M_HOLD          0x1000   // S press

//----------------------------------------------------------------------------//

void USB_MSD_Config(void);
void Drive(u8 Device, u32 Value);
void SysTick_ISP(void);
void Wait_mS(u16 mS);
void AiPosi(u8 Val);
void BiPosi(u8 Val);
void Beep(u16 mS);
void Keys_Detect(void);

void Battery_update(u16, u16);
u16  Bat_Vol(void);

void __USB_Port(u8 NewState);
void TIM6_ISP(void);
void GPIO_SWD_NormalMode(void);

#endif

/********************************* END OF FILE ********************************/
