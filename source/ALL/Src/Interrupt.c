/********************* (C) COPYRIGHT 2017 e-Design Co.,Ltd. *******************/
/* Brief : Interrupt Service Routines                           Author : bure */
/******************************************************************************/
#include "Interrupt.h"
#include "usb_istr.h"
#include "Drive.h"
#include "Bios.h"

/******************************************************************************/
/*            Cortex-M4 Processor Exceptions Handlers                         */
/******************************************************************************/
u16 AutoPwr_Cnt, PD_Cnt, BeepCnt, Key_Wait_Cnt, Key_Repeat_Cnt;
vu16 KeymS_Cnt;
u8 KeymS_F = 0;
u32 sysTics = 0;

void NMI_Handler(void){;}

void HardFault_Handler(void) {while (1);}

void MemManage_Handler(void) {while (1);}

void BusFault_Handler(void)  {while (1);}

void UsageFault_Handler(void){while (1);}

void SVC_Handler(void){}

void DebugMon_Handler(void){}

void PendSV_Handler(void){}

void SysTick_Handler(void)
{
  SysTick_ISP();
  sysTics++;
}

void TIM6_DAC_IRQHandler(void)
{
  //TIM_ClearITPendingBit(TIM6, TIM_IT_Update ); 
  //TIM6_ISP();
}

/******************************************************************************/
/*                 STM32F30x Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f30x.s).                                               */
/******************************************************************************/

void USB_LP_CAN1_RX0_IRQHandler(void)
{
  USB_Istr();
}

/*********************************  END OF FILE  ******************************/

