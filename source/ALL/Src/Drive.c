/********************* (C) COPYRIGHT 2017 e-Design Co.,Ltd. *******************
  Brief   : Low-level hardware configuration
 Version  : DS212                                                Author : bure
******************************************************************************/
#include "Interrupt.h"
#include "Version.h"
#include "usb_lib.h"
#include "Process.h"
#include "Drive.h"
#include "Draw.h"
#include "Func.h"
#include "Bios.h"
#include "Disk.h"
#include "LCD.h"
#include "STM32F30x.h"

//================+=======+===========+============+===========+===========+==========+======+
// Battery status | Alarm | All empty | Half empty | Half full | Full full | Charging | Full |
//----------------+-------+-----------+------------+-----------+-----------+----------+------+
uc8  BT_S[][5] = { ";<=",  ";<=",      ";&@",       ";*@",      ">`@",      "\"#$",     "\"#$" };   // symbol string
uc16 V_BT[]    = {  3250,   3300,       3400,        3700,       3900,       4200,       4200  };   // voltage value
uc16 BT_C[]    = {  RED,    YEL,        GRN,         GRN,        GRN,        PUR,        GRN   };   // color

u16  Key_S_Time = 300;
u16 KeyIn, Vbat, Ichg;
vu32 WaitCnt, SysCnt;
u8 StateA, StateB, CouplA, CouplB, GainA, GainB, KindA, KindB;
u16 Tmp_RUN = 0, Tmp_S = 0, Tmp_M = 0;
u8 Key_UPD = 0;
u8 StdBy_Flag = 0;
u8 StdBy_Key = 0;
u16 Key_Exit = 0;
u16 H_EncdLS=0, H_EncdSt=0;
u16 V_EncdLS=0, V_EncdSt=0;
u16 Vb_Sum=4200*16/2, Vbattrey;
u8 Battery = 0;
u16 KeyS_Dly = 0;
u16 KeyM_Dly = 0;

/*******************************************************************************
 Wait_mS: Delay function Input: ms value of delay (in case of 72MHz frequency)
*******************************************************************************/
void Wait_mS(u16 mS) // Input value is 0-65536 mS
{
  WaitCnt = mS;
  while(WaitCnt){};
}
/*******************************************************************************
 SysTick_ISP:  SysTick timer interrupt handler
*******************************************************************************/
void SysTick_ISP(void)
{
    u16 i;

    if(WaitCnt) 
        WaitCnt--;
    
    if(KeymS_F)                                                 // starting timing after single click
        KeymS_Cnt++;                    

    if(KeyS_Dly)    
        KeyS_Dly--; 

    if(KeyM_Dly)    
        KeyM_Dly--; 

    if(SysCnt % 20 == 0){   
        if(SysCnt == 1000){ 
            SysCnt = 0; 

            for(i=0; i<4; i++)                                  // power detection
                if(Vbat <= V_BT[i])     
                    break;  

            if(__Info(P_VUSB))  
                Battery = 5;    
            else            
                Battery = i;    
        }   

        Vb_Sum += __Info(P_VBAT) - Vb_Sum / 16;                 // power detection
        Vbat = Bat_Vol();   

        if(Key_Wait_Cnt)        
            Key_Wait_Cnt--; 

        if(Key_Repeat_Cnt)      
            Key_Repeat_Cnt--;   

        Key_UPD = 1;                                            // key scan sign

        if(BeepCnt > 40) 
            BeepCnt-= 40;
        else             
            __Ctrl(BUZZ_ST, DISABLE);
    }

    if(++SysCnt % 2 == 0){
        if(KeyM_Dly == 0){
            V_EncdSt = ~GPIOB->IDR & (KU_BIT | KD_BIT);         // read encoder status

            if((V_EncdSt & ~V_EncdLS) & KU_BIT){
                if(V_EncdSt & KD_BIT) 
                    Key_Exit = K_UP;                            // encoder up action
                else                  
                    Key_Exit = K_DOWN;                          // encoder down action
            }

            V_EncdLS = V_EncdSt;                                // save encoder status
        }

        if(KeyS_Dly == 0){
            H_EncdSt = ~GPIOB->IDR & (KL_BIT | KR_BIT);         // read encoder status

            if((H_EncdSt & ~H_EncdLS) & KL_BIT){
                if(H_EncdSt & KR_BIT) 
                    Key_Exit = K_LEFT;                          // encoder action
                else                  
                    Key_Exit = K_RIGHT;                         // encoder action
            }
            
            H_EncdLS = H_EncdSt;                                // save encoder status
        }
    }
}

/*******************************************************************************
 Key status detect                                             Return: KeyIn
*******************************************************************************/
void Keys_Detect(void)
{
  static u16 Key_Last;
  u16 Key_Now = 0,KeyCode = 0;

  if(Key_UPD == 1){
    Key_UPD = 0;
    Key_Now = __Info(KEY_IN);
    if(Key_Now &(~Key_Last)){
      Key_Wait_Cnt   = 30;                            // Reset duration button 0.6s count
      Key_Repeat_Cnt = 3;                             // Set 60ms auto repeat period.
      //if(Key_Now & 0x01)  Tmp_PWR = K_PWR;
      if(Key_Now & 0x02)  Tmp_RUN = K_RUN;
      if(Key_Now & 0x80)  Tmp_S   = K_S;
      if(Key_Now & 0x01)  //Tmp_S = K_S;
      {
        if(KeymS_F){                                  // OK double-click
              KeymS_F = 0;                            // KeymS_cnt timing after single clicking the post-clocking flag.
              if(KeymS_Cnt < Key_S_Time){             // Considered as "double click" if press twice in KEYTIME milliseconds.
                KeyCode = KEY_DOUBLE_M;
              }else {
                KeyCode = K_M;
              }
              KeymS_Cnt = 0;
          }else{                                      // OK double-click
              KeymS_Cnt = 0;
              KeymS_F = 1;
              Tmp_M = K_M;
          }
      }


    }
    else{
      KeyCode = Key_Exit;

      if(Key_Now & Key_Last){
        if((Key_Wait_Cnt == 0)&&(Key_Repeat_Cnt == 0)){
          if(Key_Now & 0x80){KeyCode = S_HOLD,Tmp_S = 0;}
          if(Key_Now & 0x02){KeyCode = R_HOLD,Tmp_RUN = 0;}
          if(Key_Now & 0x01){KeyCode = M_HOLD,Tmp_M = 0;KeymS_F = 0; KeymS_Cnt = 0; }
          if(Key_Now & 0x83) Key_Repeat_Cnt = 150;      // Set 3.0s auto repeat period.
        }
      }else {
        //if(Tmp_S)   {KeyCode = Tmp_S;   Tmp_S = 0;}
        if(Tmp_S)   {KeyCode = Tmp_S;   Tmp_S  = 0;}
        if(Tmp_RUN) {KeyCode = Tmp_RUN; Tmp_RUN = 0;}

        if(Tmp_M &&(KeymS_F)&& (KeymS_Cnt > Key_S_Time)){
          KeyCode = Tmp_M;
          Tmp_M  = 0;
          KeymS_F = 0;
          KeymS_Cnt = 0;
        }

        Key_Wait_Cnt=30;
      }
    }
    Key_Last = Key_Now;
    KeyIn = KeyCode;
    Key_Exit = 0;

    if(KeyIn == K_M)KeyM_Dly = 200;
    if(KeyIn == K_S)KeyS_Dly = 100;
  }
}

/*******************************************************************************
 Battery level update
*******************************************************************************/
void Battery_update(u16 x, u16 y)
{
    u8 str[5];

    SetColor(BLK, BT_C[Battery]);
    memcpy(&str[0], &BT_S[Battery], 4);
    DispStr6x8(x, y, SYMB, str);
}

/*******************************************************************************
 Bat_Vol: Detect battery voltage
*******************************************************************************/
u16 Bat_Vol(void)
{
    u16 Tmp = Vb_Sum * 2 / 16;

    if(Vbattrey>Tmp+10 || Vbattrey<Tmp-10) 
        Vbattrey = Tmp;                                 // debounce

    return Vbattrey;
}

/*******************************************************************************
 DevCtrl:  Hardware device control
*******************************************************************************/
void AiPosi(u8 Val)
{
    __Ctrl(AOFFSET, (((u8)Val-100)*1024*GK[GainA]) / KgA[KindA+(StateA?1:0)] + KpA[KindA+(StateA?1:0)]);
}

void BiPosi(u8 Val)
{
    __Ctrl(BOFFSET, (((u8)Val-100)*1024*GK[GainB]) / KgB[KindB+(StateB?1:0)] + KpB[KindB+(StateB?1:0)]);
}

void Beep(u16 mS)
{
    __Ctrl(BUZZ_ST, DISABLE);
    BeepCnt = mS;
    __Ctrl(BUZZVOL, 5 * 10);                                // Set the buzzer volume(0~100%)
    __Ctrl(BUZZ_ST, ENABLE); 
}

/*******************************************************************************
  Mass Storge Device Disk Config
*******************************************************************************/
void USB_MSD_Config(void)
{
  NVIC_InitTypeDef  NVIC_InitStructure;
  SPI_InitTypeDef   SPI_InitStructure;
  GPIO_InitTypeDef  GPIO_InitStructure;

  SCI_CLK_HIGH();
  GPIOA->MODER  &= 0xFF3FFFFF;
  GPIOA->MODER  |= 0x00400000;
  SCI_DIO_LOW();
  GPIOA->MODER  &= 0xFCFFFFFF;
  GPIOA->MODER  |= 0x02000000;
  GPIOA->OTYPER |= 0x1000;      // Open-Drain Output
  __Ctrl(DELAYmS, 100);         // Detect key state after 1 second delay
  SCI_CLK_HIGH();
  SCI_DIO_HIGH();

  GPIOA->BSRR    = USB_DN;
  GPIOA->MODER  &= 0xFF3FFFFF;
  GPIOA->MODER  |= 0x00800000;

  GPIOA->BSRR    = USB_DP;
  GPIOA->MODER  &= 0xFCFFFFFF;
  GPIOA->MODER  |= 0x02000000;
  GPIOA->OTYPER &= 0xEFFF;      // Push-Pull Output

  GPIO_InitStructure.GPIO_Pin   = SPI_SCK | SPI_MISO | SPI_MOSI;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin   = SPI_CS;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
  GPIO_Init(GPIOD, &GPIO_InitStructure);

  GPIO_PinAFConfig(MAP_SCK );
  GPIO_PinAFConfig(MAP_MISO);
  GPIO_PinAFConfig(MAP_MOSI);

  SPI_InitStructure.SPI_Direction         = SPI_Direction_2Lines_FullDuplex;
  SPI_InitStructure.SPI_Mode              = SPI_Mode_Master;
  SPI_InitStructure.SPI_DataSize          = SPI_DataSize_8b;
  SPI_InitStructure.SPI_CPOL              = SPI_CPOL_High;
  SPI_InitStructure.SPI_CPHA              = SPI_CPHA_2Edge;
  SPI_InitStructure.SPI_NSS               = SPI_NSS_Soft;
  SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;
  SPI_InitStructure.SPI_FirstBit          = SPI_FirstBit_MSB;
  SPI_InitStructure.SPI_CRCPolynomial     = 7;
  SPI_Init(SPIx, &SPI_InitStructure);

  SPI_RxFIFOThresholdConfig(SPIx, SPI_RxFIFOThreshold_QF);
  SPI_Cmd(SPIx, ENABLE);

  ExtFlash_CS_HIGH();
  ExtFlash_RST_LOW ();
  __Ctrl(DELAYmS, 100);
  ExtFlash_RST_HIGH();

  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
  NVIC_InitStructure.NVIC_IRQChannel                   = USB_IRQ;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  USB_Init();
  Disk_Init();
}

/*******************************************************************************
 Set USB device IO port NewState = ENABLE / DISABLE
*******************************************************************************/
void __USB_Port(u8 NewState)
{
  SCI_CLK_HIGH();
  if(NewState == DISABLE){         // USB DN & DP Pins Disconnect
    SCI_DIO_LOW();
    SCI_CLK_LOW();
    GPIOA->MODER  &= 0xFC3FFFFF;
    GPIOA->MODER  |= 0x01400000;
    GPIOA->OTYPER |= 0x1800;      // Dio Output Open-Drain
  } else {                        // USB DN & DP Pins Connect
    GPIOA->MODER  &= 0xFC3FFFFF;
    GPIOA->MODER  |= 0x02800000;
    GPIOA->OTYPER &= 0xE7FF;      // Push-Pull Output
  }
}
/*******************************************************************************
 Key initialization
*******************************************************************************/
void GPIO_SWD_NormalMode(void)
{
  GPIO_InitTypeDef         GPIO_InitStructure;

  GPIO_InitStructure.GPIO_Pin   = SWC | SWD ;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
}

/******************************** END OF FILE *********************************/
