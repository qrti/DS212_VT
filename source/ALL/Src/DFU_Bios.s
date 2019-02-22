;/******************** (C) COPYRIGHT 2017 e-Design Co.,Ltd. ********************
; File Name : DFU_Bios.s
; Version   : For DS212                                           Author : bure
;******************************************************************************/

        RSEG BIOS:CODE(4)

;//=============================================================================
;//                 DFU Module related function entry address definition
;//=============================================================================

;//=============================================================================
        EXPORT  __Info
__Info  
        LDR     PC,=0x08000191          ;absolute value error in IAR 8 -> B.W    0x08000191
;//=============================================================================
        EXPORT  __Ctrl
__Ctrl  
        LDR      PC,=0x08000195
;//=============================================================================
        EXPORT  __Lic_Gen
__Lic_Gen
        LDR      PC,=0x08000199
;//=============================================================================
        EXPORT  __Ident
__Ident
        LDR      PC,=0x0800019D
;//=============================================================================
        EXPORT  __ExtFlashSecWr
__ExtFlashSecWr
        LDR      PC,=0x080001A1
;//=============================================================================
        EXPORT  __ExtFlashDataRd
__ExtFlashDataRd
        LDR      PC,=0x080001A5
;//=============================================================================
        EXPORT  __I2C_Write
__I2C_Write
        LDR      PC,=0x080001A9
;//=============================================================================
        EXPORT  __I2C_Read
__I2C_Read
        LDR      PC,=0x080001AD
;//=============================================================================
        EXPORT  __SetBlock
__SetBlock
        LDR      PC,=0x080001B1
;//=============================================================================
        EXPORT  __SetPosi
__SetPosi
        LDR      PC,=0x080001B5
;//=============================================================================
        EXPORT  __SetPixel
__SetPixel
        LDR      PC,=0x080001B9
;//=============================================================================
        EXPORT  __SendPixels
__SendPixels
        LDR      PC,=0x080001BD
;//=============================================================================
        EXPORT  __ReadPixel
__ReadPixel
        LDR      PC,=0x080001C1
;//=============================================================================
        EXPORT  __Disp_Logo
__Disp_Logo
        LDR      PC,=0x080001C5
;//=============================================================================
        EXPORT  __Font_8x14
__Font_8x14
        LDR      PC,=0x080001C9
;//=============================================================================
        EXPORT  __Disp_Str
__Disp_Str
        LDR      PC,=0x080001CD
;//=============================================================================
        EXPORT  __FLASH_Prog
__FLASH_Prog
        LDR      PC,=0x080001D1
;//=============================================================================
        EXPORT  __FLASH_Erase
__FLASH_Erase
        LDR      PC,=0x080001D5
;//=============================================================================
        EXPORT  __Disp_OEM
__Disp_OEM
        LDR      PC,=0x080001D9
;//=============================================================================

        END
;******************************* END OF FILE ***********************************

