/*****************************************************************************
File Name : main.c
Version   : DS212_VT                                     Author : qrt@qland.de
******************************************************************************/

// IAR compile options -> C/C++ compiler -> Optimizations -> Level High, Speed, Function inlinining OFF
// otherwise USB access does not work

// todo
// - drawLine in assembler
// - data save
// - display interpolation

// done

#include <stdio.h>
#include "Version.h"
#include "Process.h"
#include "Drive.h"
#include "Func.h"
#include "Draw.h"
#include "Bios.h"
#include "Disk.h"
#include "LCD.h"
#include "FAT12.h"
#include "File.h"
#include "Math.h"

//------------------------------------------------------------------------------

#define ABUF_SIZE       32                                  // ADC input buffer
#define EVIN_NUM        16                                  // evaluated input
#define SBUF_SIZE       (0x2000 - ABUF_SIZE)                // sample buffer

#define SLLEF           0                                   // status line
#define SLRIG           LCD_ROW
#define SLWID           (SLRIG - SLLEF)
#define SLHIG           14
#define SLTOP0          (LCD_COL - 1)
#define SLBOT0          (LCD_COL - SLHIG)
#define SLTOP1          (LCD_COL - SLHIG - 1)
#define SLBOT1          (LCD_COL - SLHIG * 2)

#define WFLEF           0                                   // wave form
#define WFRIG           LCD_ROW
#define WFBOT           0
#define WFTOP           (LCD_COL - SLHIG * 2)
#define WFHIG           (WFTOP - WFBOT)
#define WFWID           (WFRIG - WFLEF)
#define WFMID           (WFBOT + WFHIG / 2)

#define CHACOLA         YEL                                 // channel color A
#define CHACOLB         BLU                                 //               B

#define VOLGRID         33                                  // grids
#define TIMGRID         40

#define STDBRI           3                                  // standard brightness
#define BRETI           100                                 // brightness restore time in 1/10 s

//------------------------------------------------------------------------------

void chVal(CONTROL*,s8);
void prepare(void);
void drawSL(void);
void formVol(u8, s8);
void timeSL(char, u32);
void shiftSam(void);
void drawGraph(void);
void calcGrid(void);
void drawDelGrid(u16);
void drawTo(s16, s16);
void drawLine(s16, s16, s16, s16, u16);
void setPixel(s16, s16);
void clearWFRow(u16);
void dotWFRow(u16);
void dotClearWFRow(u16);
void clearWFWin(void);
void drawStr(u16, u16, u16, u16, u8*);
void drawStr6x14(u16, u16, u16, u16, u8*);
void fillSL(u16, u16, u16, u16);
void saveSettings(void);
void saveScreen(void);
void calibrate(void);
void setBright(u8);
void warnBeep(void);

//------------------------------------------------------------------------------

extern u32 sysTics;                                                     // in Interrupt.c

u8 APP_VERSION[] = "V0.1";                                              // must not exceed 12 characters

u16* samA = &Smpl[0] + ABUF_SIZE;                                       // sample buffers
u16* samB = &Smpl[0x2000] + ABUF_SIZE;
u16 sac;                                                                // sample counter
u32 eTime, sTime, bTime;                                                // elapsed-, screen-, buffer-time in s
u32 tb;                                                                 // timebase in ms
u8 curScr, curBri, brReTi;                                              // current-screen, -brightness, restore time

u16 tiba[] =  {  1,      2,      5,      10,     20,     50,     100,    150,    300,    600,    1200,   3600 };    // timebase in 1/10 s
u8* tbStr[] = { "0.1s", "0.2s", "0.5s", "1s",   "2s",   "5s",   "10s",  "15s",  "30s",  "1m",   "2m",   "3m"  };    //          string

// enum { MAIN, RTIM, MODE, VOLA, VOLB, COUP };                         // main, timebase, mode, volt A, B, coupling
enum { CALIB, SVSET, SVSCR, PREPARE, PREP2, RUNNING, STOPPED, READY };  // calibrate, save, prepare, running, stopped, ready
enum { FINI, INFI };                                                    // once, loop
enum { ACAC, DCDC, ACDC, DCAC };                                        // AC-, DC-coupling

struct control cPrep = {
    { "Main ", "Rtim ", "Mode ", "VolA ", "VolB ", "CoAB " },           // caption
    {  0,       0,       FINI,    6,       6,       ACAC   },           // default
    {  PREPARE, 0,       FINI,    6,       6,       ACAC   },           // value
    {  CALIB,   0,       FINI,    0,       0,       ACAC   },           // min
    {  PREPARE, 11,      INFI,    9,       9,       DCAC   },           // max
    {  1,       1,       1,       1,       1,       1      },           // change
    6,                                                                  // number of items
    MAIN,                                                               // selection
    { (u8* []){ "Calib   ", "SaveSet ", "SaveScr ",                     // main prepare sub captions
                "Start   ", "Prepare ",                                 //      Start=PREPARE, Prepare=PREP2
                "Running ", "Stopped ", "Ready   "},                    // states
      (u8* []){ "13m",  "27m",  "1.1h", "2.2h", "4.5h",                 // runtime @ SBUF_SIZE = 8160
                "11h",  "22h",  "1.4d", "2.8d", "5.6d",
                "1.6W", "1.1M" },
      (u8* []){ "Fini", "Infi" },                                       // mode
      (u8* []){ "10mV", "20mV", "50mV", "0.1V",                         // volt A LV
                        "0.2V", "0.5V", "1.0V",                         //        MV
                        "2.0V", "5.0V", "10V" },                        //        HV
      (u8* []){ "10mV", "20mV", "50mV", "0.1V",                         // volt B LV
                        "0.2V", "0.5V", "1.0V",                         //        MV
                        "2.0V", "5.0V", "10V" },                        //        HV
      (u8* []){ "ACAC", "DCDC", "ACDC", "DCAC" } }                      // couple AB
};

enum { ETIME, STIME, BTIME };                                                   // elapsed-, screen-, buffer-time
enum { TOTAL=-2, GLIDE, SCREEN };                                               // SCREEN screen visible
enum { VCAB, VCA, VCB };                                                        // visible channel

struct control cRun = {
    { "time", "nSc ", "vSc ",  "ofA",   "ofB",   "mag ", "vCh ", "bri "  },     // caption
    {  ETIME,  25,     TOTAL,   0,       0,       0,      VCAB,   STDBRI },     // default
    {  ETIME,  1,      TOTAL,   0,       0,       0,      VCAB,   STDBRI },     // value
    {  ETIME,  1,      TOTAL,  -40,     -40,     -15,     VCAB,   0      },     // min
    {  BTIME,  25,     SCREEN,  40,      40,      15,     VCB,    10     },     // max
    {  1,      1,      1,       1,       1,       1,      1,      1      },     // change
    8,                                                                          // number of items
    NSCR,                                                                       // selection
    {  0,      0,      0,       0,      0,      0,                              // item text
      (u8* []){ "AB", "A", "B" }, 0 }                                           //           visible channel
};

u16 mvUnit[]  = { 10,  20, 50, 100, 200, 500, 1000, 2000, 5000, 10000 };

u8* gridStr[] = { "1s", "2s", "5s", "10s", "15s", "30s",            // grid strings
                  "1m", "2m", "5m", "10m", "15m", "30m",
                  "1h", "2h", "6h", "12h",
                  "1d", "2d",
                  "1w", "2w", "4w" };

u32 gridSec[] = { 1, 2, 5, 10, 15, 30,                              // grid seconds s
                  60, 120, 300, 600, 900, 1800,                     //              m
                  3600, 7200, 21600, 43200,                         //              h
                  86400, 172800,                                    //              d
                  604800, 1209600, 2419200 };                       //              w

#define cLV     1.206                                               // low voltage correction (kpg 1230)
#define cMV     1.012                                               // mid                    (    1020)
#define cHV     1.143                                               // high                   (    1120)

u16 dpu[] =  { 50, 100, 250, 500,                                   // ADC digits per unit LV
                   100, 250, 500,                                   //                     MV
                   100, 250, 500 };                                 //                     HV

float rco[] = { cLV, cLV, cLV, cLV,                                 // ADC range correction LV
                     cMV, cMV, cMV,                                 //                      MV
                     cHV, cHV, cHV };                               //                      HV

u8 buf[32];                                                         // sprintf buffer

void main(void)
{
    //============================== system initialization ==================================

    __Ctrl(SYS_CFG, RCC_DEV | TIM_DEV | GPIO_OPA | ADC_DAC | SPI);

    GPIO_SWD_NormalMode();                                  // turn off SWD burner function

    #if defined (APP1)                                      // see option.h and DS212.icf
    NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x8000);        // first partition APP
    #elif defined (APP2)
    NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x20000);       // second partition APP
    #endif

    SysTick_Config(SystemCoreClock / 1000);                 // SysTick = 1 mS

    setBright(STDBRI);                                      // set standard brightness

    __Ctrl(BUZZVOL, 50);                                    // set the buzzer volume (0~100 %)
    Beep(200);                                              // 200 ms

    USB_MSD_Config();
    Init_Fat_Value();

    __Ctrl(SMPL_ST, DISABLE);
    __Ctrl(SMPL_ST, SIMULTANEO);                            // SIMULTANEO, INTERLEAVE
    __Ctrl(SMPLBUF, (u32)Smpl);                             // 0x4000
    __Ctrl(SMPLNUM, ABUF_SIZE);                             // ADC buffer size
    __Ctrl(SMPL_ST, ENABLE);

    //============================ display boot prompt information page ==========================

    SetColor(BLK, WHT);
    DispStr(0, 90, PRN, "                                        ");
    DispStr(0, 70, PRN, "                                        ");
    #if defined (APP1)
    DispStr(8, 90, PRN, "       Oscilloscope  APP1");
    #elif defined (APP2)
    DispStr(8, 90, PRN, "       Oscilloscope  APP2");
    #endif

    DispStr(8+26*8, 90, PRN,                            APP_VERSION);
    DispStr(8,      70, PRN, "       System Initializing ...      ");

    __Ctrl(DELAYmS, 500);

    ADC_StartConversion(ADC1);
    ADC_StartConversion(ADC3);
    ADC_StartConversion(ADC2);
    ADC_StartConversion(ADC4);

    //============================ first write firmware auto-calibration ==========================

     Read_CalFlag();

     if(Cal_Flag == 1)
         calibrate();
    else
        Read_Kpg();                                                 // read calibration parameters

    __Ctrl(DELAYmS, 500);

    if(Load_Param() == OK)
        cRun.max[VSCR] = cRun.val[NSCR];                            // set visual screens to number of screens

    if(cRun.val[BRIG] < STDBRI)                                     // if low saved brightness
        brReTi = BRETI;                                             // set brightness restore time

    //============================ main interface display ====================================

    ClrScrn(DAR);                                                   // background clear screen

    //================================== main cycle program ==============================

    u32 sa=0, sb=0;
    u32 neSaTi=0, neDrTi=0, neSeTi=0;                               // next sample-, draw-, sec-time
    u8 dtog=0, secTic=1;                                            // draw toggle, second ticker

    Keys_Detect();                                                  // debounce

    while(1){
        Keys_Detect();                                              // key scan

        if(KeyIn){
            if(cRun.val[BRIG] < STDBRI){                            // brightness
                if(curBri == 0)                                     // if screen off ignore key
                    KeyIn = 0;

                setBright(STDBRI);
                brReTi = BRETI;
            }
        }

        switch(KeyIn){
            case K_RUN:                                             // run key - - - - - - - - - - - - -
                if(cPrep.val[MAIN] == RUNNING){
                    cPrep.val[MAIN] = STOPPED;
                }
                else if(cPrep.val[MAIN] == STOPPED){
                    neSaTi = neSeTi = sysTics;
                    cPrep.val[MAIN] = RUNNING;
                }

                break;

            case R_HOLD:
                if(cPrep.val[MAIN]==PREPARE || cPrep.val[MAIN]==PREP2){
                    prepare();
                    neSaTi = neSeTi = sysTics;
                    cPrep.val[MAIN] = RUNNING;
                }
                else if(cPrep.val[MAIN] == SVSET){
                    saveSettings();
                    cPrep.val[MAIN] = PREPARE;
                }
                else if(cPrep.val[MAIN] == SVSCR){
                    saveScreen();
                    cPrep.val[MAIN] = PREPARE;
                }
                else if(cPrep.val[MAIN] == CALIB){
                    calibrate();
                    cPrep.val[MAIN] = PREPARE;
                }
                else{
                    cPrep.val[MAIN] = PREPARE;
                }

                break;

            case K_S:                                               // top wheel - - - - - - - - - - - - -
                if(cPrep.val[MAIN] <= PREPARE)
                    cPrep.val[MAIN] = PREP2;
                else if(cPrep.val[MAIN] == PREP2)
                    cPrep.val[MAIN] = PREPARE;

                break;

            case K_LEFT:
                if(cPrep.val[MAIN] <= PREPARE)
                    cPrep.sel = --cPrep.sel<0 ? 0 : cPrep.sel;
                else
                    cRun.sel = --cRun.sel<0 ? 0 : cRun.sel;

                break;

            case K_RIGHT:
                if(cPrep.val[MAIN] <= PREPARE)
                    cPrep.sel = ++cPrep.sel>=cPrep.numItm ? cPrep.numItm-1 : cPrep.sel;
                else
                    cRun.sel = ++cRun.sel>=cRun.numItm ? cRun.numItm-1 : cRun.sel;

                break;

            case S_HOLD:
                break;

            case KEY_DOUBLE_S:
                break;

            case K_M:                                               // side wheel - - - - - - - - - - - -
                if(cPrep.val[MAIN] == PREPARE){
                    if(cPrep.sel > MAIN)
                        cPrep.val[cPrep.sel] = cPrep.def[cPrep.sel];
                }
                else{
                    cRun.val[cRun.sel] = cRun.def[cRun.sel];
                }

                break;

            case K_UP:
                if(cPrep.val[MAIN] <= PREPARE)
                    chVal(&cPrep, 1);
                else
                    chVal(&cRun, 1);

                break;

            case K_DOWN:
                if(cPrep.val[MAIN] <= PREPARE)
                    chVal(&cPrep, -1);
                else
                    chVal(&cRun, -1);

                break;

            case M_HOLD:
                break;

            case KEY_DOUBLE_M:
                break;

            default:
                break;
        }

        KeyIn = 0;

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        if(cPrep.val[MAIN] == RUNNING){
            if(sysTics >= neSaTi){
                __Ctrl(SMPL_ST, DISABLE);
                __Ctrl(SMPLTIM, 360-1);                             // 144 MHz / 360 = 400 kHz = 2.5 uS
                __Ctrl(SMPLNUM, ABUF_SIZE);
                __Ctrl(SMPL_ST, ENABLE);

                sa=0, sb=0;

                while((__Info(CHA_CNT) && __Info(CHB_CNT))!=0)
                    ;

                for(u16 i=EVIN_NUM/2; i<ABUF_SIZE-EVIN_NUM/2; i++){
                    sa += Smpl[i];
                    sb += Smpl[0x2000 + i];
                }

                if(sac == SBUF_SIZE){
                    shiftSam();
                    sac--;
                }

                samA[sac] = sa / EVIN_NUM;
                samB[sac++] = sb / EVIN_NUM;

                if(cPrep.val[MODE]==FINI && sac==SBUF_SIZE)
                    cPrep.val[MAIN] = READY;

                neSaTi += tb;
            }

            if(sysTics >= neSeTi){
                eTime++;
                neSeTi += 1000;
            }
        }

        if(sysTics >= neDrTi){                                  // every 100 ms
            if(dtog++ & 1)                                      // every 200 ms
                drawSL();
            else
                drawGraph();

            neDrTi = sysTics + 100;                             // next in 100 ms

            if(brReTi && !(--brReTi))                           // count down brightness restore time
                setBright(cRun.val[BRIG]);

            if(!(--secTic)){                                    // every 1 s
                if(Bat_Vol() < 3200){                           // automatic shutdown if battery < 3.2 V
                    Battery = 0;
                    Battery_update(SLRIG-18, SLBOT0);
                    warnBeep();
                    __Ctrl(DELAYmS, 1000);
                    __Ctrl(PWROFF, ENABLE);
                }

                secTic = 10;                                    // next in 10 * 100 ms
            }
        }
    }
}

//------------------------------------------------------------------------------

void chVal(CONTROL* ctrl, s8 ch)
{
    u8 sel = ctrl->sel;

    if(ch > 0){
        ctrl->val[sel] += ctrl->cha[sel];

        if(ctrl->val[sel] > ctrl->max[sel])
            ctrl->val[sel] = ctrl->max[sel];
    }
    else if(ch < 0){
        ctrl->val[sel] -= ctrl->cha[sel];

        if(ctrl->val[sel] < ctrl->min[sel])
            ctrl->val[sel] = ctrl->min[sel];
    }

    if(ctrl == &cRun){
        if(sel == NSCR){
            cRun.max[VSCR] = cRun.val[NSCR];

            if(cRun.val[VSCR] > cRun.max[VSCR])
                cRun.val[VSCR] = cRun.max[VSCR];
        }
        else if(sel==BRIG && cRun.val[BRIG]>=STDBRI){
            setBright(cRun.val[BRIG]);
        }
    }
}

u8 tgi;                                                             // time grid index
float ufA, ufB;

void prepare(void)
{
    u8 c = cPrep.val[COUP];
    u8 coA = c==ACAC || c==ACDC ? AC : DC;
    u8 coB = c==ACAC || c==DCAC ? AC : DC;

    ufA = rco[cPrep.val[VOLA]] / dpu[cPrep.val[VOLA]];              // unit factor = range correction / digits per unit
    ufB = rco[cPrep.val[VOLB]] / dpu[cPrep.val[VOLB]];

    initChannels(cPrep.val[VOLA], cPrep.val[VOLB], coA, coB);
    tb = tiba[cPrep.val[RTIM]] * 100;                               // timebase in ms
    bTime = tiba[cPrep.val[RTIM]] * SBUF_SIZE / 10;                 // buffer-time
    eTime = 0;                                                      // elapsed time
    sac = 0;

    clearWFWin();
}

#define ITXP(i)     cPrep.itx[i][cPrep.val[i]]
#define ITXR(i)     cRun.itx[i][cRun.val[i]]

void drawSL(void)
{
    static u8 blc;
    u16 color;
    u16 posx = SLLEF;

    if(cPrep.val[MAIN] <= PREPARE){
        drawStr(posx, SLBOT0, YEL, BLK, "Prepare ");
        drawStr(posx, SLBOT1, cPrep.sel==MAIN ? WHT : GRY, BLK, ITXP(MAIN));
        posx += 8*8;

        for(u8 i=1; i<cPrep.numItm; i++){
            drawStr6x14(posx, SLBOT0, GRY, BLK, cPrep.cap[i]);

            sprintf((char*)buf, "%-4s ", ITXP(i));
            drawStr6x14(posx, SLBOT1, cPrep.sel==i ? WHT : GRY, BLK, buf);

            posx += 5 * 6;
        }
    }
    else{
        if(cPrep.val[MAIN] == PREP2)
            color = YEL;
        else if(cPrep.val[MAIN] == READY)
            color = GRN;
        else
            color = blc<5 ? RED : BLK;

        drawStr(posx, SLBOT0, color, BLK, ITXP(MAIN));

        if(cRun.val[TIME] == ETIME)                                 // elapsed time
            timeSL('E', eTime);
        else if(cRun.val[TIME] == STIME)                            // screen
            timeSL('S', sTime);
        else if(cRun.val[TIME] == BTIME)                            // buffer
            timeSL(cPrep.val[MODE]==FINI ? 'F' : 'I', bTime);       // finit or infinit

        drawStr6x14(posx, SLBOT1, cRun.sel==TIME ? WHT : GRY, BLK, buf);

        fillSL(posx+9*6, SLBOT1, posx+8*8-1, SLTOP1);
        posx += 8*8;

        for(u8 i=1; i<cRun.numItm; i++){
            u8 pa = 0;

            if(i == VSCR){
                drawStr6x14(posx, SLBOT0, GRY, BLK, cRun.cap[i]);

                if(cRun.val[i] == TOTAL)
                    sprintf((char*)buf, "T   ");
                else if(cRun.val[i] == GLIDE)
                    sprintf((char*)buf, "G%-3d", curScr + 1);
                else if(cRun.val[i] == SCREEN)
                    sprintf((char*)buf, "S%-3d", curScr + 1);
                else
                    sprintf((char*)buf, "%-4d", cRun.val[i]);
            }
            else if(i == VCHN){
                drawStr6x14(posx, SLBOT0, GRY, BLK, cRun.cap[i]);
                sprintf((char*)buf, "%-4s", ITXR(i));
            }
            else if(i == OFFA){
                s8 o = cRun.val[i];
                sprintf((char*)buf, "%s%c ", cRun.cap[i], o==0 ? ' ' : (o<0 ? '-' : '+'));
                drawStr6x14(posx, SLBOT0, GRY, BLK, buf);

                formVol(cPrep.val[VOLA], o);
                pa = 6;
            }
            else if(i == OFFB){
                s8 o = cRun.val[i];
                sprintf((char*)buf, "%s%c ", cRun.cap[i], o==0 ? ' ' : (o<0 ? '-' : '+'));
                drawStr6x14(posx, SLBOT0, GRY, BLK, buf);

                formVol(cPrep.val[VOLB], o);
                pa = 6;
            }
            else{
                drawStr6x14(posx, SLBOT0, GRY, BLK, cRun.cap[i]);
                sprintf((char*)buf, "%-2d  ", cRun.val[i]);
            }

            if(i==BRIG && cRun.val[BRIG]<STDBRI)                    // mark lower brightnesses
                color = PUR;
            else
                color = WHT;

            drawStr6x14(posx, SLBOT1, cRun.sel==i ? color : GRY, BLK, buf);
            posx += 4 * 6 + pa;
        }
    }

    u8 c = cPrep.val[COUP];

    fillSL(posx, SLBOT1, SLRIG-11*6-1, SLTOP0);
    posx = SLRIG - 11 * 6;

    sprintf((char*)buf, "%4s %s", ITXP(VOLA), c==ACAC || c==ACDC ? "AC" : "DC");
    drawStr6x14(posx, SLBOT0, CHACOLA, BLK, buf);

    sprintf((char*)buf, "%4s %s", ITXP(VOLB), c==ACAC || c==DCAC ? "AC" : "DC");
    drawStr6x14(posx, SLBOT1, CHACOLB, BLK, buf);

    posx += 7 * 6;

    fillSL(posx, SLBOT0, SLRIG-18-1, SLTOP0);                       // status line 0, rightmost battery
    Battery_update(SLRIG-18, SLBOT0);

    fillSL(posx, SLBOT1, SLRIG-3*6-1, SLTOP1);                      // status line 1, rightmost grid width
    sprintf((char*)buf, "%3s", gridStr[tgi]);
    drawStr6x14(SLRIG-3*6, SLBOT1, GRY, BLK, buf);

    blc = ++blc<10 ? blc : 0;
}

// voltage index 0..9, offset -40..40 in 1/10 unit
//
void formVol(u8 vi, s8 o)
{
    s32 mv = mvUnit[vi] * o / 10;
    u32 av = abs(mv);

    if(av == 0)
        sprintf((char*)buf, "0    ");                                       // 0
    else if(av < 10)
        sprintf((char*)buf, "%dmV  ", av);                                  // 1mV..9mV
    else if(av < 100)
        sprintf((char*)buf, "%dmV ", av);                                   // 10mV..99mV
    else if(av < 10000)
        sprintf((char*)buf, "%d.%dV ", av / 1000, (av % 1000) / 100);       // 0.1V..9.9V
    else
        sprintf((char*)buf, "%dV  ", av / 1000);                            // 10V..99V
}

void timeSL(char c, u32 t)
{
    u8 h = (t / 3600) % 24;
    u8 m = (t / 60) % 60;

    if(t <= 23*60*60 + 59*60 + 59){
        u8 s = t % 60;
        sprintf((char*)buf, "%c%02d:%02d:%02d", c, h, m, s);
    }
    else{
        u8 d = t / (3600 * 24);
        sprintf((char*)buf, "%c%02d %02d:%02d", c, d, h, m);
    }
}

void fillSL(u16 x0, u16 y0, u16 x1, u16 y1)
{
    for(u16 x=x0; x<=x1; x++){
        __SetPosi(x, y0);

        for(u16 y=y0; y<=y1; y++)
            __SetPixel(BLK);
    }
}

void shiftSam(void)
{
    for(u16 i=1; i<SBUF_SIZE; i++){
        samA[i-1] = samA[i];
        samB[i-1] = samB[i];
    }
}

float scaX, tgx, tgw;
u16 sas;
u8 vgh;

void drawGraph(void)
{
    static float scaA, scaB, xf;
    static u16 sae, offA, offB;
    static u8 showA, showB;
    static u32 sua, sub;
    static u16 scn, x;
    static s16 ya, yb, yya, yyb, sa, sb;

    if(sac != 0){
        if(cRun.val[VSCR] == TOTAL){                        // TOTAL
            scaX = (float)WFWID / sac;
            if(scaX > 1.0) scaX = 1.0;
            curScr = 0;
            sas = 0;
            sae = sac;
        }
        else if(cRun.val[VSCR] == GLIDE){                   // GLIDE
            float sps = (float)SBUF_SIZE / cRun.val[NSCR];
            scaX = (float)WFWID / sps;
            curScr = (u8)((sac - 1) / sps);
            sas = (u16)(sac>=sps ? sac-sps : 0);
            sae = sac;
        }
        else{                                               // SCREEN and FIXED
            float sps = (float)SBUF_SIZE / cRun.val[NSCR];
            scaX = (float)WFWID / sps;
            curScr = cRun.val[VSCR]==SCREEN ? (u8)((sac-1)/sps) : cRun.val[VSCR]-1;
            sas = (u16)(sps * curScr);
            sae = (u16)(sas + sps);
            if(sae > sac) sae = sac;
            if(sas > sac) { sas = 0; sae = 0; }
        }

        calcGrid();

        scaA = vgh * ufA;                                   // scale = grid height * unit factor
        offA = WFMID + cRun.val[OFFA] * vgh / 10;           // offset = mid + off * grid height / 10

        scaB = vgh * ufB;
        offB = WFMID + cRun.val[OFFB] * vgh / 10;

        showA = cRun.val[VCHN] != VCB;
        showB = cRun.val[VCHN] != VCA;

        sua = sub = 0; scn = 1;
        xf = 0; x = 0;

        for(u16 i=sas; i<sae; i++){
            xf += scaX;

            if(xf >= x){
                if(scn != 1){
                    sa = (sua + samA[i]) / scn;
                    sb = (sub + samB[i]) / scn;
                    sua = 0; sub = 0; scn = 1;
                }
                else{
                    sa = samA[i];
                    sb = samB[i];
                }

                drawDelGrid(x);

                ya = (s16)(offA + (sa - 2048) * scaA);
                yb = (s16)(offB + (sb - 2048) * scaB);

                if(x > 0){
                    if(showA)
                        drawLine(WFLEF+x-1, yya, WFLEF+x, ya, sa<4070 && sa>5 ? CHACOLA : RED);

                    if(showB)
                        drawLine(WFLEF+x-1, yyb, WFLEF+x, yb, sb<4070 && sb>5 ? CHACOLB : RED);
                }

                yya = ya; yyb = yb;
                x++;
            }
            else{
                sua += samA[i];
                sub += samB[i];
                scn++;
            }
        }

        for(u16 xx=x; xx<WFRIG; xx++)
            drawDelGrid(xx);
    }
}

void drawDelGrid(u16 x)
{
    if(x >= tgx){                                       // vertical time grid
        dotWFRow(WFLEF + x);
        tgx += tgw;
    }
    else{                                               // horizontal voltage grid
        if(x & 3)
            clearWFRow(WFLEF + x);
        else
            dotClearWFRow(WFLEF + x);
    }
}

void calcGrid(void)
{
    float tpp = tb / (scaX * 1000);                     // time per pixel in s
    float tpg = tpp * TIMGRID;                          //          grid

    tgi = 0;                                            // time grid index

    while(tgi<sizeof(gridSec)/sizeof(u8*) && gridSec[tgi]<tpg)
        tgi++;

    tgw = gridSec[tgi] / tpp;                           // time grid width in pixel
    float tgn = curScr * WFWID / tgw;                   //           number
    tgx = (1 - (tgn - (u16)tgn)) * tgw;                 //           x in pixel

    sTime = tb * sas / 1000;                            // screen start time
    vgh = VOLGRID + cRun.val[MAGN];                     // voltage grid height in pixel
}

u16 wfcol;

void drawLine(s16 x0, s16 y0, s16 x1, s16 y1, u16 col)
{
    wfcol = col;

    s16 ax = 1, ay = 1;
    s16 dx = x1 - x0;
    s16 dy = y1 - y0;

    if(dx < 0){
        dx = -dx;
        ax = -ax;
    }

    if(dy < 0){
        dy = -dy;
        ay = -ay;
    }

    if(dx!=0 && dy!=0){
        if(dx >= dy){
            u16 r = dx / 2;

            for(u16 x=0; x<dx; x++){
                r += dy;

                if(r >= dx){
                    r -= dx;
                    setPixel(x0, y0);
                    x0 += ax;
                    y0 += ay;
                }
                else{
                    setPixel(x0, y0);
                    x0 += ax;
                }
            }
        }
        else{
            u16 r = dy / 2;

            for(u16 y=0; y<dy; y++){
                r += dx;

                if(r >= dy){
                    r -= dy;
                    setPixel(x0, y0);
                    x0 += ax;
                    y0 += ay;
                }
                else{
                    setPixel(x0, y0);
                    y0 += ay;
                }
            }
        }
    }
    else if(dx!=0 && dy==0){
        for(u16 x=0; x<dx; x++){
            setPixel(x0, y0);
            x0 += ax;
        }
    }
    else if(dx==0 && dy!=0){
        for(u16 y=0; y<dy; y++){
            setPixel(x0, y0);
            y0 += ay;
        }
    }
}

void setPixel(s16 x, s16 y)
{
    if(y>=WFBOT && y<WFTOP){
        __SetPosi(x, y);
        __SetPixel(wfcol);
    }
}

void dotWFRow(u16 x)
{
    __SetPosi(x, WFBOT);

    for(u16 y=WFBOT; y<WFTOP; y++)
        __SetPixel(y&3 ? DAR : GRY);
}

void clearWFRow(u16 x)
{
    __SetPosi(x, WFBOT);

    for(u16 y=WFBOT; y<WFTOP; y++)
        __SetPixel(DAR);
}

void dotClearWFRow(u16 x)
{
    __SetPosi(x, WFBOT);

    for(u16 y=WFBOT; y<WFTOP; y++)
        __SetPixel((y-WFMID)%vgh ? DAR : (y!=WFMID ? GRY : PUR));
}

void clearWFWin()
{
    for(u16 x=WFLEF; x<WFRIG; x++){
        __SetPosi(x, WFBOT);

        for(u16 y=WFBOT; y<WFTOP; y++)
            __SetPixel(DAR);
    }
}

void drawStr(u16 x0, u16 y0, u16 fColor, u16 bColor, u8 *s)
{
u16 x, y, b;

    SetBlock(x0, y0, LCD_ROW-1, y0+13);

    while(*s != 0){
        for(x=0; x<8; x++){
            b = Get_TAB_8x14(*s, x);

            for(y=0; y<14; ++y){
                if(b & 4) SetPixel(fColor);
                else      SetPixel(bColor);
                b >>= 1;
            }
        }

        s++;                                        // string pointer+1
    }

    SetBlock(0, 0, LCD_ROW-1, LCD_COL-1);           // restore full-size window
}

void drawStr6x14(u16 x0, u16 y0, u16 fColor, u16 bColor, u8 *s)
{
u16 x, y, b;

    SetBlock(x0, y0, LCD_ROW-1, y0+13);             // set block for string

    while(*s != 0){
        for(x=0; x<6; x++){
            SetPixel(bColor);                       // base line +1
            b = Get_TAB_6x8(*s, x);

            for(y=0; y<13; ++y){
                if(b & 1) SetPixel(fColor);
                else      SetPixel(bColor);
                b >>= 1;
            }
        }

        s++;                                        // string pointer+1
    }

    SetBlock(0, 0, LCD_ROW-1, LCD_COL-1);           // restore full-size window
}

void saveSettings(void)
{
    Beep(200);                                      // beep 200 ms

    Save_Param();                                   // save settings
    __Ctrl(DELAYmS, 1000);                          // wait a second
}

void saveScreen(void)
{
    Beep(200);                                      // beep 200 ms

    cPrep.val[MAIN] = READY;                        // change status bar
    drawSL();

    __USB_Port(DISABLE);
    u8 rv = Save_Bmp();
    __USB_Port(ENABLE);

    if(rv != OK){
        drawStr(SLLEF, SLBOT0, BLK, BLK, "I/O Err ");
        warnBeep();
    }

    __Ctrl(DELAYmS, 1000);                          // wait a second
}

void calibrate(void)
{
    Beep(200);                                      // beep 200 ms

    SetColor(DAR, ORN);                             // message
    DispStr(24, 30, PRN, "Run calibration");
    DispStr(24, 50, PRN, "Please wait a few seconds ...");

    Cal_Flag = 0;                                   // zero correction
    Zero_Align();
    Save_Kpg();

    clearWFWin();                                   // clear waveform
    sac = 0;                                        // 'delete' waveform
}

void setBright(u8 b)
{
    if(b != curBri){
        __Ctrl(B_LIGHT, b==1 ? 5 : b*10);
        curBri = b;
    }
}

void warnBeep(void)
{
    for(u8 i=0; i<3; i++){
        Beep(200);
        __Ctrl(DELAYmS, 200);
    }
}