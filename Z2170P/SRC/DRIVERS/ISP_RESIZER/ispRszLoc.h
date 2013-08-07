/*
================================================================================
*             Texas Instruments OMAP(TM) Platform Software
* (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
*
* Use of this software is controlled by the terms and conditions found
* in the license agreement under which this software has been supplied.
*
================================================================================
*/

#ifndef __ISPRSZLOC_H__
#define __ISPRSZLOC_H__


#define ALIGN 32
#define PIXEL_EVEN      2
#define MAX_HORZ_PIXEL_8BIT 31
#define MAX_HORZ_PIXEL_16BIT    15

#define CAM_FCLK 166000 /* in KHZ */


/* REGS */

#define ISP_RSZ_BASE_ADDRESS 0x480BD000

typedef volatile struct {
  UINT32 RSZ_PID;           //0x0000
  UINT32 RSZ_PCR;           //0x4
  UINT32 RSZ_CNT;           //0x8
  UINT32 RSZ_OUT_SIZE;      //0xC
  UINT32 RSZ_IN_START;      //0x10
  UINT32 RSZ_IN_SIZE;       //0x14
  UINT32 RSZ_SDR_INADD;     //0x18
  UINT32 RSZ_SDR_INOFF;     //0x1C
  UINT32 RSZ_SDR_OUTADD;    //0x20
  UINT32 RSZ_SDR_OUTOFF;    //0x24
  UINT32 RSZ_HFILT10;       //0x28
  UINT32 RSZ_HFILT32;       //0x2C
  UINT32 RSZ_HFILT54;       //0x30
  UINT32 RSZ_HFILT76;       //0x34
  UINT32 RSZ_HFILT98;       //0x38
  UINT32 RSZ_HFILT1110;     //0x3C
  UINT32 RSZ_HFILT1312;     //0x40
  UINT32 RSZ_HFILT1514;     //0x44
  UINT32 RSZ_HFILT1716;     //0x48
  UINT32 RSZ_HFILT1918;     //0x4C
  UINT32 RSZ_HFILT2120;     //0x50
  UINT32 RSZ_HFILT2322;     //0x54
  UINT32 RSZ_HFILT2524;     //0x58
  UINT32 RSZ_HFILT2726;     //0x5C
  UINT32 RSZ_HFILT2928;     //0x60
  UINT32 RSZ_HFILT3130;     //0x64
  UINT32 RSZ_VFILT10;       //0x68
  UINT32 RSZ_VFILT32;       //0x6C
  UINT32 RSZ_VFILT54;       //0x70
  UINT32 RSZ_VFILT76;       //0x74
  UINT32 RSZ_VFILT98;       //0x78
  UINT32 RSZ_VFILT1110;     //0x7C
  UINT32 RSZ_VFILT1312;     //0x80
  UINT32 RSZ_VFILT1514;     //0x84
  UINT32 RSZ_VFILT1716;     //0x88
  UINT32 RSZ_VFILT1918;     //0x8C
  UINT32 RSZ_VFILT2120;     //0x90
  UINT32 RSZ_VFILT2322;     //0x94
  UINT32 RSZ_VFILT2524;     //0x98
  UINT32 RSZ_VFILT2726;     //0x9C
  UINT32 RSZ_VFILT2928;     //0xA0
  UINT32 RSZ_VFILT3130;     //0xA4
  UINT32 RSZ_YENH;          //0xA8
} OMAP_ISP_RSZ_REGS;

#define RSZ_PID_PREV_SHIFT          0
#define RSZ_PID_CID_SHIFT           8
#define RSZ_PID_TID_SHIFT           16

#define RSZ_PCR_ENABLE              (1 << 0)
#define RSZ_PCR_BUSY                (1 << 1)
#define RSZ_PCR_ONESHOT             (1 << 2)

#define RSZ_CNT_HRSZ_SHIFT          0
#define RSZ_CNT_HRSZ_MASK           0x3FF
#define RSZ_CNT_VRSZ_SHIFT          10
#define RSZ_CNT_VRSZ_MASK           0xFFC00
#define RSZ_CNT_HSTPH_SHIFT         20
#define RSZ_CNT_HSTPH_MASK          0x700000
#define RSZ_CNT_VSTPH_SHIFT         23
#define RSZ_CNT_VSTPH_MASK          0x3800000
#define RSZ_CNT_CBILIN_MASK         0x20000000
#define RSZ_CNT_INPTYP_MASK         0x08000000
#define RSZ_CNT_INPTYP_SHIFT        27
#define RSZ_CNT_PIXFMT_MASK         0x04000000
#define RSZ_CNT_YCPOS               (1 << 26) 
#define RSZ_CNT_INPTYP              (1 << 27)
#define RSZ_CNT_INPSRC              (1 << 28)
#define RSZ_CNT_CBILIN              (1 << 29)
#define RSZ_IN_START_HORZ_ST_SHIFT      0
#define RSZ_IN_START_VERT_ST_SHIFT      16
#define RSZ_IN_SIZE_HORZ_SHIFT      0
#define RSZ_IN_SIZE_HORZ_MASK       0x00001FFF
#define RSZ_IN_SIZE_VERT_SHIFT      16
#define RSZ_IN_SIZE_VERT_MASK       0x1FFF0000
#define RSZ_OUT_SIZE_HORZ_SHIFT     0
#define RSZ_OUT_SIZE_HORZ_MASK      0x00000FFF
#define RSZ_OUT_SIZE_VERT_SHIFT     16
#define RSZ_OUT_SIZE_VERT_MASK      0x0FFF0000
#define RSZ_SDR_OUTOFF_OFFSET_SHIFT     0
#define RSZ_HFILT_COEFL_SHIFT       0
#define RSZ_HFILT_COEFH_SHIFT       16

#define RSZ_VFILT_COEFL_SHIFT       0
#define RSZ_VFILT_COEFH_SHIFT       16

// Size bounds for resizer operation
#define RSZ_MAX_IN_HEIGHT                  4095
#define RSZ_MINIMUM_RESIZE_VALUE           64
#define RSZ_MAXIMUM_RESIZE_VALUE           1024
#define RSZ_MID_RESIZE_VALUE               512
#define RSZ_MAX_7TAP_HRSZ_OUTWIDTH_ES2     3300
#define RSZ_MAX_7TAP_VRSZ_OUTWIDTH_ES2     1650
#define RSZ_MAX_IN_WIDTH_ONTHEFLY_MODE_ES2 4095
#define RSZ_DEFAULTSTPIXEL                 0
#define RSZ_BYTE_ALIGN                     32

#define RSZ_CNT_HRSZ(x)             (x <<  0) // 9  - 0
#define RSZ_CNT_VRSZ(x)             (x << 10) // 19 - 10
#define RSZ_CNT_HSTPH(x)            (x << 20) // 22 - 20 range 0 - 7
#define RSZ_CNT_VSTPH(x)            (x << 23) // 25 - 23 range 0 - 7
#define RSZ_OUT_SIZE_HORZ(x)        (x <<  0) //10 - 0
#define RSZ_OUT_SIZE_VERT(x)        (x << 16) //26 - 16

#define RSZ_IN_START_HORZ_ST(x)     (x << 0)  //12 - 0
#define RSZ_IN_START_VERT_ST(x)     (x << 16) //28 - 16

#define RSZ_IN_SIZE_HORZ(x)         (x <<  0) //12 - 0
#define RSZ_IN_SIZE_VERT(x)         (x << 16) //28 - 16



#define OMAP_CAMISP_REGS_PA                     0x480BC000 // Camera ISP
#define ISP_IRQ0ENABLE_RSZ_DONE_IRQ      (1 << 24)
#define ISP_CTRL_RSZ_CLK_EN              (1 << 13)
#define ISP_CTRL_SBL_RD_RAM_EN           (1 << 18)
#define ISP_CTRL_SBL_WR0_RAM_EN          (1 << 20)


typedef volatile struct {     //offset

  UINT32 ISP_REVISION;           //0x0
  UINT32 ISP_SYSCONFIG;          //0x4
  UINT32 ISP_SYSSTATUS;          //0x8
  UINT32 ISP_IRQ0ENABLE;         //0xC
  UINT32 ISP_IRQ0STATUS;         //0x10
  UINT32 ISP_IRQ1ENABLE;         //0x14
  UINT32 ISP_IRQ1STATUS;         //0x18
  UINT32 ISP_Reserved_1C_2F[05]; //0x1C-2F
  UINT32 TCTRL_GRESET_LENGTH;    //0x30
  UINT32 TCTRL_PSTRB_REPLAY;     //0x34
  UINT32 ISP_Reserved_38_3F[02]; //0x38-0x3F
  UINT32 ISP_CTRL;               //0x40
  UINT32 ISP_SECURE;             //0x44
  UINT32 ISP_Reserved_48_4F[02]; //0x48-0x4F
  UINT32 TCTRL_CTRL;             //0x50
  UINT32 TCTRL_FRAME;            //0x54
  UINT32 TCTRL_PSTRB_DELAY;      //0x58
  UINT32 TCTRL_STRB_DELAY;       //0x5C
  UINT32 TCTRL_SHUT_DELAY;       //0x60
  UINT32 TCTRL_PSTRB_LENGTH;     //0x64
  UINT32 TCTRL_STRB_LENGTH;      //0x68
  UINT32 TCTRL_SHUT_LENGTH;      //0x6C
  UINT32 PING_PONG_ADDR;         //0x70
  UINT32 PING_PONG_MEM_RANGE;    //0x74
  UINT32 PING_PONG_BUF_SIZE;     //0x78

} OMAP_CAM_ISP_REGS;


#define CAM_MMU_BASE_ADDRESS 0x480BD400
#define CAMMMU_CNTL_MMU_EN           (1 << 1)

typedef volatile struct {
   UINT32 MMU_REVISION;      //0x00
   UINT32 MMU_Reserved1[3];  //0x04-0x0C reserved
   UINT32 MMU_SYSCONFIG;     //0x10
   UINT32 MMU_SYSSTATUS;     //0x14
   UINT32 MMU_IRQSTATUS;     //0x18
   UINT32 MMU_IRQENABLE;     //0x1C
   UINT32 MMU_Reserved2[8];  //0x20 - 0x3F reserved
   UINT32 MMU_WALKING_ST;    //0x40
   UINT32 MMU_CNTL;          //0x44
   UINT32 MMU_FAULT_AD;      //0x48
   UINT32 MMU_TTB;           //0x4C
   UINT32 MMU_LOCK;          //0x50
   UINT32 MMU_LD_TLB;        //0x54
   UINT32 MMU_CAM;           //0x58
   UINT32 MMU_RAM;           //0x5C
   UINT32 MMU_GFLUSH;        //0x60
   UINT32 MMU_FLUSH_ENTRY;   //0x64
   UINT32 MMU_READ_CAM;      //0x68
   UINT32 MMU_READ_RAM;      //0x6C
   UINT32 MMU_EMU_FAULT_AD;  //0x70
}OMAP_CAM_MMU_REGS;

typedef struct rsz_prev_state_t {
    DWORD irqEnable;
    DWORD mmuEnable;    
    DWORD ispCtrl;
}RSZ_PREV_STATE_S;


typedef enum RSZ_STATE_E {
    RSZ_UNINIT = 0,
    RSZ_INIT,
    RSZ_LOADED,
    RSZ_BUSY,
    RSZ_FREE
}RSZ_STATE_E;
    

/* Context */
typedef struct rsz_device_context_t {
    BOOL                 isInitialized;
    UINT32               openRefCount;
    OMAP_ISP_RSZ_REGS   *m_pCAMRSZ;
    OMAP_CAM_ISP_REGS   *m_pCAMISP;
    OMAP_CAM_MMU_REGS   *m_pCamMMU;
    RSZ_PREV_STATE_S     prevState;
    DWORD                inUseContext;
}RSZ_DEVICE_CONTEXT_S;

typedef struct rsz_context_t {
    RSZParams_t             params;   
    RSZ_STATE_E             state;
    RSZ_DEVICE_CONTEXT_S   *dev;
}RSZ_CONTEXT_T;



#endif

