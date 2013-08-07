// All rights reserved ADENEO EMBEDDED 2010

#ifndef _SENSOR_CTRL_H
#define _SENSOR_CTRL_H

#pragma warning(push)
#pragma warning(disable : 4201 6244)
#include "ispreg.h"
#include <windows.h>
#include <Cs.h>
#include <Csmedia.h>
#pragma warning(pop)

#ifdef __cplusplus
extern "C" {
#endif

class CIspCtrl
{
public:
    
    CIspCtrl();
    ~CIspCtrl();
    
    BOOL InitializeCamera();
    BOOL EnableCamera(PCS_DATARANGE_VIDEO pCsDataRangeVideo);
    BOOL DisableCamera();
    CAM_ISP_CONFIG_REGS* GetIspCfgRegs() {return m_pIspConfigRegs ;}
    CAM_ISP_CCDC_REGS*  GetCCDCRegs() {return m_pCCDCRegs ;}
    LPVOID              GetFrameBuffer() {return m_pYUVVirtualAddr;}
    BOOL                ChangeFrameBuffer(ULONG ulVirtAddr);
    
private:
    BOOL                    m_bEnabled;
    CAM_ISP_CONFIG_REGS     *m_pIspConfigRegs;
    CAM_ISP_CCDC_REGS       *m_pCCDCRegs;
    LPVOID                  m_pYUVVirtualAddr;      //YUV virtual address
    LPVOID                  m_pYUVPhysicalAddr;     //YUV physical address
    LPVOID                  m_pYUVDMAAddr;          //YUV DMA address

    LPVOID GetPhysFromVirt(
        ULONG ulVirtAddr
    );
    
    BOOL MapCameraReg();
       
    BOOL ConfigGPIO4MDC();
    
    BOOL CCDCInitCFG(
        PCS_DATARANGE_VIDEO pCsDataRangeVideo);
    
    BOOL ConfigOutlineOffset(
        UINT32 offset, 
        UINT8 oddeven, 
        UINT8 numlines);
        
    BOOL CCDCSetOutputAddress(
        ULONG SDA_Address);
        
    BOOL CCDCEnable(
        BOOL bEnable);
    
    BOOL AllocBuffer();
        
    BOOL DeAllocBuffer();
        
    BOOL CCDCInit(
        PCS_DATARANGE_VIDEO pCsDataRangeVideo);
    
    BOOL ISPInit();
    
    BOOL ISPEnable(
        BOOL bEnable);

    BOOL ISPConfigSize(
        UINT width,
        UINT height,
        UINT bpp); // bits per pixel

    BOOL IsCCDCBusy();
    
    BOOL Check_IRQ0STATUS();
    
    BOOL CCDCInitSYNC();    
  
};

#ifdef __cplusplus
}
#endif

#endif
