// All rights reserved ADENEO EMBEDDED 2010
// Copyright (c) 2007, 2008 BSQUARE Corporation. All rights reserved.

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
//
//  File:  ioctl_tab.h
//
//  Configuration file for the OAL IOCTL component.
//
//  This file is included by the platform's ioctl.c file and defines the 
//  global IOCTL table, g_oalIoCtlTable[]. Therefore, this file may ONLY
//  define OAL_IOCTL_HANDLER entries. 
//
// IOCTL CODE,                          Flags   Handler Function
//------------------------------------------------------------------------------

{ IOCTL_HAL_RTC_TIME ,                  0,  OALIoCtlHalRtcTime              },
{ IOCTL_HAL_RTC_ALARM,                  0,  OALIoCtlHalRtcAlarm             },

{ IOCTL_HAL_TRANSLATE_IRQ,              0,  OALIoCtlHalRequestSysIntr       },
{ IOCTL_HAL_REQUEST_SYSINTR,            0,  OALIoCtlHalRequestSysIntr       },
{ IOCTL_HAL_RELEASE_SYSINTR,            0,  OALIoCtlHalReleaseSysIntr       },
{ IOCTL_HAL_REQUEST_IRQ,                0,  OALIoCtlHalRequestIrq           },
{ IOCTL_HAL_IRQ2SYSINTR,                0,  OALIoCtlHalIrq2Sysintr          },
{ IOCTL_HAL_ILTIMING,                   0,  OALIoCtlHalILTiming             },

{ IOCTL_HAL_INITREGISTRY,               0,  BSPIoCtlHalInitRegistry         },
{ IOCTL_HAL_INIT_RTC,                   0,  OALIoCtlHalInitRTC              },
{ IOCTL_HAL_REBOOT,                     0,  OALIoCtlHalReboot               },

{ IOCTL_HAL_DDK_CALL,                   0,  OALIoCtlHalDdkCall              },

{ IOCTL_HAL_DISABLE_WAKE,               0,  OALIoCtlHalDisableWake          },
{ IOCTL_HAL_ENABLE_WAKE,                0,  OALIoCtlHalEnableWake           },
{ IOCTL_HAL_GET_WAKE_SOURCE,            0,  OALIoCtlHalGetWakeSource        },

{ IOCTL_HAL_GET_CACHE_INFO,             0,  OALIoCtlHalGetCacheInfo         },
{ IOCTL_HAL_GET_DEVICE_INFO,            0,  OALIoCtlHalGetDeviceInfo        },
{ IOCTL_HAL_GET_DEVICEID,               0,  OALIoCtlHalGetDeviceId          },
{ IOCTL_HAL_GET_UUID,                   0,  OALIoCtlHalGetUUID              },
{ IOCTL_PROCESSOR_INFORMATION,          0,  OALIoCtlProcessorInfo           },
{ IOCTL_HAL_GET_CPUID,                  0,  OALIoCtlHalGetCpuID             },
{ IOCTL_HAL_GET_DIEID,                  0,  OALIoCtlHalGetDieID             },
{ IOCTL_HAL_GET_CPUFAMILY,              0,  OALIoCtlHalGetCpuFamily         },
{ IOCTL_HAL_GET_CPUREVISION,            0,  OALIoCtlHalGetCpuRevision       },
{ IOCTL_HAL_GET_CPUSPEED,              0,  OALIoCtlHalGetCpuSpeed         },
{ IOCTL_HAL_UPDATE_MODE,                0,  OALIoCtlHalUpdateMode           },

{ IOCTL_HAL_GET_HWENTROPY,              0,  OALIoCtlHalGetHWEntropy         },
{ IOCTL_HAL_GETREGSECUREKEYS,           0,  OALIoCtlHalGetRegSecureKeys     },

{ IOCTL_HAL_QUERY_FORMAT_PARTITION,     0,  OALIoCtlHalQueryFormatPartition },

{ IOCTL_HAL_GET_RNDIS_MACADDR,          0,  OALIoCtlHalGetRNdisMacAddr      },
{ IOCTL_HAL_GET_DISPLAY_RES,            0,  OALIoctlGetDisplayRes           },
{ IOCTL_HAL_CONVERT_CA_TO_PA,           0,  OALIoctlConvertCAtoPA           },


{ IOCTL_HAL_POSTINIT,                   0,  OALIoCtlHALPostInit             },
{ IOCTL_HAL_OEM_PROFILER,               0,  OALIoCtlHALProfiler             },
{ IOCTL_HAL_I2CHSMODE,                  0,  OALIoCtlHalI2CMode              },
{ IOCTL_HAL_I2CCOPYFNTABLE,             0,  OALIoCtlHalI2CCopyFnTable       },
{ IOCTL_HAL_PADCFGCOPYFNTABLE,             0,  OALIoCtlHalPadCfgCopyFnTable       },

{ IOCTL_HAL_GET_BSP_VERSION,            0,  OALIoCtlHalGetBspVersion        },
{ IOCTL_HAL_GET_NEON_STATS,             0,  OALIoctlHalGetNeonStats         },
{ IOCTL_HAL_GET_ECC_TYPE,             0,  OALIoctlHalGetEccType         },

{ IOCTL_HAL_CONTEXTSAVE_GETBUFFER,      0,  OALIoCtlHalContextSaveGetBuffer },
{ IOCTL_PRCM_DEVICE_GET_DEVICEMANAGEMENTTABLE, 0, OALIoCtlPrcmDeviceGetDeviceManagementTable},
{ IOCTL_PRCM_DEVICE_GET_SOURCECLOCKINFO,0,  OALIoCtlPrcmDeviceGetSourceClockInfo},
{ IOCTL_PRCM_DEVICE_GET_DEVICESTATUS,   0,  OALIoCtlPrcmDeviceGetDeviceStatus},
{ IOCTL_PRCM_CLOCK_GET_SOURCECLOCKINFO, 0,  OALIoCtlPrcmClockGetSourceClockInfo},
{ IOCTL_PRCM_CLOCK_SET_SOURCECLOCK,     0,  OALIoCtlPrcmClockSetSourceClock },
{ IOCTL_PRCM_CLOCK_SET_SOURCECLOCKDIVISOR, 0, OALIoCtlPrcmClockSetSourceClockDivisor},
{ IOCTL_PRCM_CLOCK_SET_DPLLSTATE,       0,  OALIoCtlPrcmClockSetDpllState   },
{ IOCTL_PRCM_DOMAIN_SET_WAKEUPDEP,      0,  OALIoCtlPrcmDomainSetWakeupDependency},
{ IOCTL_PRCM_DOMAIN_SET_SLEEPDEP,       0,  OALIoCtlPrcmDomainSetSleepDependency},
{ IOCTL_PRCM_DOMAIN_SET_POWERSTATE,     0,  OALIoCtlPrcmDomainSetPowerState },
{ IOCTL_OPP_REQUEST,                    0,  OALIoCtlOppRequest},
{ IOCTL_SMARTREFLEX_CONTROL,            0,  OALIoCtlSmartReflexControl},
{ IOCTL_UPDATE_RETENTION_VOLTAGES,      0,  OALIoCtlUpdateRetentionVoltages },
{ IOCTL_INTERRUPT_LATENCY_CONSTRAINT,   0,  OALIoCtlInterruptLatencyConstraint},

{ IOCTL_PRCM_SET_SUSPENDSTATE,          0,  OALIoCtlPrcmSetSuspendState     },

{ IOCTL_HAL_PROFILE,                    0,  OALIoCtlIgnore },
{ IOCTL_HAL_DUMP_REGISTERS,             0,  OALIoCtlHalDumpRegisters },

// Required Termination
{ 0,                                    0,  NULL                        }
