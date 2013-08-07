

#include <pxa_nand.h>
#include <monahans.h>
#include <pxa_dma_engine.h>
#include <ipm_api.h>
#include <flash_cfg.h>
#include "args.h"
#include "ioctl_cfg.h"
#include "MFP_DRV.h"

void FmdUtilSetMFP(void)
{
  MFP_SetActiveMode(PXA_COMPONENT_DATA_FLASH_ID);
}

INT32 FmdUtilDmaInit(PXA_DFC_NAND_CONTEXT *pContext)
{
    return DFC_API_SUCCESS;
}

INT32 FmdUtilDmaDeinit(PXA_DFC_NAND_CONTEXT *pContext)
{
    return DFC_API_SUCCESS;
}

INT32 FmdUtilIntrInit(PXA_DFC_NAND_CONTEXT *pContext)
{
    return DFC_API_SUCCESS;
}

void FmdUtilIntrDeinit(PXA_DFC_NAND_CONTEXT *pContext)
{
    
}

void FmdUtilInitDfcMode(PXA_DFC_NAND_MODE *pMode)
{
    pMode->enableDMA = 0;
    pMode->enableINTR=0;    
    pMode->enableECC = 1;
    pMode->enableSpareArea =1;
    pMode->chipSelect = 0;
    pMode->enableRelocTableUpdate = 0;
    pMode->enableClockGatingWA = 1;
    return;
}

void FmdUtilAddrMapping(UINT32 *pBase, UINT32 addr, ULONG size)
{
    PHYSICAL_ADDRESS ioPhysicalBase={addr,0};
    
    *pBase=(UINT32)MmMapIoSpace(ioPhysicalBase, size, FALSE);

}

void FmdUtilAddrUnMapping(UINT32 base, ULONG size)
{
     
}

void FmdUtilInitCS(CRITICAL_SECTION *pCS)
{
    
}

void FmdUtilDelCS(CRITICAL_SECTION *pCS)
{
    
}

void FmdUtilEnterCS(CRITICAL_SECTION *pCS)
{
    
}

void FmdUtilLeaveCS(CRITICAL_SECTION *pCS)
{
    
}


BOOL FmdUtilWriteProtect(PXA_DFC_NAND_CONTEXT *pContext, SECTOR_ADDR sector)
{
    return TRUE;
}

BOOL FmdUtilEraseProtect(PXA_DFC_NAND_CONTEXT *pContext, BLOCK_ID blockId)
{
    return TRUE;
}

INT32 FmdUtilWaitForInterrupt(PXA_DFC_NAND_CONTEXT *pContext)
{
    //We will never reach here
    return DFC_API_FAILURE;
}

INT32 FmdUtilWaitForDmaDone(PXA_DFC_NAND_CONTEXT *pContext) 
{
    //We will never reach here
    return DFC_API_FAILURE;
}

void FmdUtilInterruptDone(PXA_DFC_NAND_CONTEXT *pContext)
{
}
void FmdUtilClearEvent(HANDLE event)
{
   
}
void FmdUtilIpmSet(PXA_DFC_NAND_CONTEXT *pContext)
{
}
BOOL FmdUtilIpmInit(PXA_DFC_NAND_CONTEXT *pContext)
{
    return TRUE;
}