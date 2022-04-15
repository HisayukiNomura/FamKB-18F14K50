
#ifndef __GLOBAL_DEF_H__
#define	__GLOBAL_DEF_H__

#include <xc.h> // include processor files - each processor file is guarded.  


#ifdef	__cplusplus
extern "C" {
#endif /* __cplusplus */

#define USE_KEYBORAD_LED      // キーボードのLEDを使用する。
// #define USE_USBLED             // USBの接続状態LEDとしてIO_RC6を使用する。この設定が有効の場合、キーボードLEDのうち、RC6を使用するSCROLL LOCKは無効になる。
    
    
#ifdef	__cplusplus
}
#endif /* __cplusplus */

#endif	/* XC_HEADER_TEMPLATE_H */

