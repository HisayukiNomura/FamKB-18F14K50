
#include <stdint.h>
#include "system.h"
#include "globaldef.h"
#include "usb/usb_device.h"
#include "mcc_generated_files/pin_manager.h"

/*
 * USBのステータスをLEDで表示させる。RC6を使用するが、これはScroll Lockでも使うので、
 * シンボル USE_USBLED　を使って、USBステータスは使用しないようにしておく。デバッグ時には
 * globaldef.hをへんこうする。
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
            /* 構成完了。75msごとに点滅させる*/
            if(ledCount == 1) {
                IO_RC7_SetHigh();
            } else if(ledCount == 75) {
                IO_RC7_SetLow();
            } else if(ledCount > 150) {
                ledCount = 0;
            }
            break;
        default:
            /*未構成でサスペンドされていない。もっと遅いペースて点滅させる。50msオン、950msオフ*/
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
