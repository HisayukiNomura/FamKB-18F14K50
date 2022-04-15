
#include <stdint.h>
#include "system.h"
#include "globaldef.h"
#include "usb/usb_device.h"
#include "mcc_generated_files/pin_manager.h"

/*
 * USB�̃X�e�[�^�X��LED�ŕ\��������BRC6���g�p���邪�A�����Scroll Lock�ł��g���̂ŁA
 * �V���{�� USE_USBLED�@���g���āAUSB�X�e�[�^�X�͎g�p���Ȃ��悤�ɂ��Ă����B�f�o�b�O���ɂ�
 * globaldef.h���ւ񂱂�����B
*/

void APP_LEDUpdateUSBStatus(void)
{
#ifdef USE_USBLED
    static uint16_t ledCount = 0;

    if(USBIsDeviceSuspended() == true) {
        IO_RC6_SetLow();
        return;
    }

    switch(USBGetDeviceState())
    {         
        case CONFIGURED_STATE:
            /* �\�������B75ms���Ƃɓ_�ł�����*/
            if(ledCount == 1) {
                IO_RC7_SetHigh();
            } else if(ledCount == 75) {
                IO_RC7_SetLow();
            } else if(ledCount > 150) {
                ledCount = 0;
            }
            break;
        default:
            /*���\���ŃT�X�y���h����Ă��Ȃ��B�����ƒx���y�[�X�ē_�ł�����B50ms�I���A950ms�I�t*/
            if(ledCount == 1) {
                IO_RC6_SetHigh();
            } else if(ledCount == 50) {
                IO_RC6_SetLow();
            } else if(ledCount > 950) {
                ledCount = 0;
            }
            break;
    }
    ledCount++;
#endif
}
