/******************** (C) COPYRIGHT 2017 e-Design Co.,Ltd. ********************
Project Name: DS212
* File Name:  Files.c                                              Author: xie
******************************************************************************/
#include <stdlib.h>
#include "Version.h"
#include "string.h"
#include "STM32F30x.h"
#include "stm32f30x_flash.h"
#include "Draw.h"
#include "Func.h"
#include "File.h"
#include "FAT12.h"
#include "Process.h"
#include "Flash.h"
#include "Func.h"
#include "Drive.h"
#include "Bios.h"
#include "Lcd.h"

#define Kpg_Address     0x08007800      // save calibrated Kpg[] zero offset and gain coefficient

uc8 BmpHead[54] = { 0x42, 0x4D, 0x76, 0x96, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x76, 0x00, 0x00, 0x00, 0x28, 0x00,
                    0x00, 0x00, 0x40, 0x01, 0x00, 0x00, 0xF0, 0x00,
                    0x00, 0x00, 0x01, 0x00, 0x04, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x82, 0x0B, 0x00, 0x00, 0x12, 0x0b,
                    0x00, 0x00, 0x12, 0x0b, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

uc16 BMP_Color[16] = { WHT,  CYN, CYN_, YEL, YEL_, PUR, PUR_, GRN,
                       GRN_, GRY, ORN,  BLU, RED,  BLK, LGN,  DAR };

u8 Cal_Flag = 1;
u8 KpgSave_Len = 32;

extern u8 DiskBuf[4096];

/*******************************************************************************
 Read_Kpg: Read calibration flag from FLASH
*******************************************************************************/
void Read_CalFlag(void)
{
    u16* ptr;

    ptr = (u16*)(Kpg_Address + KpgSave_Len*2);          // *2 address addition   

    if(*ptr != 0xaa55) 
        return;

    ptr = (u16*)Kpg_Address;

    if(*ptr++ != 0x0300)
        return;

    Cal_Flag = *ptr++;

    return;
}

/*******************************************************************************
 Save_Kpg: Save Calibration Parameters - Return: 0=Success
*******************************************************************************/
u8 Save_Kpg(void)
{
    unsigned short i, j;
    u16* ptr;

    FLASH_Unlock();
    j = FLASH_ErasePage(Kpg_Address);

    if(j == FLASH_COMPLETE){
        ptr = (u16*)&DiskBuf;
        *ptr++ = 0x0300;                                // version 3.00
        *ptr++ = Cal_Flag;

        for(i=0; i<12; i++)                             // KpA/B + KgA/B values are saved
            *ptr++ = Kpg[i];

        ptr = (u16*)&DiskBuf;
        *(ptr + KpgSave_Len) = 0xaa55;                  // pointer addition

        ptr = (u16*)&DiskBuf;
        
        for(i=0; i<256; i+=2){                          // 128 SHORT
            j = FLASH_ProgramHalfWord(Kpg_Address + i, *ptr++);

            if(j != FLASH_COMPLETE) 
                break;
        }
    }

    FLASH_Lock();
    
    return j;
}

/*******************************************************************************
 Read_Kpg: Read calibration parameters from FLASH
*******************************************************************************/
void Read_Kpg(void)
{
    u8 i;
    u16* ptr;

    ptr = (u16*)(Kpg_Address + KpgSave_Len*2);          // *2 address addition

    if(*ptr != 0xaa55) 
        return;

    ptr = (u16*)Kpg_Address;
    
    if(*ptr++ != 0x0300) 
        return;
    
    Cal_Flag = *ptr++;

    for(i=0; i<6; i++)                                  // only KpA/B values are restored
        Kpg[i] = *ptr++;                                // CH_A, CH_B zero offset  
    
    return;
}

#define PAR_VERSION     0x04

extern CONTROL cPrep, cRun;

/*******************************************************************************
 Save_Param: Save current working parameters to U disk - Return: 0=Success
*******************************************************************************/
u8 Save_Param(void)
{
    u8 Sum = 0, Version = PAR_VERSION;
    u16 i;
    u16* ptr = (u16*)DiskBuf;

    u16 pCluster[3];
    u32 pDirAddr[1];

    u8 Filename[] = "QVOLTRA0TXT";

    // Word2Hex(Filename, __Info(DEV_SN));

    // #if defined (APP1)
    //     Filename[8] = 'P'; Filename[9] = 'A'; Filename[10] = 'R';
    // #elif defined (APP2)
    //     Filename[8] = 'P'; Filename[9] = 'A'; Filename[10] = '2';
    // #endif

    switch(OpenFileRd(DiskBuf, Filename, pCluster, pDirAddr)){
        case NEW:                                                           // original WPT file does not exist
            if(OpenFileWr(DiskBuf, Filename, pCluster, pDirAddr) != OK)
                return DISK_RW_ERR;

        case OK:                                                            // the original WPT file exists
            memset(DiskBuf, 0, 512);
            *ptr++ = Version;                                               // save version

            for(i=RTIM; i<cPrep.numItm; i++)
                *ptr++ = cPrep.val[i];

            for(i=TIME; i<cRun.numItm; i++)
                *ptr++ = cRun.val[i];

            for(i=0; i<511; i++)                                            // calculate the parameter table checksum
                Sum += DiskBuf[i];
            
            DiskBuf[511] = (~Sum) + 1;                                      // neg

            if(ProgFileSec(DiskBuf, pCluster) != OK)                        // write data
                return FILE_RW_ERR; 

            if(CloseFile(DiskBuf, 512, pCluster, pDirAddr) != OK)
                return FILE_RW_ERR;

            return OK;

        default:  
            return FILE_RW_ERR;
    }
}

/*******************************************************************************
 Load_Param: Work parameters before loading From USB drive - Return: 0=Success
*******************************************************************************/
u8 Load_Param(void)
{
    u8 Sum = 0, Version = PAR_VERSION;
    u16 i, rc;
    u16* ptr = (u16*)DiskBuf;

    u16 pCluster[3];
    u32 pDirAddr[1];

    u8 Filename[] = "QVOLTRA0TXT";

    // Word2Hex(Filename, __Info(DEV_SN));

    // #if defined (APP1)
    //     Filename[8] = 'P'; Filename[9] = 'A'; Filename[10] = 'R';
    // #elif defined (APP2)
    //     Filename[8] = 'P'; Filename[9] = 'A'; Filename[10] = '2';
    // #endif

    rc = OpenFileRd(DiskBuf, Filename, pCluster, pDirAddr);

    if(rc != OK){ 
        return rc;                                              // FILE_RW_ERR;
    }
    else{
        if(ReadFileSec(DiskBuf, pCluster) != OK) 
            return 100 + FILE_RW_ERR;

        if(Version != (*ptr++ & 0xff))                          // version error return
            return VER_ERR;

        for(i=0; i<512; ++i) 
            Sum += DiskBuf[i];
        
        if(Sum != 0)                                            // checksum error return
            return SUM_ERR; 

        for(i=RTIM; i<cPrep.numItm; i++)
            cPrep.val[i] = *ptr++;

        for(i=TIME; i<cRun.numItm; i++)
            cRun.val[i] = *ptr++;

        return OK;
    }
}

#define PROGWID     160
#define PROGLEF     0
#define PROGBOT     0
#define PROGHIG     2

/*******************************************************************************
 Save_Bmp: Save the current screen display image as BMP format - Input: File number - Return value: 0x00 = success
*******************************************************************************/
u8 Save_Bmp(void)
{                      
    u8 pFileName[12] = "IMAGE_xxBMP";
    u16 pCluster[3];
    u32 pDirAddr[1];
    u8 num[4], n;

    for(n=0; n<100; n++){
        u8ToDec3Str(num, n);
        pFileName[6] = num[1];
        pFileName[7] = num[2];

        if(OpenFileWr(DiskBuf, pFileName, pCluster, pDirAddr) == OK)
            break;
    }

    if(n >= 100)
        return FILE_RW_ERR;

    memcpy(DiskBuf, BmpHead, 54);                           // copy header

    u16 p = 0x0036;                                         // buffer position palette

    for(u16 i=0; i<16; i++){
        DiskBuf[p+i*4]   = (BMP_Color[i] & 0xF800) >> 8;    // blue
        DiskBuf[p+i*4+1] = (BMP_Color[i] & 0x07E0) >> 3;    // green
        DiskBuf[p+i*4+2] = (BMP_Color[i] & 0x001F) << 3;    // red
        DiskBuf[p+i*4+3] = 0;                               // alpha
    }

    p = 0x0076;                                             // buffer position image data

    s16 cH, cL;                                             // color variables

    s16 k = (54 + 64 + 240 * 160) / PROGWID;                // progress variables
    s16 l = 160;                                            // byte progress per y loop
    s16 r = k - l - 1;                                      // delay progress bar for 2 y loops to not show up in saved bitmap
    u16 px = PROGLEF;

    for(u16 y=0; y<240; y++){
        for(u16 x=0; x<320; x+=2){
            __SetPosi(x, y);
            cH = __ReadPixel();

            __SetPosi(x + 1, y);
            cL = __ReadPixel();
            
            DiskBuf[p++] = (colorNum(cH) << 4) + colorNum(cL);

            if(p >= SEC_SIZE){
                if(ProgFileSec(DiskBuf, pCluster) != OK)    // write buffer
                    return FILE_RW_ERR;

                p = 0;                                      // reset buffer position
            }
        }

        r += l;                                             // progress bar

        if(r >= k){                                 
            __SetPosi(px, PROGBOT);
            px += 2;

            for(u16 y=0; y<PROGHIG; y++)
                __SetPixel(GRN);
        
            r -= k;
        }
    }

    if(p!=0 && ProgFileSec(DiskBuf, pCluster)!=OK)          // write remnant
        return FILE_RW_ERR;

    if(CloseFile(DiskBuf, 76*512, pCluster, pDirAddr) != OK) 
        return FILE_RW_ERR;
    
    return OK;
}

/*******************************************************************************
 colorNum: Find the corresponding color palette number of the current color
*******************************************************************************/
u8 colorNum(u16 Color)
{
    if(Color == WHT)                return 0;
    else if((Color & CYN)  == CYN)  return 1;
    else if((Color & CYN_) == CYN_) return 2;
    else if((Color & YEL)  == YEL)  return 3;
    else if((Color & YEL_) == YEL_) return 4;
    else if((Color & PUR)  == PUR)  return 5;
    else if((Color & PUR_) == PUR_) return 6;
    else if((Color & GRN)  == GRN)  return 7;
    else if((Color & GRN_) == GRN_) return 8;
    else if((Color & GRY)  == GRY)  return 9;
    else if((Color & ORN)  == ORN)  return 10;
    else if((Color & BLU)  == BLU)  return 11;
    else if((Color & RED)  == RED)  return 12;
    else if(Color == BLK)           return 13;
    else if((Color & LGN) == LGN)   return 14;
    else                            return 15;
}

/*******************************************************************************
 Save_Param: Save current working parameters to U disk - Return: 0=Success
*******************************************************************************/
// u8 Save_Param(void)
// {
//     u8 Sum = 0, Filename[12], Version = PAR_VERSION;
//     u16 i, Tmp[2];
//     u16* ptr = (u16*)DiskBuf;
// 
//     u16 pCluster[3];
//     u32 pDirAddr[1];
// 
//     Word2Hex(Filename, __Info(DEV_SN));
// 
//     #if defined (APP1)
//         Filename[8] = 'P'; Filename[9] = 'A'; Filename[10] = 'R';
//     #elif defined (APP2)
//         Filename[8] = 'P'; Filename[9] = 'A'; Filename[10] = '2';
//     #endif
// 
//     switch(OpenFileRd(DiskBuf, Filename, pCluster, pDirAddr)){
//         case OK:                                                            // the original WPT file exists
//             Tmp[0] = *pCluster;
//             
//             #if defined (APP1)
//                 Filename[8] = 'B'; Filename[9] = 'A'; Filename[10] = 'K';   // convert to BAK file
//             #elif defined (APP2)
//                 Filename[8] = 'B'; Filename[9] = 'A'; Filename[10] = '2';  
//             #endif
// 
//             if(OpenFileWr(DiskBuf, Filename, pCluster, pDirAddr) != OK)
//                 return DISK_RW_ERR;
// 
//             if(ReadFileSec(DiskBuf, Tmp) != OK) 
//                 return FILE_RW_ERR;
// 
//             if(ProgFileSec(DiskBuf, pCluster) != OK) 
//                 return FILE_RW_ERR;                                         // save BAK file
// 
//             if(CloseFile(DiskBuf, 512, pCluster, pDirAddr) != OK)
//                 return FILE_RW_ERR;
// 
//         case NEW:                                                           // original WPT file does not exist
//             #if defined (APP1)
//                 Filename[8] = 'P'; Filename[9] = 'A'; Filename[10] = 'R';   // create WPT file
//             #elif defined (APP2)
//                 Filename[8] = 'P'; Filename[9] = 'A'; Filename[10] = '2';   
//             #endif
// 
//             if(OpenFileWr(DiskBuf, Filename, pCluster, pDirAddr) != OK)
//                 return DISK_RW_ERR;
// 
//             memset(DiskBuf, 0, 512);
//             *ptr++ = Version;                                              // save version
// 
//             for(i=RTIM; i<cPrep.numItm; i++)
//                 *ptr++ = cPrep.val[i];
// 
//             for(i=OFFA; i<cRun.numItm; i++)
//                 *ptr++ = cRun.val[i];
// 
//             for(i=0; i<511; i++)                                            // calculate the parameter table checksum
//                 Sum += DiskBuf[i];
//             
//             DiskBuf[511] = (~Sum) + 1;                                      // neg
// 
//             if(ProgFileSec(DiskBuf, pCluster) != OK)                        // write data
//                 return FILE_RW_ERR; 
// 
//             if(CloseFile(DiskBuf, 512, pCluster, pDirAddr) != OK)
//                 return FILE_RW_ERR;
// 
//             return OK;
// 
//         default:  
//             return FILE_RW_ERR;
//     }
// }
