// All rights reserved ADENEO EMBEDDED 2010
// Portions Copyright (c) 2009 BSQUARE Corporation. All rights reserved.
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this source code is subject to the terms of the Microsoft end-user
// license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
// If you did not accept the terms of the EULA, you are not authorized to use
// this source code. For a copy of the EULA, please see the LICENSE.RTF on your
// install media.
//
// Portions Copyright (c) Texas Instruments.  All rights reserved.
//
//------------------------------------------------------------------------------
//
#include <windows.h>
#include <bsp.h>
#include <ceddk.h>
#include <ceddkex.h>

#include "sdk_i2c.h"
#include "sdk_gpio.h"

#include "ispreg.h"
#include "tvp5146.h"
#include "util.h"
#include "params.h"
#include "ISPctrl.h"
#include "dbgsettings.h"

#define FOURCC_UYVY     mmioFOURCC('U', 'Y', 'V', 'Y')  // MSYUV: 1394 conferencing camera 4:4:4 mode 1 and 3
#define FOURCC_YUY2     mmioFOURCC('Y', 'U', 'Y', '2')

//-----------------------------------------------------------------------------
// Global variable

//------------------------------------------------------------------------------
//  Variable define
#define GPIO_MODE_MASK_EVENPIN  0xfffffff8
#define GPIO_MODE_MASK_ODDPIN   0xfff8ffff

//------------------------------------------------------------------------------
//  Buffer size define
#ifdef ENABLE_PACK8
    #define IMAGE_CAMBUFF_SIZE              (MAX_X_RES*MAX_Y_RES*2)//video input buffer size
    #define NUM_BYTES_LINE                  (MAX_X_RES*2)   
#else
    #define IMAGE_CAMBUFF_SIZE              (MAX_X_RES*MAX_Y_RES*4)//video input buffer size
    #define NUM_BYTES_LINE                  (MAX_X_RES*4)
#endif //ENABLE_PACK8


CIspCtrl::CIspCtrl()
{   
    m_pIspConfigRegs=NULL;
    m_pCCDCRegs = NULL;
    m_pYUVDMAAddr=NULL;
    m_pYUVVirtualAddr=NULL;
    m_pYUVPhysicalAddr=NULL;
    m_bEnabled= FALSE;
}

CIspCtrl::~CIspCtrl()
{
}

//-----------------------------------------------------------------------------
//
//  Function:       GetPhysFromVirt
//
//  Maps the Virtual address passed to a physical address.
//
//  returns a physical address with a page boundary size.
//
LPVOID
CIspCtrl::GetPhysFromVirt(
    ULONG ulVirtAddr
    )
{
    ULONG aPFNTab[1];
    ULONG ulPhysAddr;


    if (LockPages((LPVOID)ulVirtAddr, UserKInfo[KINX_PAGESIZE], aPFNTab,
                   LOCKFLAG_QUERY_ONLY))
        {
         // Merge PFN with address offset to get physical address         
         ulPhysAddr= ((*aPFNTab << UserKInfo[KINX_PFN_SHIFT]) & UserKInfo[KINX_PFN_MASK])|(ulVirtAddr & 0xFFF);
        } else {
        ulPhysAddr = 0;
        }
    return ((LPVOID)ulPhysAddr);
}
//------------------------------------------------------------------------------
//
//  Function:  MapCameraReg
//
//  Read data from register
//      
BOOL CIspCtrl::MapCameraReg()
{
        DEBUGMSG(ZONE_FUNCTION,(TEXT("+MapCameraReg\r\n")));
        PHYSICAL_ADDRESS pa;
//    UINT32 *pAddress = NULL;

        
        //To map Camera ISP register
    pa.QuadPart = CAM_ISP_CONFIG_BASE_ADDRESS;
    m_pIspConfigRegs = (CAM_ISP_CONFIG_REGS *)MmMapIoSpace(pa, sizeof(CAM_ISP_CONFIG_REGS), FALSE);
    if (m_pIspConfigRegs == NULL)
    {
        ERRORMSG(ZONE_ERROR,(TEXT("Failed map Camera ISP CONFIG physical address to virtual address!\r\n")));
        return FALSE;
    }
    
    //To map Camera ISP CCDC register
    pa.QuadPart = CAM_ISP_CCDC_BASE_ADDRESS;
    m_pCCDCRegs = (CAM_ISP_CCDC_REGS *)MmMapIoSpace(pa, sizeof(CAM_ISP_CCDC_REGS), FALSE);
    if (m_pCCDCRegs == NULL)
    {
        ERRORMSG(ZONE_ERROR,(TEXT("Failed map Camera ISP CCDC physical address to virtual address!\r\n")));
        return FALSE;
    }
    
    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  ConfigGPIO4MDC
//
//  Set GPIO58  mode4 output  0->1 (reset)
//  Set GPIO134 mode4 output  0 
//  Set GPIO136 mode4 output  1 
//  Set GPIO54  mode4 output  1         
//  Set GPIO139 mode4 input 
//
BOOL CIspCtrl::ConfigGPIO4MDC()
{
    DEBUGMSG(ZONE_FUNCTION,(TEXT("+ConfigGPIO4MDC\r\n")));

    HANDLE hGpio = GPIOOpen();
  
    // CAM_VD_EN_1V8
    GPIOClrBit(hGpio, VIDEO_CAPTURE_SELECT1);
    GPIOSetMode(hGpio, VIDEO_CAPTURE_SELECT1, GPIO_DIR_OUTPUT);

    // CAM_VD_SEL_1V8
    GPIOSetBit(hGpio, VIDEO_CAPTURE_SELECT2);
    GPIOSetMode(hGpio, VIDEO_CAPTURE_SELECT2, GPIO_DIR_OUTPUT);

    Sleep(20);
    GPIOClrBit(hGpio, VIDEO_CAPTURE_RESET);
    GPIOSetMode(hGpio, VIDEO_CAPTURE_RESET, GPIO_DIR_OUTPUT);
    Sleep(20);
    GPIOSetBit(hGpio, VIDEO_CAPTURE_RESET);
    GPIOSetMode(hGpio, VIDEO_CAPTURE_RESET, GPIO_DIR_OUTPUT);

    GPIOClose(hGpio);


#ifndef BSP_EVM2
    PHYSICAL_ADDRESS pa;
    CAM_GPIO_REGS   *pCamGPIO;

    // map in pad control registers for camera GPIO pins
    pa.QuadPart = CAM_GPIO_BASE_ADDRESS;
    pCamGPIO = (CAM_GPIO_REGS *)MmMapIoSpace(pa, sizeof(CAM_GPIO_REGS), FALSE);
    if (pCamGPIO == NULL)
    {
        ERRORMSG(ZONE_ERROR,(TEXT("Failed map Camera GPIO physical address to virtual address!\r\n")));
        return FALSE;
    }

    UINT32 reg_value;
    
    //Set GPIO54 mode4
    reg_value = INREG32(&pCamGPIO->GPIO54);
    reg_value &= GPIO_MODE_MASK_EVENPIN;
    reg_value |= 0x4;
    OUTREG32(&pCamGPIO->GPIO54, reg_value);
    
    //Set GPIO58 mode4
    reg_value = INREG32(&pCamGPIO->GPIO58);
    reg_value &= GPIO_MODE_MASK_EVENPIN;
    reg_value |= 0x4;
    OUTREG32(&pCamGPIO->GPIO58, reg_value);
    
    //Set GPIO134 mode4
    reg_value = INREG32(&pCamGPIO->GPIO134);
    reg_value &= GPIO_MODE_MASK_EVENPIN;
    reg_value |= 0x4;
    OUTREG32(&pCamGPIO->GPIO134, reg_value);
    
    //Set GPIO136 mode4
    reg_value = INREG32(&pCamGPIO->GPIO136);
    reg_value &= GPIO_MODE_MASK_EVENPIN;
    reg_value |= 0x4;
    OUTREG32(&pCamGPIO->GPIO136, reg_value);        
    
    //Set GPIO139 mode4
    reg_value = INREG32(&pCamGPIO->GPIO139);
    reg_value &= GPIO_MODE_MASK_ODDPIN;
    reg_value |= 0x00040000;
    OUTREG32(&pCamGPIO->GPIO139, reg_value);    
    

    HANDLE hGPIO = NULL;    
    hGPIO = CreateFile(L"GIO1:",  
    GENERIC_READ | GENERIC_WRITE,       
    FILE_SHARE_READ|FILE_SHARE_WRITE,                                
    NULL,                         
    OPEN_EXISTING,              
    0,                                
    NULL);                            

    DEBUGMSG(ZONE_VERBOSE, (TEXT("hGPIO=0x%x. \r\n"),hGPIO)); 
    
    DWORD   GPIO_PIN_Number;
    DWORD GPIOoutBuffer ;
    DWORD GPIOinBuffer[2];
    
    // set GPIO58 0 -> 1 (reset)

    GPIOinBuffer[0] = 58;
    GPIOinBuffer[1] = GPIO_DIR_OUTPUT;
    if (DeviceIoControl(hGPIO, IOCTL_GPIO_SETMODE, &GPIOinBuffer, 
            sizeof(GPIOinBuffer), &GPIOoutBuffer, sizeof(GPIOoutBuffer), NULL, NULL) == 0)
    {
        ERRORMSG(ZONE_ERROR, (TEXT("DeviceIoControl:IOCTL_GPIO_SETMODE of GPIO58 communication has problem. \r\n")));    
        return FALSE;
    }       
    
    GPIO_PIN_Number = 58;
    
    if (DeviceIoControl(hGPIO, IOCTL_GPIO_CLRBIT, &GPIO_PIN_Number, 
            sizeof(GPIO_PIN_Number), &GPIOoutBuffer, sizeof(GPIOoutBuffer), NULL, NULL) == 0)
    {
        DEBUGMSG(ZONE_ERROR, (TEXT("DeviceIoControl:IOCTL_GPIO_CLRBIT of GPIO58 communication has problem. \r\n")));    
        return FALSE;
    }       
    
    Sleep(10);

    if (DeviceIoControl(hGPIO, IOCTL_GPIO_SETBIT, &GPIO_PIN_Number, 
            sizeof(GPIO_PIN_Number), &GPIOoutBuffer, sizeof(GPIOoutBuffer), NULL, NULL) == 0)
    {
        DEBUGMSG(ZONE_ERROR, (TEXT("DeviceIoControl:IOCTL_GPIO_CLRBIT of GPIO58 communication has problem. \r\n")));    
        return FALSE;
    }
    // Set GPIO134 "0"
    
    GPIOinBuffer[0] = 134;
    GPIOinBuffer[1] = GPIO_DIR_OUTPUT;
    if (DeviceIoControl(hGPIO, IOCTL_GPIO_SETMODE, &GPIOinBuffer, 
            sizeof(GPIOinBuffer), &GPIOoutBuffer, sizeof(GPIOoutBuffer), NULL, NULL) == 0)
    {
        DEBUGMSG(ZONE_ERROR, (TEXT("DeviceIoControl:IOCTL_GPIO_SETMODE of GPIO134 communication has problem. \r\n")));    
        return FALSE;
    }       
    
    GPIO_PIN_Number = 134;
    if (DeviceIoControl(hGPIO, IOCTL_GPIO_CLRBIT, &GPIO_PIN_Number, 
            sizeof(GPIO_PIN_Number), &GPIOoutBuffer, sizeof(GPIOoutBuffer), NULL, NULL) == 0)
    {
        DEBUGMSG(ZONE_ERROR, (TEXT("DeviceIoControl:IOCTL_GPIO_CLRBIT of GPIO134 communication has problem. \r\n")));    
        return FALSE;
    }
    
    // Set GPIO136 "1"
    
    GPIOinBuffer[0] = 136;
    GPIOinBuffer[1] = GPIO_DIR_OUTPUT;
    if (DeviceIoControl(hGPIO, IOCTL_GPIO_SETMODE, &GPIOinBuffer, 
            sizeof(GPIOinBuffer), &GPIOoutBuffer, sizeof(GPIOoutBuffer), NULL, NULL) == 0)
    {
        DEBUGMSG(ZONE_ERROR, (TEXT("DeviceIoControl:IOCTL_GPIO_SETMODE of GPIO136 communication has problem. \r\n")));    
        return FALSE;
    }       
    
    GPIO_PIN_Number = 136;
    if (DeviceIoControl(hGPIO, IOCTL_GPIO_SETBIT, &GPIO_PIN_Number, 
            sizeof(GPIO_PIN_Number), &GPIOoutBuffer, sizeof(GPIOoutBuffer), NULL, NULL) == 0)
    {
        DEBUGMSG(ZONE_ERROR, (TEXT("DeviceIoControl:IOCTL_GPIO_CLRBIT of GPIO136 communication has problem. \r\n")));    
        return FALSE;
    }
    
    //set GPIO54 "1"
    
    GPIOinBuffer[0] = 54;
    GPIOinBuffer[1] = GPIO_DIR_OUTPUT;
    if (DeviceIoControl(hGPIO, IOCTL_GPIO_SETMODE, &GPIOinBuffer, 
            sizeof(GPIOinBuffer), &GPIOoutBuffer, sizeof(GPIOoutBuffer), NULL, NULL) == 0)
    {
        DEBUGMSG(ZONE_ERROR, (TEXT("DeviceIoControl:IOCTL_GPIO_SETMODE of GPIO54 communication has problem. \r\n")));    
        return FALSE;
    }       
    
    GPIO_PIN_Number = 54;
    if (DeviceIoControl(hGPIO, IOCTL_GPIO_SETBIT, &GPIO_PIN_Number, 
            sizeof(GPIO_PIN_Number), &GPIOoutBuffer, sizeof(GPIOoutBuffer), NULL, NULL) == 0)
    {
        DEBUGMSG(ZONE_ERROR, (TEXT("DeviceIoControl:IOCTL_GPIO_CLRBIT of GPIO54 communication has problem. \r\n")));    
        return FALSE;
    }
    // Set GPIO139 input
    
    GPIOinBuffer[0] = 139;
    GPIOinBuffer[1] = GPIO_DIR_OUTPUT;
    if (DeviceIoControl(hGPIO, IOCTL_GPIO_SETMODE, &GPIOinBuffer, 
            sizeof(GPIOinBuffer), &GPIOoutBuffer, sizeof(GPIOoutBuffer), NULL, NULL) == 0)
    {
        DEBUGMSG(ZONE_ERROR, (TEXT("DeviceIoControl:IOCTL_GPIO_SETMODE of GPIO139 communication has problem. \r\n")));    
        return FALSE;
    }   

    // Close GPIO handle
    if(CloseHandle(hGPIO) == 0)    // Call this function to close port.
    {
        DEBUGMSG(ZONE_ERROR, (TEXT("CloseHandle(hGPIO) failed.\r\n")));    
        return FALSE;
    }       

#else
    // GPIO setup is handled by XLDR/EBOOT
#endif  

    return TRUE;
}       

//------------------------------------------------------------------------------
//
//  Function:  CCDCInitSYNC
//
//  Init. ISPCCDC_SYN_MODE register 
//
//
BOOL CIspCtrl::CCDCInitSYNC()
{
        DEBUGMSG(ZONE_FUNCTION, (TEXT("+CCDCInitSYNC\r\n")));
        
        UINT32 syn_mode = 0 ;
        syn_mode |= ISPCCDC_SYN_MODE_WEN;// Video data to memory 
        syn_mode |= ISPCCDC_SYN_MODE_DATSIZ_10;// cam_d is 10 bits
        syn_mode |= ISPCCDC_SYN_MODE_VDHDEN;// Enable timing generator
#ifdef ENABLE_PROGRESSIVE_INPUT
        syn_mode |= ISPCCDC_SYN_MODE_FLDMODE_PROGRESSIVE; // Set field mode: interlaced
#else
        syn_mode |= ISPCCDC_SYN_MODE_FLDMODE_INTERLACED; // Set field mode: interlaced
#endif

        syn_mode = (syn_mode & ISPCCDC_SYN_MODE_INPMOD_MASK)| ISPCCDC_SYN_MODE_INPMOD_RAW;//Set input mode: RAW
#ifdef ENABLE_PACK8     
        syn_mode |=ISPCCDC_SYN_MODE_PACK8; //pack 8-bit in memory
#endif //   ENABLE_PACK8    
        ISP_OutReg32(&m_pCCDCRegs->CCDC_SYN_MODE, syn_mode);
        
#ifdef ENABLE_BT656     
        ISP_OutReg32(&m_pCCDCRegs->CCDC_REC656IF, ISPCCDC_REC656IF_R656ON); //enable BT656
#endif //ENABLE_BT656
        return TRUE;
}
//------------------------------------------------------------------------------
//
//  Function:  CCDCInitCFG
//
//  Init. Camera CCDC   
//
BOOL CIspCtrl::CCDCInitCFG(PCS_DATARANGE_VIDEO pCsDataRangeVideo)
{
        DEBUGMSG(ZONE_FUNCTION, (TEXT("+CCDCInitCFG\r\n")));

        PCS_VIDEOINFOHEADER pCsVideoInfoHdr = &(pCsDataRangeVideo->VideoInfoHeader);

        UINT biCompression = pCsVideoInfoHdr->bmiHeader.biCompression & ~BI_SRCPREROTATE;

        // Request Init
        UINT32 setting = 0 ;
        ISP_InReg32(&m_pCCDCRegs->CCDC_CFG, &setting);
        
        setting |= (ISPCCDC_CFG_VDLC | (1 << ISPCCDC_CFG_FIDMD_SHIFT)); // must be set to 1 according to TRM

        setting &= ~ISPCCDC_CFG_MSBINVI; // MSB of chroma input signal not inverted when stored to memory
        
        #ifdef ENABLE_PACK8
            if (FOURCC_UYVY == biCompression) {
                setting &= ~ISPCCDC_CFG_BSWD; // normal - don't swap bytes
            }
            else if (FOURCC_YUY2 == biCompression) {
                setting |= ISPCCDC_CFG_BSWD; //swap byte
            }
            else {
                // Unsupported format
                DEBUGMSG(ZONE_ERROR, 
                    (TEXT("CIspCtrl::CCDCInitCFG: ERROR - Unsupported FOURCC\r\n")));
                return FALSE;
            }
        #endif
        
        #ifdef ENABLE_BT656
            #ifndef ENABLE_PACK8
                setting |=ISPCCDC_CFG_BW656; //using 10-bit BT656
            #endif //ENABLE_PACK8
        #endif //ENABLE_BT656
        
        ISP_OutReg32(&m_pCCDCRegs->CCDC_CFG, setting);
        return TRUE;
}
//------------------------------------------------------------------------------
//
//  Function:  ConfigOutlineOffset
//
//  Configures the output line offset when stored in memory.
//  Configures the num of even and odd line fields in case of rearranging
//  the lines
//  offset: twice the Output width and aligned on 32byte boundary.
//  oddeven: odd/even line pattern to be chosen to store the output
//  numlines: Configure the value 0-3 for +1-4lines, 4-7 for -1-4lines
//
BOOL CIspCtrl::ConfigOutlineOffset(UINT32 offset, UINT8 oddeven, UINT8 numlines)
{       
     DEBUGMSG(ZONE_FUNCTION, (TEXT("+ConfigOutlineOffset\r\n")));
     UINT32 setting = 0;    
     
    // Make sure offset is multiple of 32bytes. ie last 5bits should be zero 
    setting = offset & ISP_32B_BOUNDARY_OFFSET;
    ISP_OutReg32(&m_pCCDCRegs->CCDC_HSIZE_OFF, setting);

    // By default Donot inverse the field identification 
    ISP_InReg32(&m_pCCDCRegs->CCDC_SDOFST, &setting);
    setting &= (~ISPCCDC_SDOFST_FINV);
    ISP_OutReg32(&m_pCCDCRegs->CCDC_SDOFST, setting);

    // By default one line offset
    ISP_InReg32(&m_pCCDCRegs->CCDC_SDOFST, &setting);
    setting &= ISPCCDC_SDOFST_FOFST_1L;
    ISP_OutReg32(&m_pCCDCRegs->CCDC_SDOFST, setting);

    switch (oddeven) {
    case EVENEVEN:      /*even lines even fields*/
        ISP_InReg32(&m_pCCDCRegs->CCDC_SDOFST, &setting);
        setting |= ((numlines & 0x7) << ISPCCDC_SDOFST_LOFST0_SHIFT);
        ISP_OutReg32(&m_pCCDCRegs->CCDC_SDOFST, setting);
        break;
    case ODDEVEN:       /*odd lines even fields*/
        ISP_InReg32(&m_pCCDCRegs->CCDC_SDOFST, &setting);
        setting |= ((numlines & 0x7) << ISPCCDC_SDOFST_LOFST1_SHIFT);
        ISP_OutReg32(&m_pCCDCRegs->CCDC_SDOFST, setting);
        break;
    case EVENODD:       /*even lines odd fields*/
        ISP_InReg32(&m_pCCDCRegs->CCDC_SDOFST, &setting);
        setting |= ((numlines & 0x7) << ISPCCDC_SDOFST_LOFST2_SHIFT);
        ISP_OutReg32(&m_pCCDCRegs->CCDC_SDOFST, setting);
        break;
    case ODDODD:        /*odd lines odd fields*/
        ISP_InReg32(&m_pCCDCRegs->CCDC_SDOFST, &setting);
        setting |= ((numlines & 0x7) << ISPCCDC_SDOFST_LOFST3_SHIFT);
        ISP_OutReg32(&m_pCCDCRegs->CCDC_SDOFST, setting);
        break;
    default:
        break;
    }
    return TRUE;
}       
//------------------------------------------------------------------------------
//
//  Function:  CCDCSetOutputAddress
//
//  Configures the memory address where the output should be stored.
//
BOOL CIspCtrl::CCDCSetOutputAddress(ULONG SDA_Address)
{       
        //DEBUGMSG(ISP_MSG_FLAG, (TEXT("+TVPReadReg(SlaveAddress0x81: 0x%x)\r\n"), reg_value));
                
        ULONG addr = (SDA_Address) ;
        addr = addr & ISP_32B_BOUNDARY_BUF;
        ISP_OutReg32(&m_pCCDCRegs->CCDC_SDR_ADDR, addr);
        return TRUE;            
}   
//------------------------------------------------------------------------------
//
//  Function:  CCDCEnable
//
//  Enables the CCDC module.
//
BOOL CIspCtrl::CCDCEnable(BOOL bEnable)
{       
        DEBUGMSG(ZONE_FUNCTION, (TEXT("+CCDCEnable\r\n"))); 
        BOOL rc = FALSE;
        UINT32 setting = 0 ;
        
        ISP_InReg32(&m_pCCDCRegs->CCDC_PCR, &setting);  
        if (bEnable == 1)
            setting |= (ISPCCDC_PCR_EN);
        else
            setting &= ~(ISPCCDC_PCR_EN);
            
        rc = ISP_OutReg32(&m_pCCDCRegs->CCDC_PCR, setting);
        return rc;          
}   

//------------------------------------------------------------------------------
//
//  Function:  AllocBuffer
//
//  AllocBuffer for video input and format transfer out 
//
BOOL CIspCtrl::AllocBuffer()
{
    DEBUGMSG(ZONE_FUNCTION, (TEXT("+AllocBuffer\r\n")));
    if(m_pYUVDMAAddr)
        return TRUE;
    DWORD dwSize = IMAGE_CAMBUFF_SIZE;
    PHYSICAL_ADDRESS   pCamBufferPhys; 
    DMA_ADAPTER_OBJECT camBuffer;
        
    camBuffer.ObjectSize = sizeof(camBuffer);
    camBuffer.InterfaceType = Internal;
    camBuffer.BusNumber = 0;        
    
    m_pYUVDMAAddr = (PBYTE)HalAllocateCommonBuffer(&camBuffer, dwSize, &pCamBufferPhys, TRUE );
    
    if (m_pYUVDMAAddr == NULL)
    {   
        ERRORMSG(ZONE_ERROR, (TEXT("HalAllocateCommonBuffer failed !!!\r\n")));
        return FALSE;
    }
        
        m_pYUVVirtualAddr = (PBYTE)VirtualAlloc(NULL,dwSize, MEM_RESERVE,PAGE_NOACCESS);        

        if (m_pYUVVirtualAddr == NULL)
        {   ERRORMSG(ZONE_ERROR, (TEXT("Sensor buffer memory alloc failed !!!\r\n")));
            return FALSE;
        }
      
      VirtualCopy(m_pYUVVirtualAddr, (VOID *) (pCamBufferPhys.LowPart >> 8), dwSize, PAGE_READWRITE | PAGE_PHYSICAL); // | PAGE_NOCACHE  );
      
      m_pYUVPhysicalAddr = GetPhysFromVirt((ULONG)m_pYUVVirtualAddr);
      if(!m_pYUVPhysicalAddr)
      {
        ERRORMSG(ZONE_ERROR,(_T("GetPhysFromVirt 0x%08X failed: \r\n"), m_pYUVVirtualAddr));
        return FALSE;
      }
        
      DEBUGMSG(MASK_DMA, (TEXT("m_pYUVVirtualAddr=0x%x\r\n"),m_pYUVVirtualAddr));   
      DEBUGMSG(MASK_DMA, (TEXT("m_pYUVPhysicalAddr=0x%x\r\n"),m_pYUVPhysicalAddr)); 
      DEBUGMSG(MASK_DMA, (TEXT("m_pYUVDMAAddr=0x%x\r\n"),m_pYUVDMAAddr));   
      
      return TRUE;      
}   
//------------------------------------------------------------------------------
//
//  Function:  DeAllocBuffer
//
//  DeAllocBuffer for video input and format transfer out   
//
BOOL CIspCtrl::DeAllocBuffer()
{
    DEBUGMSG(ZONE_FUNCTION, (TEXT("+DeAllocBuffer\r\n")));
    //Do not free memory to prevent the memory fragment.
#if 0   
    if(!m_pYUVDMAAddr)
        return TRUE;
    if(!VirtualFree( m_pYUVDMAAddr, 0, MEM_RELEASE ))
        DEBUGMSG(1,(_T("CIspCtrl::DeAllocBuffer failed \r\n")));
        
    m_pYUVVirtualAddr=NULL;
    m_pYUVPhysicalAddr=NULL;
    m_pYUVDMAAddr=NULL;
#endif  
    return TRUE;    
}
//------------------------------------------------------------------------------
//
//  Function:  CCDCInit
//
//  Init. Camera CCDC   
//
BOOL CIspCtrl::CCDCInit(PCS_DATARANGE_VIDEO pCsDataRangeVideo)
{
    DEBUGMSG(ZONE_FUNCTION, (TEXT("+CCDCInit\r\n")));

    if (!CCDCInitCFG(pCsDataRangeVideo))
        return FALSE;
        
    if (!CCDCInitSYNC())
        return FALSE;

    // Allocate buffer for YUV
    if(!AllocBuffer())
        return FALSE;
    
    //Set CCDC_SDR address
    if (!CCDCSetOutputAddress((ULONG) m_pYUVPhysicalAddr)) {
        DeAllocBuffer();
        return FALSE;
    }
    
    return TRUE;        
}       

//------------------------------------------------------------------------------
//
//  Function:  ISPInit
//
//  Init. Camera ISP
//
BOOL CIspCtrl::ISPInit()
{
    DEBUGMSG(ZONE_FUNCTION, (TEXT("+ISPInit\r\n")));
            
    UINT32 setting = 0;

    // Init ISP power capability
    ISP_InReg32(&m_pIspConfigRegs->SYSCONFIG, &setting);
    setting &= ~ISP_SYSCONFIG_AUTOIDLE;// Disable auto idle for subsystem
    setting |= (ISP_SYSCONFIG_MIdleMode_NoStandBy << ISP_SYSCONFIG_MIdleMode_SHIFT);// No standby   
    ISP_OutReg32(&m_pIspConfigRegs->SYSCONFIG, setting);

    // Disable all interrupts and clear interrupt status 
    setting = 0;
    ISP_OutReg32(&m_pIspConfigRegs->IRQ0ENABLE, setting);
    ISP_OutReg32(&m_pIspConfigRegs->IRQ1ENABLE, setting);

    ISP_InReg32(&m_pIspConfigRegs->IRQ0STATUS, &setting);
    ISP_OutReg32(&m_pIspConfigRegs->IRQ0STATUS, setting);
    ISP_InReg32(&m_pIspConfigRegs->IRQ1STATUS, &setting);
    ISP_OutReg32(&m_pIspConfigRegs->IRQ1STATUS, setting);
        

    return TRUE;        
}
//------------------------------------------------------------------------------
//
//  Function:  ISPEnable
//
//  Reset and enable ISP need component
//
BOOL CIspCtrl::ISPEnable(BOOL bEnable)
{
      
      DEBUGMSG(ZONE_FUNCTION, (TEXT("+ISPEnable\r\n")));
        UINT32 setting = 0;
        ULONG ulTimeout = 50;       

        if (bEnable == TRUE)
        {
            ISP_InReg32(&m_pIspConfigRegs->SYSCONFIG, &setting);    
            setting |= (ISP_SYSCONFIG_SOFTRESET);   
            ISP_OutReg32(&m_pIspConfigRegs->SYSCONFIG, setting);
             
             
            // Wait till the isp wakes out of reset 
            ISP_InReg32(&m_pIspConfigRegs->SYSSTATUS, &setting);
            setting &= 0x1;

            while ((setting != 0x1) && ulTimeout--)
            {     
                DEBUGMSG(ZONE_VERBOSE, (TEXT("+ISPInit: reset not completed,ulTimeout=%d\r\n"),ulTimeout));
                Sleep(10);// Reset not completed
                ISP_InReg32(&m_pIspConfigRegs->SYSSTATUS, &setting);
                setting &= (0x1);
            }  
            DEBUGMSG(ZONE_VERBOSE, (TEXT("+ISPEnable: soft reset completed: setting=%d, ulTimeout=%d\r\n"),setting,ulTimeout));
        }    
        setting = 0;
    if (bEnable == TRUE)
    {
        // Enable using module clock and disable SBL_AUTOIDLE, PAR BRIDGE
        setting |= (ISPCTRL_CCDC_WEN_POL | ISPCTRL_CCDC_CLK_EN | ISPCTRL_CCDC_RAM_EN | (ISPCTRL_SYNC_DETECT_VSFALL << ISPCTRL_SYNC_DETECT_SHIFT) | (0 << ISPCTRL_SHIFT_SHIFT) | ISPCTRL_CCDC_FLUSH
                    |ISPCTRL_SBL_WR0_RAM_EN |ISPCTRL_SBL_WR1_RAM_EN | ISPCTRL_SBL_RD_RAM_EN  | (0 << ISPCTRL_PAR_BRIDGE_SHIFT)                    
                               );
#ifdef ENABLE_PACK8                            
            setting |=ISPCTRL_SHIFT_2;   
#endif //ENABLE_PACK8           
            ISP_OutReg32(&m_pIspConfigRegs->CTRL, setting);
                
            // Enable IRQ0
        setting = 0xFFFFFFFF;
        ISP_OutReg32(&m_pIspConfigRegs->IRQ0ENABLE, setting);    
        }
        else
        {   
            // Disable using module clock
        setting &= ~(ISPCTRL_CCDC_CLK_EN | ISPCTRL_CCDC_RAM_EN |ISPCTRL_SBL_WR0_RAM_EN |ISPCTRL_SBL_WR1_RAM_EN | ISPCTRL_SBL_RD_RAM_EN | ISPCTRL_RSZ_CLK_EN);                  
            ISP_OutReg32(&m_pIspConfigRegs->CTRL, setting);
            
            // Disable IRQ0
        setting = 0;
        ISP_OutReg32(&m_pIspConfigRegs->IRQ0ENABLE, setting);  
        }
     
        return TRUE;        
}
//------------------------------------------------------------------------------
//
//  Function:  ISPConfigSize
//
//
// Configures CCDC HORZ/VERT_INFO registers to decide the start line
// stored in memory.
//
// output_w : output width from the CCDC in number of pixels per line
// output_h : output height for the CCDC in number of lines
//
//
BOOL CIspCtrl::ISPConfigSize(UINT width, UINT height, UINT bpp)
{

    DEBUGMSG(ZONE_FUNCTION, (TEXT("+ISPConfigSize\r\n")));

    UINT32 setting ;
    UINT numBytesLine = 0;
    UINT lines = 0;

    if (bpp != 16) {
        DEBUGMSG(ZONE_ERROR, 
            (TEXT("CIspCtrl::ISPConfigSize: ERROR - Unsupported bpp value %d\r\n"), 
            bpp));
        return FALSE;
    }

    numBytesLine = (width * bpp)/8;

#ifdef ENABLE_PROGRESSIVE_INPUT
    lines = height;
#else
    lines = (height/2);
#endif
    // Set output_w         
    setting = (HORZ_INFO_SPH_VAL << ISPCCDC_HORZ_INFO_SPH_SHIFT) | ((numBytesLine - 1)<< ISPCCDC_HORZ_INFO_NPH_SHIFT);
    ISP_OutReg32(&m_pCCDCRegs->CCDC_HORZ_INFO, setting);
    
    //vertical shift
    setting = ((VERT_START_VAL) << ISPCCDC_VERT_START_SLV0_SHIFT | (VERT_START_VAL) << ISPCCDC_VERT_START_SLV1_SHIFT);
    ISP_OutReg32(&m_pCCDCRegs->CCDC_VERT_START, setting);
         
    // Set output_h
    setting = (lines - 1) << ISPCCDC_VERT_LINES_NLV_SHIFT;
    ISP_OutReg32(&m_pCCDCRegs->CCDC_VERT_LINES, setting);


    ConfigOutlineOffset(numBytesLine, 0, 0);
#ifdef ENABLE_DEINTERLACED_OUTPUT    //There is no field pin connected to OMAP3 from tvp5416, so only for BT656.
    //de-interlace
    ConfigOutlineOffset(numBytesLine, EVENEVEN, 1);
    ConfigOutlineOffset(numBytesLine, ODDEVEN, 1);
    ConfigOutlineOffset(numBytesLine, EVENODD, 1);
    ConfigOutlineOffset(numBytesLine, ODDODD, 1);   
#endif //ENABLE_DEINTERLACED_OUTPUT
    return TRUE;        
}

//------------------------------------------------------------------------------
//
//  Function:  IsCCDCBusy
//
//  To check CCDC busy bit
//
BOOL CIspCtrl::IsCCDCBusy()
{
        DEBUGMSG(ZONE_FUNCTION, (TEXT("+IsCCDCBusy\r\n")));
                
        UINT32 setting = 0;
        
        ISP_InReg32(&m_pCCDCRegs->CCDC_PCR, &setting); 
        setting &= ISPCCDC_PCR_BUSY;

        if (setting)
            return TRUE;    
        else
            return FALSE;                   
}
//------------------------------------------------------------------------------
//
//  Function:  Check_IRQ0STATUS
//
//  Dump ISP_IRQ0STATUS register and check CCDC_VD0_IRQ bit
//
BOOL CIspCtrl::Check_IRQ0STATUS()
{
        DEBUGMSG(ZONE_FUNCTION, (TEXT("+IsCCDCBusy\r\n")));
                
        UINT32 setting = 0;
        UINT32 check_bit = 0;

        ISP_InReg32(&m_pIspConfigRegs->IRQ0STATUS, &setting);

        DEBUGMSG(ZONE_VERBOSE, (TEXT("+ISP_IRQ0STATUS:0x%x\r\n"),setting)); 
        check_bit = setting & IRQ0STATUS_CCDC_VD0_IRQ;

        if (check_bit)
        {
            //CCDCEnable(false);    
            return TRUE;
        }       
        else
            return FALSE;                   
}
        
        //------------------------------------------------------------------------------
//
//  Function:  InitializeCamera
//
//  To initialize Camera .
//
BOOL CIspCtrl::InitializeCamera()
{    
    DEBUGMSG(ZONE_FUNCTION, (TEXT("+InitializeCamera\r\n")));    
            
    if (ConfigGPIO4MDC() == FALSE)
    {   DEBUGMSG(ZONE_ERROR, (TEXT("GPIO Init. failed \r\n")));     
        return FALSE;
    }
    MapCameraReg(); //Map camera registers
        
    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  EnableCamera
//
//  To enable Camera.
//
BOOL CIspCtrl::EnableCamera(PCS_DATARANGE_VIDEO pCsDataRangeVideo)
{    
    DEBUGMSG(ZONE_FUNCTION, (TEXT("+EnableCamera\r\n")));

    if (m_bEnabled) {
        return TRUE;
    }

    ISPInit();// Init. ISP      
    ISPEnable(TRUE);// Soft reset ISP and clock on used module      
    

    PCS_VIDEOINFOHEADER pCsVideoInfoHdr = &(pCsDataRangeVideo->VideoInfoHeader);

    UINT biWidth        = pCsVideoInfoHdr->bmiHeader.biWidth;
    UINT biHeight       = abs(pCsVideoInfoHdr->bmiHeader.biHeight);
    UINT biSizeImage    = pCsVideoInfoHdr->bmiHeader.biSizeImage;
    UINT biBitCount     = pCsVideoInfoHdr->bmiHeader.biBitCount;

    if(!CCDCInit(pCsDataRangeVideo)) // Init. CCDC               
        return FALSE;

    if (!ISPConfigSize(biWidth, biHeight, biBitCount))// Init. video size 
        return FALSE;
        
    memset(m_pYUVVirtualAddr, 0, biSizeImage); //clear the frame buffer

    if (!CCDCEnable(TRUE))
        return FALSE;

    m_bEnabled = TRUE;

    return TRUE;

}

//------------------------------------------------------------------------------
//
//  Function:  DisableCamera
//
//  To disable Camera.
//
BOOL CIspCtrl::DisableCamera()
{
    DEBUGMSG(ZONE_FUNCTION, (TEXT("+DisableCamera\r\n")));

    if (!m_bEnabled) {
        return TRUE;
    }
    
    CCDCEnable(FALSE);
    ISPEnable(FALSE);
    
    // Free camera buffer
    DeAllocBuffer();

    m_bEnabled = FALSE;

    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  ChangeFrameBuffer
//
//  To Change the frame buffer address to CCDC_SDR.
//
BOOL CIspCtrl::ChangeFrameBuffer(ULONG ulVirtAddr)
{
    DEBUGMSG(ZONE_FUNCTION, (TEXT("+ChangeFrameBuffer\r\n")));  

    m_pYUVVirtualAddr= (LPVOID) ulVirtAddr;
    m_pYUVPhysicalAddr = GetPhysFromVirt((ULONG)m_pYUVVirtualAddr);
    if(!m_pYUVPhysicalAddr)
        return FALSE;   
    
    //Set CCDC_SDR address
    CCDCSetOutputAddress((ULONG) m_pYUVPhysicalAddr);
    
    return TRUE;
}
