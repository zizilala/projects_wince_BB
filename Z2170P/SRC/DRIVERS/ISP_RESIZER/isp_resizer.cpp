//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#pragma warning(push)
#pragma warning(disable : 4201)
#include <windows.h>
#include <ceddk.h>
#include <ceddkex.h>
#include <oal.h>
#include <oalex.h>
#pragma warning(pop)
#include <isp_resizer.h>
#include <ispRszLoc.h>

#include <omap3530.h>
#include <oal_clock.h>



#if defined (__cplusplus)
extern "C" {
#endif /* defined (__cplusplus) */


static UINT16 Rsz4TapModeHorzFilterTable[32] = {
  #include "rDrvRsz4TapHorzTable.txt"
};

static UINT16 Rsz4TapModeVertFilterTable[32] = {
  #include "rDrvRsz4TapVertTable.txt"
};

static UINT16 Rsz7TapModeHorzFilterTable[28] = {
  #include "rDrvRsz7TapHorzTable.txt"
};

static UINT16 Rsz7TapModeVertFilterTable[28] = {
  #include "rDrvRsz7TapVertTable.txt"
};

static RSZ_DEVICE_CONTEXT_S rszDev;


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
BOOL InitializeRSZHW(RSZ_DEVICE_CONTEXT_S * dev);
BOOL DeinitializeRSZHW(RSZ_DEVICE_CONTEXT_S * dev);
BOOL CalculateRSZHWParams(RSZParams_t *pRSZParams);
BOOL setADDR_RSZParams(RSZ_CONTEXT_T * rszContext,RSZParams_t *pSrcRSZParams);
BOOL ConfigureRSZHW(RSZ_CONTEXT_T * rszContext);
BOOL EnableRSZ(RSZ_CONTEXT_T * rszContext,BOOL bEnableFlag);
BOOL WaitRSZComplete(RSZ_CONTEXT_T * rszContext);


//------------------------------------------------------------------------------
//
//  InitializeRSZHW
//
//  Description: This function initializes the Resizer HW component
//
//

BOOL InitializeRSZHW(RSZ_DEVICE_CONTEXT_S * dev)
{
    DWORD i = 0;
    DWORD dwRSZCNTData = 0;
    PHYSICAL_ADDRESS pa;

    EnableDeviceClocks(OMAP_DEVICE_CAMERA, TRUE);

    /* Map ISP registers */
    pa.QuadPart = OMAP_CAMISP_REGS_PA;
    dev->m_pCAMISP = (OMAP_CAM_ISP_REGS *)MmMapIoSpace(pa, sizeof(OMAP_CAM_ISP_REGS), FALSE);

    if (dev->m_pCAMISP == NULL)
    {
        RETAILMSG(1, (L"ERROR: InitializeRSZHW: "
            L"Failed to map camera controller registers\r\n"
            ));
        goto clean;
    }

    /* Map resizer registers */
    pa.QuadPart = ISP_RSZ_BASE_ADDRESS;
    dev->m_pCAMRSZ =  (OMAP_ISP_RSZ_REGS *)MmMapIoSpace(pa, sizeof(OMAP_ISP_RSZ_REGS), FALSE);

    if (dev->m_pCAMRSZ == NULL)
    {
        RETAILMSG(1, (L"ERROR: InitializeRSZHW: "
            L"Failed to map Resizer Engine registers\r\n"
            ));
        goto clean;
    }

    /* Map the camera MMU regs */
    pa.QuadPart = CAM_MMU_BASE_ADDRESS;
    dev->m_pCamMMU = (OMAP_CAM_MMU_REGS *)MmMapIoSpace(pa, sizeof(OMAP_CAM_MMU_REGS), FALSE);
    if (dev->m_pCamMMU == NULL)
    {
        RETAILMSG(1,(L"ERROR: InitializeRSZHW:Unable to Map SCM address space\r\n"));
        goto clean;
    }

    /* bit 24 is for resizer */
    /* save old state */
    dev->prevState.irqEnable = (INREG32(&dev->m_pCAMISP->ISP_IRQ0ENABLE) & ISP_IRQ0ENABLE_RSZ_DONE_IRQ);
    CLRREG32(&dev->m_pCAMISP->ISP_IRQ0ENABLE,ISP_IRQ0ENABLE_RSZ_DONE_IRQ);

    /* disable MMU */
    dev->prevState.mmuEnable = (INREG32(&dev->m_pCamMMU->MMU_CNTL) & CAMMMU_CNTL_MMU_EN);
    CLRREG32(&dev->m_pCamMMU->MMU_CNTL, CAMMMU_CNTL_MMU_EN);

    /* enable resizer clock and SBL */
    dev->prevState.ispCtrl = INREG32(&dev->m_pCAMISP->ISP_CTRL);
    SETREG32(&dev->m_pCAMISP->ISP_CTRL,( ISP_CTRL_RSZ_CLK_EN
                                    |ISP_CTRL_SBL_RD_RAM_EN
                                    |ISP_CTRL_SBL_WR0_RAM_EN));

    dwRSZCNTData &= ~RSZ_CNT_INPTYP;
    dwRSZCNTData &= ~RSZ_CNT_INPSRC;
    dwRSZCNTData &= ~RSZ_CNT_YCPOS;
    dwRSZCNTData &= ~RSZ_CNT_CBILIN;

    OUTREG32(&dev->m_pCAMRSZ->RSZ_CNT, dwRSZCNTData);
    
    /* Program the filter coeffs later based on resizer ratios */
    OUTREG32(&dev->m_pCAMRSZ->RSZ_YENH, i);
    return TRUE;

clean:
    EnableDeviceClocks(OMAP_DEVICE_CAMERA, FALSE);
    return FALSE;
}



//------------------------------------------------------------------------------
//
//  InitializeRSZHW
//
//  Description: This function initializes the Resizer HW component
//
//

BOOL DeinitializeRSZHW(RSZ_DEVICE_CONTEXT_S * dev)
{
    /* restore old state */
    OUTREG32(&dev->m_pCAMISP->ISP_CTRL,dev->prevState.ispCtrl);
    if (dev->prevState.mmuEnable)
        SETREG32(&dev->m_pCamMMU->MMU_CNTL, CAMMMU_CNTL_MMU_EN);
    else
        CLRREG32(&dev->m_pCamMMU->MMU_CNTL, CAMMMU_CNTL_MMU_EN);
    if (dev->prevState.irqEnable)
        SETREG32(&dev->m_pCAMISP->ISP_IRQ0ENABLE,ISP_IRQ0ENABLE_RSZ_DONE_IRQ);
    else
        CLRREG32(&dev->m_pCAMISP->ISP_IRQ0ENABLE,ISP_IRQ0ENABLE_RSZ_DONE_IRQ);


    /* unmap registers */
    MmUnmapIoSpace((PVOID)dev->m_pCamMMU,sizeof(OMAP_CAM_MMU_REGS)); 
    MmUnmapIoSpace((PVOID)dev->m_pCAMISP, sizeof(OMAP_CAM_ISP_REGS));
    MmUnmapIoSpace((PVOID)dev->m_pCAMRSZ, sizeof(OMAP_ISP_RSZ_REGS));

    EnableDeviceClocks(OMAP_DEVICE_CAMERA, FALSE);    
    
    return TRUE;
}



//------------------------------------------------------------------------------
//
//  CalculateRSZHWParams
//  Description: This function calculates the RSZ HW params. It does not configure the
//               hardware.
//
BOOL CalculateRSZHWParams(RSZParams_t *pRSZParams)
{
    ULONG rsz, rsz_7, rsz_4;
    ULONG sph;
    ULONG input_w, input_h;
    ULONG output_w, output_h;
    ULONG output;
    ULONG max_in_otf, max_out_7tap;
   

    if(pRSZParams->enableZoom)
    {
        //Recycling variables

        //width [bytes]; 32-byte aligned.
        input_w = pRSZParams->width << 1; //YUV422: 2 bytes per pixel
        input_w /= RSZ_BYTE_ALIGN;
        input_w *= RSZ_BYTE_ALIGN;

        //crop width [bytes]; 32-byte aligned.
        output_w = pRSZParams->cropWidth << 1;
        output_w /= RSZ_BYTE_ALIGN;
        output_w *= RSZ_BYTE_ALIGN;
        
        //height [lines]; forced to be even.
        input_h = pRSZParams->height - (pRSZParams->height % 2);

        //crop height [lines]; forced to be even.
        output_h = pRSZParams->cropHeight - (pRSZParams->cropHeight % 2);
        
        RETAILMSG(1, (L"OMAP35xxCameraCtrl::CalculateRSZHWParams input w: %d.\r\n", input_w));
        RETAILMSG(1, (L"OMAP35xxCameraCtrl::CalculateRSZHWParams  crop w: %d.\r\n", output_w));
        RETAILMSG(1, (L"OMAP35xxCameraCtrl::CalculateRSZHWParams input h: %d.\r\n", input_h));
        RETAILMSG(1, (L"OMAP35xxCameraCtrl::CalculateRSZHWParams  crop h: %d.\r\n", output_h));
        
        //Middle
        pRSZParams->ulReadAddrOffset = (input_w * input_h) >> 1; //32-byte aligned
        //Top
        pRSZParams->ulReadAddrOffset -= (input_w * output_h) >> 1; //32-byte aligned
        //Top-Left
        pRSZParams->ulReadAddrOffset += (input_w - output_w) >> 1; //may not be 32-byte aligned

        RETAILMSG(1, (L"OMAP35xxCameraCtrl::CalculateRSZHWParams Read Addr Offset remainder: 0x%x.\r\n", (pRSZParams->ulReadAddrOffset % RSZ_BYTE_ALIGN)));
    }
    else
    {
        pRSZParams->ulReadAddrOffset = 0;
    }


    if(pRSZParams->enableZoom)
    {
        input_w = pRSZParams->cropWidth;
        input_h = pRSZParams->cropHeight;
    }
    else
    {
        input_w = pRSZParams->width;
        input_h = pRSZParams->height;
    }

    output_w = pRSZParams->ulOutputImageWidth;
    output_h = pRSZParams->ulOutputImageHeight;

    /*
     * Step 1: Recalculate input/output requirements based on TRM equations
     * Step 2: Programs hardware.
     *
     */


    /* STEP 1*/

    /*
     * We need to ensure input of resizer after size calculations does not
     * exceed the actual input.
     */

    input_w = input_w - 6;
    input_h = input_h - 6;

    if (input_h > RSZ_MAX_IN_HEIGHT)
    {
        RETAILMSG(1,(L"ERROR: Height (%d) exceeds maximum supported by RSZ\r\n", input_h));
        return FALSE;
    }

    max_in_otf = RSZ_MAX_IN_WIDTH_ONTHEFLY_MODE_ES2;
    max_out_7tap = RSZ_MAX_7TAP_VRSZ_OUTWIDTH_ES2;

    if (input_w > max_in_otf)
    {
        RETAILMSG(1,(L"ERROR: Width exceeds maximum supported by RSZ\r\n"));
        return FALSE;
    }

    sph = RSZ_DEFAULTSTPHASE;

    output = output_h;

    /* Calculate height */
    rsz_7 = ((input_h - 7) * 256) / (output - 1);
    rsz_4 = ((input_h - 4) * 256) / (output - 1);

    rsz = (input_h * 256) / output;

    if (rsz <= RSZ_MID_RESIZE_VALUE)
    {
        rsz = rsz_4;
        if (rsz < RSZ_MINIMUM_RESIZE_VALUE) {
            rsz = RSZ_MINIMUM_RESIZE_VALUE;
            output = (((input_h - 4) * 256) / rsz) + 1;
            }
    }
    else
    {
        rsz = rsz_7;
        if (output_w > max_out_7tap)
            output_w = max_out_7tap;
        if (rsz > RSZ_MAXIMUM_RESIZE_VALUE)
        {
            rsz = RSZ_MAXIMUM_RESIZE_VALUE;
            output = (((input_h - 7) * 256) / rsz) + 1;
        }
    }

    /* Recalculate input */
    if (rsz > RSZ_MID_RESIZE_VALUE)
        input_h = (((64 * sph) + ((output - 1) * rsz) + 32) / 256) + 7;
    else
        input_h = (((32 * sph) + ((output - 1) * rsz) + 16) / 256) + 4;

    if(pRSZParams->enableZoom)
        pRSZParams->cropHeight = input_h;
    else
        pRSZParams->ulInputImageHeight = input_h;

    pRSZParams->v_resz = rsz;
    pRSZParams->ulOutputImageHeight = output;
    pRSZParams->cropTop = RSZ_DEFAULTSTPIXEL;
    pRSZParams->v_startphase = sph;

    /* Calculate Width */
    output = output_w;
    sph = RSZ_DEFAULTSTPHASE;

    rsz_7 = ((input_w - 7) * 256) / (output - 1);
    rsz_4 = ((input_w - 4) * 256) / (output - 1);

    rsz = (input_w * 256) / output;
    if (rsz > RSZ_MID_RESIZE_VALUE)
    {
        rsz = rsz_7;
        if (rsz > RSZ_MAXIMUM_RESIZE_VALUE)
        {
            rsz = RSZ_MAXIMUM_RESIZE_VALUE;
            output = (((input_w - 7) * 256) / rsz) + 1;
            RETAILMSG(1,(L"ERROR: Width exceeds max. Should be limited to %d\r\n", output));
        }
    }
    else
    {
        rsz = rsz_4;
        if (rsz < RSZ_MINIMUM_RESIZE_VALUE)
        {
            rsz = RSZ_MINIMUM_RESIZE_VALUE;
            output = (((input_w - 4) * 256) / rsz) + 1;
            RETAILMSG(1,(L"ERROR: Width less than min. Should be atleast %d\r\n", output));
        }
    }

        /* Recalculate input based on TRM equations */
    if (rsz > RSZ_MID_RESIZE_VALUE)
        input_w = (((64 * sph) + ((output - 1) * rsz) + 32) / 256) + 7;
    else
        input_w = (((32 * sph) + ((output - 1) * rsz) + 16) / 256) + 7;


    pRSZParams->ulOutputImageWidth = output;
    pRSZParams->h_resz = rsz;
    if(pRSZParams->enableZoom)
        pRSZParams->cropWidth = input_w;
    else
        pRSZParams->ulInputImageWidth = input_w;

    pRSZParams->cropLeft = RSZ_DEFAULTSTPIXEL;
    pRSZParams->h_startphase = sph;

    //pRSZParams->ulOutOffset = (pRSZParams->ulOutputImageWidth *2);    
    //pRSZParams->ulOutOffset = ((pRSZParams->ulOutOffset >> 5) + 1)<<5;
        
    return TRUE;

}

BOOL SetRSZParams(
    RSZParams_t *pInRSZParams,    
    RSZ_CONTEXT_T * rszContext    
)
{
    int mul = 1;
    RSZParams_t *pOutRSZParams = &rszContext->params;
    rszContext->state = RSZ_UNINIT; /* to detect errors */

    if ((pInRSZParams->width <= 0) || (pInRSZParams->height <= 0) ||
		(pInRSZParams->ulOutputImageWidth <= 0) || (pInRSZParams->ulOutputImageHeight <= 0) ||
		(pInRSZParams->ulReadOffset <= 0) || (pInRSZParams->ulOutOffset<= 0)) {
		    RETAILMSG(1,(L"ERROR: SetRSZParams: params set 0\r\n"));
		    return FALSE;
        }
    if ((pInRSZParams->ulInpType!= RSZ_INTYPE_YCBCR422_16BIT) &&
			(pInRSZParams->ulInpType != RSZ_INTYPE_PLANAR_8BIT)) {
		RETAILMSG(TRUE, (L"ERROR: SetRSZParams: Invalid input type\n"));
		return FALSE;
	}
	if ((pInRSZParams->ulPixFmt != RSZ_PIX_FMT_UYVY) &&
			(pInRSZParams->ulPixFmt!= RSZ_PIX_FMT_YUYV)) {
		RETAILMSG(TRUE, (L"ERROR: SetRSZParams: Invalid pixel format\n"));
		return FALSE;
	}
	if (pInRSZParams->ulInpType == RSZ_INTYPE_YCBCR422_16BIT)
		mul = 2;
	else
		mul = 1;
    
	if (pInRSZParams->ulReadOffset < (pInRSZParams->width * mul)) {
		RETAILMSG(TRUE, (L"ERROR: SetRSZParams: Pitch is incorrect\n"));
		return FALSE;
	}
	if (pInRSZParams->ulOutOffset < (pInRSZParams->ulOutputImageWidth * mul)) {
		RETAILMSG(TRUE, (L"ERROR: SetRSZParams: Out pitch cannot be less than out hsize\n"));
		return FALSE;
	}
	/* Output H size should be even */
	if ((pInRSZParams->ulOutputImageWidth % PIXEL_EVEN) != 0) {
		RETAILMSG(TRUE, (L"ERROR: SetRSZParams: Output H size should be even\n"));
		return FALSE;
	}
	if (pInRSZParams->cropLeft < 0) {
		RETAILMSG(TRUE, (L"ERROR: SetRSZParams: Horz start pixel cannot be less than zero\n"));
		return FALSE;
    }
    if ((pInRSZParams->ulReadOffset%ALIGN)!=0)
    {   
        RETAILMSG(TRUE, (L"ERROR: SetRSZParams: ReadOffset should be 32-byte aligned\n"));
        return FALSE;
    }

    if ((pInRSZParams->ulOutOffset%ALIGN)!=0)
    {
        RETAILMSG(TRUE, (L"ERROR: SetRSZParams: ulOutOffset should be 32-byte aligned\n"));
        return FALSE;
    }


    /* calculate */
    memcpy(pOutRSZParams,pInRSZParams,sizeof(RSZParams_t));
    if (!CalculateRSZHWParams(pOutRSZParams))
    {
        RETAILMSG(TRUE, (L"ERROR: SetRSZParams: CalculateRSZHWParams returned FALSE\r\n"));
        return FALSE;
    }
            

    if (pOutRSZParams->ulInpType == RSZ_INTYPE_PLANAR_8BIT) {
		if (pOutRSZParams->cropLeft > MAX_HORZ_PIXEL_8BIT) {
            RETAILMSG(TRUE, (L"ERROR: SetRSZParams: cropLeft (%d) is greater than %d \r\n",pOutRSZParams->cropLeft, MAX_HORZ_PIXEL_8BIT));
			return FALSE;
        }
	}
	if (pOutRSZParams->ulInpType == RSZ_INTYPE_YCBCR422_16BIT) {
		if (pOutRSZParams->cropLeft > MAX_HORZ_PIXEL_16BIT) {
            RETAILMSG(TRUE, (L"ERROR: SetRSZParams: cropLeft (%d) is greater than %d \r\n",pOutRSZParams->cropLeft, MAX_HORZ_PIXEL_16BIT));			
			return FALSE;
	    }    
    }
    
    rszContext->state = RSZ_INIT;
    return TRUE;
}

BOOL setADDR_RSZParams(
    RSZ_CONTEXT_T * rszContext,
    RSZParams_t *pSrcRSZParams
)
{
    RSZParams_t *pRSZParams = &rszContext->params;
    //rszContext->state = RSZ_UNINIT;    
    if (pSrcRSZParams->ulReadAddr%ALIGN != 0)
    {
        RETAILMSG(TRUE, (L"ERROR: setADDR_RSZParams: srcAddr 0x%x should be 32-byte aligned\r\n",pSrcRSZParams->ulReadAddr));
        return FALSE;
    }
    if (pSrcRSZParams->ulWriteAddr%ALIGN != 0)
    {
        RETAILMSG(TRUE, (L"ERROR: setADDR_RSZParams: destAddr 0x%x should be 32-byte aligned\r\n",pSrcRSZParams->ulWriteAddr));
        return FALSE;
    }
    if (pSrcRSZParams->cropLeft < 0) {
		RETAILMSG(TRUE, (L"ERROR: setADDR_RSZParams: Horz start pixel cannot be less than zero (%d)\n",pSrcRSZParams->cropLeft));
		return FALSE;
    }
    if ((pRSZParams->ulInpType == RSZ_INTYPE_PLANAR_8BIT) && (pSrcRSZParams->cropLeft > MAX_HORZ_PIXEL_8BIT)) {
        RETAILMSG(TRUE, (L"ERROR: setADDR_RSZParams: Horz start pixel cannot be more than %d\n",MAX_HORZ_PIXEL_8BIT));
		return FALSE;
	}
	if ((pRSZParams->ulInpType == RSZ_INTYPE_YCBCR422_16BIT) && (pSrcRSZParams->cropLeft > MAX_HORZ_PIXEL_16BIT))
    {
		RETAILMSG(TRUE, (L"ERROR: setADDR_RSZParams: Horz start pixel cannot be more than %d\n",MAX_HORZ_PIXEL_8BIT));
		return FALSE;
	}
    
    pRSZParams->ulReadAddr = pSrcRSZParams->ulReadAddr;
    pRSZParams->ulWriteAddr = pSrcRSZParams->ulWriteAddr;
    pRSZParams->cropLeft= pSrcRSZParams->cropLeft;

    rszContext->state = RSZ_INIT;

	return TRUE;
}
    


//------------------------------------------------------------------------------
//
//  ConfigureRSZHW
//  Description: This function configures the CCDC HW with the VideoInfo header
//               information passed to camera driver
//
//
BOOL
ConfigureRSZHW(
    RSZ_CONTEXT_T * rszContext    
)
{
    ULONG rszCnt = 0;
    RSZ_DEVICE_CONTEXT_S * dev = rszContext->dev;
    RSZParams_t *pRSZParams = &rszContext->params;

    if (rszContext->state != RSZ_INIT)
    {
        RETAILMSG(TRUE, (L"ERROR: ConfigureRSZHW: state(%d) is wrong \r\n",rszContext->state));
        return FALSE;
    }

    if (dev->inUseContext != 0)
    {
        RETAILMSG(TRUE, (L"ERROR: ConfigureRSZHW: inUseContext(0x%x) is not zero \r\n",dev->inUseContext));
        return FALSE;
    }

    /* check for addresses to be not null */
    if ((pRSZParams->ulReadAddr == NULL) || (pRSZParams->ulWriteAddr==NULL))
    {
        RETAILMSG(TRUE, (L"ERROR: ConfigureRSZHW: ulReadAddr or ulWriteAddr are zero \r\n"));
        return FALSE;
    }


    dev->inUseContext = (DWORD)rszContext;    

    /* RSZ_CNT */
    rszCnt = RSZ_CNT_VSTPH(pRSZParams->v_startphase)|
            RSZ_CNT_HSTPH(pRSZParams->h_startphase)|
            RSZ_CNT_VRSZ((pRSZParams->v_resz - 1))|
            RSZ_CNT_HRSZ((pRSZParams->h_resz - 1));
    
    rszCnt |= RSZ_CNT_INPSRC; /* always Memory input */
    
    if (pRSZParams->ulInpType == RSZ_INTYPE_PLANAR_8BIT)
        rszCnt |= RSZ_CNT_INPTYP;
    else { /* YUV */
        if (pRSZParams->ulPixFmt == RSZ_PIX_FMT_YUYV)
            rszCnt |= RSZ_CNT_YCPOS;
    }    
    OUTREG32(&dev->m_pCAMRSZ->RSZ_CNT, rszCnt);
    
    OUTREG32(&dev->m_pCAMRSZ->RSZ_SDR_INOFF, pRSZParams->ulReadOffset);    
    OUTREG32(&dev->m_pCAMRSZ->RSZ_IN_START, RSZ_IN_START_VERT_ST(pRSZParams->cropTop)|
                                       RSZ_IN_START_HORZ_ST(pRSZParams->cropLeft));

    if(pRSZParams->enableZoom)
    {
        OUTREG32(&dev->m_pCAMRSZ->RSZ_IN_SIZE, RSZ_IN_SIZE_VERT(pRSZParams->cropHeight)|
                                      RSZ_IN_SIZE_HORZ(pRSZParams->cropWidth));
    }
    else
    {
        OUTREG32(&dev->m_pCAMRSZ->RSZ_IN_SIZE, RSZ_IN_SIZE_VERT(pRSZParams->ulInputImageHeight)|
                                      RSZ_IN_SIZE_HORZ(pRSZParams->ulInputImageWidth));
    }
    OUTREG32(&dev->m_pCAMRSZ->RSZ_OUT_SIZE, RSZ_OUT_SIZE_VERT(pRSZParams->ulOutputImageHeight)|
                                       RSZ_OUT_SIZE_HORZ(pRSZParams->ulOutputImageWidth));
    // Set the output line offset
    OUTREG32(&dev->m_pCAMRSZ->RSZ_SDR_OUTOFF, (pRSZParams->ulOutOffset << RSZ_SDR_OUTOFF_OFFSET_SHIFT));
    // Program the Horz filter coeffs
    if(pRSZParams->h_resz <= RSZ_MID_RESIZE_VALUE)
        {
            OUTREG32((&dev->m_pCAMRSZ->RSZ_HFILT10), ((Rsz4TapModeHorzFilterTable[0] << RSZ_HFILT_COEFL_SHIFT)
                | (Rsz4TapModeHorzFilterTable[1] << RSZ_HFILT_COEFH_SHIFT)));
            OUTREG32((&dev->m_pCAMRSZ->RSZ_HFILT32), ((Rsz4TapModeHorzFilterTable[2] << RSZ_HFILT_COEFL_SHIFT)
                | (Rsz4TapModeHorzFilterTable[3] << RSZ_HFILT_COEFH_SHIFT)));
            OUTREG32((&dev->m_pCAMRSZ->RSZ_HFILT54), ((Rsz4TapModeHorzFilterTable[4] << RSZ_HFILT_COEFL_SHIFT)
                | (Rsz4TapModeHorzFilterTable[5] << RSZ_HFILT_COEFH_SHIFT)));
            OUTREG32((&dev->m_pCAMRSZ->RSZ_HFILT76), ((Rsz4TapModeHorzFilterTable[6] << RSZ_HFILT_COEFL_SHIFT)
                | (Rsz4TapModeHorzFilterTable[7] << RSZ_HFILT_COEFH_SHIFT)));
            OUTREG32((&dev->m_pCAMRSZ->RSZ_HFILT98), ((Rsz4TapModeHorzFilterTable[8] << RSZ_HFILT_COEFL_SHIFT)
                | (Rsz4TapModeHorzFilterTable[9] << RSZ_HFILT_COEFH_SHIFT)));
            OUTREG32((&dev->m_pCAMRSZ->RSZ_HFILT1110), ((Rsz4TapModeHorzFilterTable[10] << RSZ_HFILT_COEFL_SHIFT)
                | (Rsz4TapModeHorzFilterTable[11] << RSZ_HFILT_COEFH_SHIFT)));
            OUTREG32((&dev->m_pCAMRSZ->RSZ_HFILT1312), ((Rsz4TapModeHorzFilterTable[12] << RSZ_HFILT_COEFL_SHIFT)
                | (Rsz4TapModeHorzFilterTable[13] << RSZ_HFILT_COEFH_SHIFT)));
            OUTREG32((&dev->m_pCAMRSZ->RSZ_HFILT1514), ((Rsz4TapModeHorzFilterTable[14] << RSZ_HFILT_COEFL_SHIFT)
                | (Rsz4TapModeHorzFilterTable[15] << RSZ_HFILT_COEFH_SHIFT)));
            OUTREG32((&dev->m_pCAMRSZ->RSZ_HFILT1716), ((Rsz4TapModeHorzFilterTable[16] << RSZ_HFILT_COEFL_SHIFT)
                | (Rsz4TapModeHorzFilterTable[17] << RSZ_HFILT_COEFH_SHIFT)));
            OUTREG32((&dev->m_pCAMRSZ->RSZ_HFILT1918), ((Rsz4TapModeHorzFilterTable[18] << RSZ_HFILT_COEFL_SHIFT)
                | (Rsz4TapModeHorzFilterTable[19] << RSZ_HFILT_COEFH_SHIFT)));
            OUTREG32((&dev->m_pCAMRSZ->RSZ_HFILT2120), ((Rsz4TapModeHorzFilterTable[20] << RSZ_HFILT_COEFL_SHIFT)
                | (Rsz4TapModeHorzFilterTable[21] << RSZ_HFILT_COEFH_SHIFT)));
            OUTREG32((&dev->m_pCAMRSZ->RSZ_HFILT2322), ((Rsz4TapModeHorzFilterTable[22] << RSZ_HFILT_COEFL_SHIFT)
                | (Rsz4TapModeHorzFilterTable[23] << RSZ_HFILT_COEFH_SHIFT)));
            OUTREG32((&dev->m_pCAMRSZ->RSZ_HFILT2524), ((Rsz4TapModeHorzFilterTable[24] << RSZ_HFILT_COEFL_SHIFT)
                | (Rsz4TapModeHorzFilterTable[25] << RSZ_HFILT_COEFH_SHIFT)));
            OUTREG32((&dev->m_pCAMRSZ->RSZ_HFILT2726), ((Rsz4TapModeHorzFilterTable[26] << RSZ_HFILT_COEFL_SHIFT)
                | (Rsz4TapModeHorzFilterTable[27] << RSZ_HFILT_COEFH_SHIFT)));
            OUTREG32((&dev->m_pCAMRSZ->RSZ_HFILT2928), ((Rsz4TapModeHorzFilterTable[28] << RSZ_HFILT_COEFL_SHIFT)
                | (Rsz4TapModeHorzFilterTable[29] << RSZ_HFILT_COEFH_SHIFT)));
            OUTREG32((&dev->m_pCAMRSZ->RSZ_HFILT3130), ((Rsz4TapModeHorzFilterTable[30] << RSZ_HFILT_COEFL_SHIFT)
                | (Rsz4TapModeHorzFilterTable[31] << RSZ_HFILT_COEFH_SHIFT)));
        }
    else
        {

            OUTREG32((&dev->m_pCAMRSZ->RSZ_HFILT10), ((Rsz7TapModeHorzFilterTable[0] << RSZ_HFILT_COEFL_SHIFT)
                | (Rsz7TapModeHorzFilterTable[1] << RSZ_HFILT_COEFH_SHIFT)));
            OUTREG32((&dev->m_pCAMRSZ->RSZ_HFILT32), ((Rsz7TapModeHorzFilterTable[2] << RSZ_HFILT_COEFL_SHIFT)
                | (Rsz7TapModeHorzFilterTable[3] << RSZ_HFILT_COEFH_SHIFT)));
            OUTREG32((&dev->m_pCAMRSZ->RSZ_HFILT54), ((Rsz7TapModeHorzFilterTable[4] << RSZ_HFILT_COEFL_SHIFT)
                | (Rsz7TapModeHorzFilterTable[5] << RSZ_HFILT_COEFH_SHIFT)));
            OUTREG32((&dev->m_pCAMRSZ->RSZ_HFILT76), ((Rsz7TapModeHorzFilterTable[6] << RSZ_HFILT_COEFL_SHIFT)));

            OUTREG32((&dev->m_pCAMRSZ->RSZ_HFILT98), ((Rsz7TapModeHorzFilterTable[7] << RSZ_HFILT_COEFL_SHIFT)
                | (Rsz7TapModeHorzFilterTable[8] << RSZ_HFILT_COEFH_SHIFT)));
            OUTREG32((&dev->m_pCAMRSZ->RSZ_HFILT1110), ((Rsz7TapModeHorzFilterTable[9] << RSZ_HFILT_COEFL_SHIFT)
                | (Rsz7TapModeHorzFilterTable[10] << RSZ_HFILT_COEFH_SHIFT)));
            OUTREG32((&dev->m_pCAMRSZ->RSZ_HFILT1312), ((Rsz7TapModeHorzFilterTable[11] << RSZ_HFILT_COEFL_SHIFT)
                | (Rsz7TapModeHorzFilterTable[12] << RSZ_HFILT_COEFH_SHIFT)));
            OUTREG32((&dev->m_pCAMRSZ->RSZ_HFILT1514), ((Rsz7TapModeHorzFilterTable[13] << RSZ_HFILT_COEFL_SHIFT)));


            OUTREG32((&dev->m_pCAMRSZ->RSZ_HFILT1716), ((Rsz7TapModeHorzFilterTable[14] << RSZ_HFILT_COEFL_SHIFT)
                | (Rsz7TapModeHorzFilterTable[15] << RSZ_HFILT_COEFH_SHIFT)));
            OUTREG32((&dev->m_pCAMRSZ->RSZ_HFILT1918), ((Rsz7TapModeHorzFilterTable[16] << RSZ_HFILT_COEFL_SHIFT)
                | (Rsz7TapModeHorzFilterTable[17] << RSZ_HFILT_COEFH_SHIFT)));
            OUTREG32((&dev->m_pCAMRSZ->RSZ_HFILT2120), ((Rsz7TapModeHorzFilterTable[18] << RSZ_HFILT_COEFL_SHIFT)
                | (Rsz7TapModeHorzFilterTable[19] << RSZ_HFILT_COEFH_SHIFT)));
            OUTREG32((&dev->m_pCAMRSZ->RSZ_HFILT2322), ((Rsz7TapModeHorzFilterTable[20] << RSZ_HFILT_COEFL_SHIFT)));

            OUTREG32((&dev->m_pCAMRSZ->RSZ_HFILT2524), ((Rsz7TapModeHorzFilterTable[21] << RSZ_HFILT_COEFL_SHIFT)
                | (Rsz7TapModeHorzFilterTable[22] << RSZ_HFILT_COEFH_SHIFT)));
            OUTREG32((&dev->m_pCAMRSZ->RSZ_HFILT2726), ((Rsz7TapModeHorzFilterTable[23] << RSZ_HFILT_COEFL_SHIFT)
                | (Rsz7TapModeHorzFilterTable[24] << RSZ_HFILT_COEFH_SHIFT)));
            OUTREG32((&dev->m_pCAMRSZ->RSZ_HFILT2928), ((Rsz7TapModeHorzFilterTable[25] << RSZ_HFILT_COEFL_SHIFT)
                | (Rsz7TapModeHorzFilterTable[26] << RSZ_HFILT_COEFH_SHIFT)));
            OUTREG32((&dev->m_pCAMRSZ->RSZ_HFILT3130), ((Rsz7TapModeHorzFilterTable[27] << RSZ_HFILT_COEFL_SHIFT)));
        }

    // Program the Vert filter coeffs
    if(pRSZParams->v_resz <= RSZ_MID_RESIZE_VALUE)
        {
            OUTREG32((&dev->m_pCAMRSZ->RSZ_VFILT10), ((Rsz4TapModeVertFilterTable[0] << RSZ_VFILT_COEFL_SHIFT)
                | (Rsz4TapModeVertFilterTable[1] << RSZ_VFILT_COEFH_SHIFT)));
            OUTREG32((&dev->m_pCAMRSZ->RSZ_VFILT32), ((Rsz4TapModeVertFilterTable[2] << RSZ_VFILT_COEFL_SHIFT)
                | (Rsz4TapModeVertFilterTable[3] << RSZ_VFILT_COEFH_SHIFT)));
            OUTREG32((&dev->m_pCAMRSZ->RSZ_VFILT54), ((Rsz4TapModeVertFilterTable[4] << RSZ_VFILT_COEFL_SHIFT)
                | (Rsz4TapModeVertFilterTable[5] << RSZ_VFILT_COEFH_SHIFT)));
            OUTREG32((&dev->m_pCAMRSZ->RSZ_VFILT76), ((Rsz4TapModeVertFilterTable[6] << RSZ_VFILT_COEFL_SHIFT)
                | (Rsz4TapModeVertFilterTable[7] << RSZ_VFILT_COEFH_SHIFT)));
            OUTREG32((&dev->m_pCAMRSZ->RSZ_VFILT98), ((Rsz4TapModeVertFilterTable[8] << RSZ_VFILT_COEFL_SHIFT)
                | (Rsz4TapModeVertFilterTable[9] << RSZ_VFILT_COEFH_SHIFT)));
            OUTREG32((&dev->m_pCAMRSZ->RSZ_VFILT1110), ((Rsz4TapModeVertFilterTable[10] << RSZ_VFILT_COEFL_SHIFT)
                | (Rsz4TapModeVertFilterTable[11] << RSZ_VFILT_COEFH_SHIFT)));
            OUTREG32((&dev->m_pCAMRSZ->RSZ_VFILT1312), ((Rsz4TapModeVertFilterTable[12] << RSZ_VFILT_COEFL_SHIFT)
                | (Rsz4TapModeVertFilterTable[13] << RSZ_VFILT_COEFH_SHIFT)));
            OUTREG32((&dev->m_pCAMRSZ->RSZ_VFILT1514), ((Rsz4TapModeVertFilterTable[14] << RSZ_VFILT_COEFL_SHIFT)
                | (Rsz4TapModeVertFilterTable[15] << RSZ_VFILT_COEFH_SHIFT)));
            OUTREG32((&dev->m_pCAMRSZ->RSZ_VFILT1716), ((Rsz4TapModeVertFilterTable[16] << RSZ_VFILT_COEFL_SHIFT)
                | (Rsz4TapModeVertFilterTable[17] << RSZ_VFILT_COEFH_SHIFT)));
            OUTREG32((&dev->m_pCAMRSZ->RSZ_VFILT1918), ((Rsz4TapModeVertFilterTable[18] << RSZ_VFILT_COEFL_SHIFT)
                | (Rsz4TapModeVertFilterTable[19] << RSZ_VFILT_COEFH_SHIFT)));
            OUTREG32((&dev->m_pCAMRSZ->RSZ_VFILT2120), ((Rsz4TapModeVertFilterTable[20] << RSZ_VFILT_COEFL_SHIFT)
                | (Rsz4TapModeVertFilterTable[21] << RSZ_VFILT_COEFH_SHIFT)));
            OUTREG32((&dev->m_pCAMRSZ->RSZ_VFILT2322), ((Rsz4TapModeVertFilterTable[22] << RSZ_VFILT_COEFL_SHIFT)
                | (Rsz4TapModeVertFilterTable[23] << RSZ_VFILT_COEFH_SHIFT)));
            OUTREG32((&dev->m_pCAMRSZ->RSZ_VFILT2524), ((Rsz4TapModeVertFilterTable[24] << RSZ_VFILT_COEFL_SHIFT)
                | (Rsz4TapModeVertFilterTable[25] << RSZ_VFILT_COEFH_SHIFT)));
            OUTREG32((&dev->m_pCAMRSZ->RSZ_VFILT2726), ((Rsz4TapModeVertFilterTable[26] << RSZ_VFILT_COEFL_SHIFT)
                | (Rsz4TapModeVertFilterTable[27] << RSZ_VFILT_COEFH_SHIFT)));
            OUTREG32((&dev->m_pCAMRSZ->RSZ_VFILT2928), ((Rsz4TapModeVertFilterTable[28] << RSZ_VFILT_COEFL_SHIFT)
                | (Rsz4TapModeVertFilterTable[29] << RSZ_VFILT_COEFH_SHIFT)));
            OUTREG32((&dev->m_pCAMRSZ->RSZ_VFILT3130), ((Rsz4TapModeVertFilterTable[30] << RSZ_VFILT_COEFL_SHIFT)
                | (Rsz4TapModeVertFilterTable[31] << RSZ_VFILT_COEFH_SHIFT)));      }
    else
        {
            OUTREG32((&dev->m_pCAMRSZ->RSZ_VFILT10), ((Rsz7TapModeVertFilterTable[0] << RSZ_VFILT_COEFL_SHIFT)
                | (Rsz7TapModeVertFilterTable[1] << RSZ_VFILT_COEFH_SHIFT)));
            OUTREG32((&dev->m_pCAMRSZ->RSZ_VFILT32), ((Rsz7TapModeVertFilterTable[2] << RSZ_VFILT_COEFL_SHIFT)
                | (Rsz7TapModeVertFilterTable[3] << RSZ_VFILT_COEFH_SHIFT)));
            OUTREG32((&dev->m_pCAMRSZ->RSZ_VFILT54), ((Rsz7TapModeVertFilterTable[4] << RSZ_VFILT_COEFL_SHIFT)
                | (Rsz7TapModeVertFilterTable[5] << RSZ_VFILT_COEFH_SHIFT)));
            OUTREG32((&dev->m_pCAMRSZ->RSZ_VFILT76), ((Rsz7TapModeVertFilterTable[6] << RSZ_VFILT_COEFL_SHIFT)));

            OUTREG32((&dev->m_pCAMRSZ->RSZ_VFILT98), ((Rsz7TapModeVertFilterTable[7] << RSZ_VFILT_COEFL_SHIFT)
                | (Rsz7TapModeVertFilterTable[8] << RSZ_VFILT_COEFH_SHIFT)));
            OUTREG32((&dev->m_pCAMRSZ->RSZ_VFILT1110), ((Rsz7TapModeVertFilterTable[9] << RSZ_VFILT_COEFL_SHIFT)
                | (Rsz7TapModeVertFilterTable[10] << RSZ_VFILT_COEFH_SHIFT)));
            OUTREG32((&dev->m_pCAMRSZ->RSZ_VFILT1312), ((Rsz7TapModeVertFilterTable[11] << RSZ_VFILT_COEFL_SHIFT)
                | (Rsz7TapModeVertFilterTable[12] << RSZ_VFILT_COEFH_SHIFT)));
            OUTREG32((&dev->m_pCAMRSZ->RSZ_VFILT1514), ((Rsz7TapModeVertFilterTable[13] << RSZ_VFILT_COEFL_SHIFT)));


            OUTREG32((&dev->m_pCAMRSZ->RSZ_VFILT1716), ((Rsz7TapModeVertFilterTable[14] << RSZ_VFILT_COEFL_SHIFT)
                | (Rsz7TapModeVertFilterTable[15] << RSZ_VFILT_COEFH_SHIFT)));
            OUTREG32((&dev->m_pCAMRSZ->RSZ_VFILT1918), ((Rsz7TapModeVertFilterTable[16] << RSZ_VFILT_COEFL_SHIFT)
                | (Rsz7TapModeVertFilterTable[17] << RSZ_VFILT_COEFH_SHIFT)));
            OUTREG32((&dev->m_pCAMRSZ->RSZ_VFILT2120), ((Rsz7TapModeVertFilterTable[18] << RSZ_VFILT_COEFL_SHIFT)
                | (Rsz7TapModeVertFilterTable[19] << RSZ_VFILT_COEFH_SHIFT)));
            OUTREG32((&dev->m_pCAMRSZ->RSZ_VFILT2322), ((Rsz7TapModeVertFilterTable[20] << RSZ_VFILT_COEFL_SHIFT)));

            OUTREG32((&dev->m_pCAMRSZ->RSZ_VFILT2524), ((Rsz7TapModeVertFilterTable[21] << RSZ_VFILT_COEFL_SHIFT)
                | (Rsz7TapModeVertFilterTable[22] << RSZ_VFILT_COEFH_SHIFT)));
            OUTREG32((&dev->m_pCAMRSZ->RSZ_VFILT2726), ((Rsz7TapModeVertFilterTable[23] << RSZ_VFILT_COEFL_SHIFT)
                | (Rsz7TapModeVertFilterTable[24] << RSZ_VFILT_COEFH_SHIFT)));
            OUTREG32((&dev->m_pCAMRSZ->RSZ_VFILT2928), ((Rsz7TapModeVertFilterTable[25] << RSZ_VFILT_COEFL_SHIFT)
                | (Rsz7TapModeVertFilterTable[26] << RSZ_VFILT_COEFH_SHIFT)));
            OUTREG32((&dev->m_pCAMRSZ->RSZ_VFILT3130), ((Rsz7TapModeVertFilterTable[27] << RSZ_VFILT_COEFL_SHIFT)));
        }

    /* Configure RSZ in one shot mode */
    SETREG32(&dev->m_pCAMRSZ->RSZ_PCR, RSZ_PCR_ONESHOT);

    OUTREG32(&dev->m_pCAMRSZ->RSZ_SDR_INADD, pRSZParams->ulReadAddr + (pRSZParams->enableZoom ? pRSZParams->ulReadAddrOffset : 0));
    // Set SDR output address
    OUTREG32(&dev->m_pCAMRSZ->RSZ_SDR_OUTADD, pRSZParams->ulWriteAddr);

    rszContext->state = RSZ_LOADED;
    return TRUE;
}


//------------------------------------------------------------------------------
//
//  EnableRSZ
//  Description: This function enables the RSZ HW to resize data from memory/PRV
//
BOOL
EnableRSZHW(RSZ_CONTEXT_T * rszContext, BOOL bEnableFlag)
{  
    RSZ_DEVICE_CONTEXT_S * dev = rszContext->dev;
    
    if (bEnableFlag == TRUE)
    {
        if (rszContext->state != RSZ_LOADED)
        {
            RETAILMSG(TRUE, (L"ERROR: EnableRSZHW: state(%d) is not loaded \r\n",rszContext->state));
            return FALSE;
        }

            /* allow enable/disable only if this context's params are loaded */
        if (dev->inUseContext != (DWORD)rszContext)
        {
            RETAILMSG(TRUE, (L"ERROR: EnableRSZHW: Context(0x%x) is not the one loaded(0x%x) \r\n",rszContext,dev->inUseContext));
            return FALSE;
        }
        
        if ((INREG32(&dev->m_pCAMRSZ->RSZ_PCR) & RSZ_PCR_BUSY) == RSZ_PCR_BUSY)
        {
            RETAILMSG(TRUE,(L"ERROR: EnableRSZ: Busy bit is still set - cannot Enable until busy bit set\r\n"));
            return FALSE;
        }
        else 
        {
            SETREG32(&dev->m_pCAMRSZ->RSZ_PCR, RSZ_PCR_ENABLE);
            rszContext->state = RSZ_BUSY;
        }
    }
    else
    {        
        CLRREG32(&dev->m_pCAMRSZ->RSZ_PCR, RSZ_PCR_ENABLE);
        rszContext->state = RSZ_INIT;
        dev->inUseContext = 0;
    }
    return TRUE;
};


DWORD GetRSZHwStatus(RSZ_CONTEXT_T * rszContext)
{
    RSZ_DEVICE_CONTEXT_S * dev = rszContext->dev;
    DWORD status = (INREG32(&dev->m_pCAMRSZ->RSZ_PCR) & RSZ_PCR_BUSY);
    return status;
}

BOOL WaitRSZComplete(RSZ_CONTEXT_T * rszContext)
{
#define MAX_TIMEOUT 20
    RSZ_DEVICE_CONTEXT_S * dev = rszContext->dev;
    UINT32 timeout, dwCount,dwCalcTimeout;
    UINT32 maxTimeOut = MAX_TIMEOUT;
    int bpp;
    DWORD rsz_cnt = INREG32(&dev->m_pCAMRSZ->RSZ_CNT);
    DWORD inSize = INREG32(&dev->m_pCAMRSZ->RSZ_IN_SIZE);
    DWORD outSize = INREG32(&dev->m_pCAMRSZ->RSZ_OUT_SIZE);
    DWORD inType = (rsz_cnt & RSZ_CNT_INPTYP_MASK)>>RSZ_CNT_INPTYP_SHIFT;
    DWORD inWidth = (inSize & RSZ_IN_SIZE_HORZ_MASK)>>RSZ_IN_SIZE_HORZ_SHIFT;
    DWORD inHeight = (inSize & RSZ_IN_SIZE_VERT_MASK)>>RSZ_IN_SIZE_VERT_SHIFT;
    DWORD outWidth = (outSize & RSZ_OUT_SIZE_HORZ_MASK)>>RSZ_OUT_SIZE_HORZ_SHIFT;
    DWORD retCode = TRUE;

    /* dont wait for somebody else's submitted job */
    if (dev->inUseContext != (DWORD)rszContext)
    {
        RETAILMSG(TRUE, (L"ERROR: WaitRSZComplete: Context(0x%x) is not the one loaded(0x%x) \r\n",rszContext,dev->inUseContext));
        return FALSE;
    }

    if (inType == RSZ_INTYPE_YCBCR422_16BIT)
        bpp=2;
    else
        bpp=1;
        
    if ((inType==RSZ_INTYPE_YCBCR422_16BIT) && ((rsz_cnt & RSZ_CNT_HRSZ_MASK)>256))
    {
        timeout = (((inWidth+outWidth)/2)*inHeight*bpp)/CAM_FCLK;        
    }
    else
    {
        DWORD width = (inWidth>outWidth)?inWidth:outWidth;
        timeout = (width*inHeight*bpp)/CAM_FCLK;
    }
    timeout += 1;
    dwCalcTimeout=timeout;
    dwCount=0;
    
    while (GetRSZHwStatus(rszContext) && (maxTimeOut>0))
    {        
        Sleep(timeout);
        dwCount++;
        maxTimeOut-=timeout;
        timeout=1;        
    }
    if (maxTimeOut<=0) {
        RETAILMSG(TRUE,(L"ERROR: WaitRSZComplete: timeout\r\n"));
        retCode = FALSE;
    } else {
        if (dwCount>1)
            RETAILMSG(TRUE,(L"INFO:WaitRSZComplete: Calc Timeout=%d Time Waited=%d\r\n",dwCalcTimeout,(MAX_TIMEOUT-maxTimeOut)));
      /* do something later */        
    }
    rszContext->state = RSZ_INIT;
    dev->inUseContext = 0;

    return retCode;
    
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/** ============================================================================
 *  @func  DllEntry
 *
 *  @desc  Device driver DLL entry point. Any specific intialization is done
 *         here. The debug zones and GT are initialized here. The Successful
 *         loading of the DLL depends upon the retun value of any initialization.
 * 
 *  ============================================================================
 */ 
BOOL WINAPI DllEntry(HINSTANCE hDllInstance, INT nReason,
                         LPVOID lpvReserved)
{
    switch (nReason) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls((HMODULE)hDllInstance);            
            break;
        default:
            break;
    }

    return (TRUE);
}



DWORD
RSZ_Init(LPCTSTR pContext, LPCVOID lpvBusContext)
{
    rszDev.isInitialized = FALSE;
    rszDev.openRefCount = 0;
    rszDev.m_pCAMISP = NULL;
    rszDev.m_pCamMMU = NULL;
    rszDev.m_pCAMRSZ = NULL;
    rszDev.inUseContext = 0;

    return (DWORD)&rszDev;
        
}



BOOL 
RSZ_Deinit (DWORD hDeviceContext) 
{
    RSZ_DEVICE_CONTEXT_S * dev = (RSZ_DEVICE_CONTEXT_S *)hDeviceContext;
    if (dev->openRefCount != 0)
        return FALSE;
    
    if (dev->isInitialized)
    {
        DeinitializeRSZHW(dev);
        dev->isInitialized = FALSE;        
    }        
    dev->m_pCAMISP = NULL;
    dev->m_pCamMMU = NULL;
    dev->m_pCAMRSZ = NULL;
    dev->inUseContext = 0;

    return TRUE;
}



DWORD
RSZ_Open (DWORD hDeviceContext, DWORD AccessCode, DWORD ShareMode ) 
{

    RSZ_DEVICE_CONTEXT_S * dev = (RSZ_DEVICE_CONTEXT_S *)hDeviceContext;
    RSZ_CONTEXT_T * rszContext;
    
    if (dev->openRefCount == 0)    
        dev->isInitialized = InitializeRSZHW(dev);

    if (!(dev->isInitialized))
        goto cleanUP;

    rszContext = (RSZ_CONTEXT_T *)malloc(sizeof(RSZ_CONTEXT_T));
    if (rszContext == NULL)
        goto cleanUP;
    
    rszContext->dev = dev;
    rszContext->state = RSZ_UNINIT;
    dev->openRefCount++;        

    return ((DWORD)rszContext);

cleanUP:
    return NULL;
        
}



BOOL
RSZ_Close (DWORD hOpenContext)
{
    RSZ_CONTEXT_T * rszContext = (RSZ_CONTEXT_T *)hOpenContext;    
    RSZ_DEVICE_CONTEXT_S * dev = (RSZ_DEVICE_CONTEXT_S *)rszContext->dev;
    
    /* TODO: do we need to check for completion?? */
    free(rszContext);
    rszContext = NULL;
    dev->openRefCount--;

    if (dev->openRefCount == 0)
    {
        DeinitializeRSZHW(dev);
        dev->isInitialized = FALSE;        
    }

    return TRUE;

}



BOOL
RSZ_IOControl (DWORD hOpenContext, DWORD dwCode, PBYTE pBufIn, DWORD dwLenIn,
               PBYTE pBufOut, DWORD dwLenOut, PDWORD pdwActualOut)
{
    RSZ_CONTEXT_T * rszContext = (RSZ_CONTEXT_T *)hOpenContext;    
    RSZ_DEVICE_CONTEXT_S * dev = (RSZ_DEVICE_CONTEXT_S *)rszContext->dev;
    BOOL retCode = FALSE;
    RSZParams_t * params;

    //RETAILMSG(1,(L"RSZ_IOControl: dwcode %d rszContext 0x%x\r\n",dwCode,rszContext));

    switch (dwCode)
    {
        case RSZ_IOCTL_GET_PARAMS:
        {            
            if ((pBufOut) && (dwLenOut == sizeof(RSZParams_t)))
            {
                params = (RSZParams_t *)pBufOut;
                memcpy(params,&rszContext->params,sizeof(RSZParams_t));
                retCode = TRUE;
                if (pdwActualOut) *pdwActualOut=sizeof(RSZParams_t);
            }
        
        }
        break;

        case RSZ_IOCTL_SET_PARAMS:
        {            
            if ((pBufIn) && (dwLenIn == sizeof(RSZParams_t)))
            {
                retCode = SetRSZParams((RSZParams_t *)pBufIn,rszContext);                  
                if ((retCode) && (pBufOut)&&
                    (dwLenOut == sizeof(RSZParams_t)))
                {
                    params = (RSZParams_t *)pBufOut;
                    memcpy(params,&rszContext->params,sizeof(RSZParams_t));
                    if (pdwActualOut) *pdwActualOut=sizeof(RSZParams_t);
                }                    
            }            
        }
        break;

        case RSZ_IOCTL_GET_STATUS:
        {            
            if ((pBufOut) && (dwLenIn == sizeof(DWORD)))
            {
                *(DWORD *)pBufOut= GetRSZHwStatus(rszContext);
                if (pdwActualOut) *pdwActualOut=sizeof(DWORD);
                retCode = TRUE;
            }
        }
        break;

        case RSZ_IOCTL_START_RESIZER:
        {            
            if ((pBufIn) && (dwLenIn == sizeof(RSZParams_t)))
            {
                params = (RSZParams_t *)pBufIn;
                retCode = setADDR_RSZParams(rszContext,params);
                if (retCode)                
                    retCode = ConfigureRSZHW(rszContext);
                if (retCode)
                    retCode = EnableRSZHW(rszContext, TRUE);
                if (retCode)
                    retCode = WaitRSZComplete(rszContext);
            }
        }
        break;            
    }

    return retCode;

}




DWORD
RSZ_Read (DWORD hOpenContext, LPVOID pBuffer, DWORD Count )
{
    return (0);
}

DWORD
RSZ_Write (DWORD hOpenContext, LPCVOID pBuffer, DWORD Count )
{
    return (0);     
}

void
RSZ_PowerUp (DWORD hDeviceContext)
{    
    return ;     
}

void
RSZ_PowerDown (DWORD hDeviceContext)
{
    return ;     
}


#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */





