/******************** (C) COPYRIGHT 2017 e-Design Co.,Ltd. *********************
 File Name : Process.c
 Version   : DS212                                                Author : bure
*******************************************************************************/
#include "Version.h"
#include "Process.h"
#include "Bios.h"
#include "Func.h"
#include "Draw.h"
#include "Drive.h"
#include "File.h"
#include "Math.h"

void Analys(void);
void Align_Set(void);

// Vertical channel coefficient table
//=========+======+======+======+======+======+======+======+======+======+======+
//         |          LV Range         |              HV Range                   |
//---------+------+------+------+------+------+------+------+------+------+------+
// Range   |   0  |   1  |   2  |   3  |   4  |   5  |   6  |   7  |   8  |   9  |
//---------+------+------+------+------+------+------+------+------+------+------+
// Gain    |  x20 |  x10 |  x5  |  x2  |  x1  |  x20 |  x10 |  x4  | x2.5 |  x1  |
//---------+------+------+------+------+------+------+------+------+------+------+
uc8 GK[] = {  2,     4,     10,    20,    4,     10,    20,    4,    10,     20, };
//=========+======+======+======+======+======+======+======+======+======+======+

// index        0       1       2       3       4       5       6       7       8       9   
// volt         10mV    20mV    50mV    0.1V    0.2V    0.5V    1V      2V      5V      10V
// kind         LV      LV      LV      LV      HV      HV      HV      HV      HV      HV  
// calib        LV      LV      LV      LV      MV      MV      MV      HV      HV      HV
// gain GK[]    2       4       10      20      4       10      20      4       10      20      
// state        GND     GND     GND     GND     GND     GND     GND     ACT     ACT     ACT             

u16 Smpl[0x4000];
u8* V_Buf = VRAM_PTR;       // display buffer VRAM = CCM = 0x10000000~0x10001FFF 8 KB

s32 PavgA, PavgB;

// Vertical channel (Zero/Gain) Correction Factor Table  LV = Low voltage,  HV = High voltage
//==========+======+======+======+======+======+======+======+======+======+======+======+======+
//  Range   |  LV  |  MV  |  HV  |  LV  |  MV  |  HV  |  LV  |  MV  |  HV  |  LV  |  MV  |  HV  |
//----------+------+------+------+------+------+------+------+------+------+------+------+------+
//          | CH_A Zero offset   | CH_B Zero offset   |  CH_A Gain factor  |  CH_B Gain factor  |
//----------+------+------+------+------+------+------+------+------+------+------+------+------+
s16 Kpg[] = {  2044, 2046, 2044,    2046, 2011, 2046,    1230, 1020, 1120,    1230, 1020, 1120  };
//==========+======+======+======+======+======+======+======+======+======+======+======+======+

s16 *KpA = &Kpg[0], *KpB = &Kpg[3], *KgA = &Kpg[6], *KgB = &Kpg[9];

/*******************************************************************************
 Align_Set:  Error correction
*******************************************************************************/
void Align_Set(void)
{
    s16 i, TmpA=0, TmpB=0, StA=0, StB=0;

    Analys();

    for(i=0; i<200; i++){
        AiPosi(100);                                            // set reference point
        BiPosi(100);       
        
        Wait_mS(10);                                            // wait 10 mS
        Analys();

        TmpA = 2048 - PavgA;    
        TmpB = 2048 - PavgB;

        if(TmpA != 0){
            KpA[KindA + (StateA ? 1 : 0)] += TmpA;              // CH_A error correction
            StA = 0;
        } 
        else{
            StA++;
        }

        if(TmpB != 0){
            KpB[KindB + (StateB ? 1 : 0)] += TmpB;              // CH_B error correction
            StB = 0;
        } 
        else{
            StB++;
        }

        if(StA>4 && StB>4) 
            return;
    }
}

/*******************************************************************************
 Channel's zero alignment:  Channel vertical displacement zero alignment
  u32  Bk = TmpB; // 8192~409
  __Ctrl(BOFFSET,(((u8)Val-100)*1024*GK[GainB+5*KindB])/KgB[KindB]+KpB[KindB]);
*******************************************************************************/
void Zero_Align(void)
{
    GainA = GainB = 8;                                      // x2.5
    KindA = KindB = HV;                                     //
    StateA = StateB = ACT;                                  //
    __Ctrl(AiRANGE, HV + DC + ACT);                         // set HV block input range A    0b101
    __Ctrl(BiRANGE, HV + DC + ACT);                         //                          B
    AiPosi(100);                                            //              reference zero A [2]
    BiPosi(100);                                            //                             B
    Wait_mS(1000);                                          //
    Align_Set();                                            // correcting zero error after delay
                
    GainA = GainB = 5;                                      // x20                        
    KindA = KindB = HV;                                     //
    StateA = StateB = GND;                                  //
    __Ctrl(AiRANGE, HV + DC + GND);                         // set MV block input range A     0b001
    __Ctrl(BiRANGE, HV + DC + GND);                         //                          B
    AiPosi(100);                                            //              reference zero A [1]
    BiPosi(100);                                            //                             B 
    Wait_mS(1000);                                          //  
    Align_Set();                                            // correcting zero error after delay
                
    GainA = GainB = 2;                                      // x5                   
    KindA = KindB = LV;                                     //
    StateA = StateB = GND;                                  //
    __Ctrl(AiRANGE, LV + DC + GND);                         // set LV block input range A     0b000
    __Ctrl(BiRANGE, LV + DC + GND);                         //                          B
    AiPosi(100);                                            //              reference zero A [0]
    BiPosi(100);                                            //                             B
    Wait_mS(1000);                                          //
    Align_Set();                                            // correcting zero error after delay
}

void initChannels(u8 viA, u8 viB, u8 coA, u8 coB)
{
    GainA = viA; 
    GainB = viB; 

    KindA = viA>3 ? HV : LV;
    KindB = viB>3 ? HV : LV;

    StateA = viA>6 ? ACT : GND;
    StateB = viB>6 ? ACT : GND;

    __Ctrl(AiRANGE, KindA + coA + StateA);   
    __Ctrl(BiRANGE, KindB + coB + StateB);                             

    AiPosi(100);                                            
    BiPosi(100);                                            
}

/*******************************************************************************
 Channel's error analys
*******************************************************************************/
void Analys(void)
{
    u32 i, SumA=0, SumB=0;

    __Ctrl(SMPL_ST, DISABLE);
    __Ctrl(SMPLTIM, 360-1);                                 // 144MHz / 360 = 400 kHz = 2.5 uS
    __Ctrl(SMPLNUM, 8192);  
    __Ctrl(SMPL_ST, ENABLE);
    
    while((__Info(CHA_CNT) && __Info(CHB_CNT)) != 0)
        ;
    
    for(i=4000; i<8000; i++){
        SumA += Smpl[i];
        SumB += Smpl[i + 8192];
    }
    
    PavgA = SumA / 4000; 
    PavgB = SumB / 4000; 
}

/*******************************************************************************
 WaveLimitd: Waveform limiting output
*******************************************************************************/
// void WaveLtdOut(u8* Buf, s16 Value)
// {
//   asm(" MOVS    R2, #1     \n"                      // single string asm for IAR 8
//       " MOVS    R3, #199   \n"
//       " CMP     R1, R2     \n"
//       " ITT     MI         \n"
//       " STRHMI  R2, [R0]   \n"
//       " BXMI    LR         \n"
//       " CMP     R1,  R3    \n"
//       " ITE     LS         \n"
//       " STRHLS  R1, [R0]   \n"
//       " STRHHI  R3, [R0]     ");
// }

/******************************** END OF FILE *********************************/
