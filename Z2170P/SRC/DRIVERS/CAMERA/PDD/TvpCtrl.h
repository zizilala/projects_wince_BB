// All rights reserved ADENEO EMBEDDED 2010

#ifndef _TVP_CTRL_H
#define _TVP_CTRL_H


#ifdef __cplusplus
extern "C" {
#endif


class CTvpCtrl
{
public:
    
    CTvpCtrl();
    ~CTvpCtrl();
    
    BOOL Init();
    BOOL SelectComposite();
    BOOL SelectSVideo();
    BOOL SelectComponent();
    BOOL SetPowerState(BOOL PowerOn);
    
private:
    HANDLE    m_hI2C;
    
    BOOL ReadReg(UINT8 slaveaddress,UINT8* data);
    BOOL WriteReg(UINT8 slaveaddress,UINT8 value);
    BOOL I2CInit();
    BOOL I2CDeinit();
};

#ifdef __cplusplus
}
#endif

#endif//_TVP_CTRL_H