/*
 * File:   FamKey.c
 * Author: hisay
 *
 * Created on 2022/04/08, 13:07
 */


#include <xc.h>

#include <stdbool.h>
#include <stdint.h>
#include "mcc_generated_files/pin_manager.h"
#include "FamKey.h"
#include "mcc_generated_files/device_config.h"


/*
 * キーボードから値を読み込んで、グローバル変数KeyReportにキーコード、modifierに、
 * 修飾キー（SHIFTとか）の押下状態を設定する
 */
uint8_t modifier;
uint8_t KeyReport[6];       // 押されているキーの情報
uint8_t KeyRepIdx;          // 格納されているキーコードの数



uint8_t KeyBuffer[9];           // キーマトリックスの状態を示す変数

/*
 * USBキーボードのキーコード表と、ファミリーベーシックキーボードのマトリックス対応
 *  http://www2d.biglobe.ne.jp/~msyk/keyboard/layout/usbkeycode.html
 * 　　→　USBキーコード表
 *  http://www43.tok2.com/home/cmpslv/Famic/Fambas.htm　
 * 　　→　キーボードのマトリックス
 * 　
 * この配列は、キーボードマトリックス（8x9）の大きさで、それぞれにUSBキーボードのコード
 * が入っている。0xFFは、キーコードを発生させない。
 */

// 標準状態のマトリックス対応。
// 基本的には、キートップと対応したキーに割り当てたが、一部変更を加えている。個人的に
// 必須と思われるキーが存在しないため。
// ・かなキーは、存在しないキーを表現するための修飾キーとして操作されるようにしたので、
//   単体で押してもキーコードは発生しない。
// ・ESC(29)キーは、TAB(2B)にしてある。かな＋ESCでESCが生成されるようになっている。
//   キートップと逆だが。Qの横にはTABが欲しいし、TABのつもりでESCを押すとショックが大きいので
// ・CLR→CAPSLock。キートップはCLR/HOME なので、Homeに割り当てようと思ったが、ENDと
//   ペアで割り当てたい。そこで、かな＋左、かな＋右をHOME/ENDにした。余ったCLRをCapsLockに。
// ・その他のマッピング変更
//    GRP→左ALT、STOP→BS
uint8_t *KeyUsagePtr;
uint8_t key_usage[72] = {
  0x41,0x28,0x30,0x32,0x35,0xff,0x89,0x2a, // F8  RET   [   ]   FUNC  R-SFT ￥ STOP
  0x40,0x2f,0x34,0x33,0x87,0x38,0x2d,0x2e, // F7   レ   モ  ＊    ン    ヲ  ラ   リ
  0x3f,0x12,0x0f,0x0e,0x37,0x36,0x13,0x27, // F6   o    l   k    ,     .    p   0
  0x3e,0x0c,0x18,0x0d,0x10,0x11,0x26,0x25, // F5   i    u   j    m     n    9   8
  0x3d,0x1c,0x0a,0x0b,0x05,0x19,0x24,0x23, // F4   y    g   h    b     v    7   6
  0x3c,0x17,0x15,0x07,0x09,0x06,0x22,0x21, // F3   t    r   d    f     c    5   4
  0x3b,0x1a,0x16,0x04,0x1b,0x1d,0x08,0x20, // F2   w    s   a    x     z    e   3
  0x3a,0x2B,0x14,0xff,0xff,0xff,0x1e,0x1f, // F1 ESC    Q  CTR R-SFT  GRPH  1  2
  0x39,0x52,0x4f,0x50,0x51,0x2c,0x4c,0x49 // CLR ↑   →   ←   ↓   SPC  DEL INS
};
//
//かなを押しながらは、違うコードを生成させる
// F1-F4→F9-F12　 F5-F7→PRSC-PAUSE　　STOP→漢字
// ESC→ESC 上→PageUp 下→PageDWN 左→HOME　右→END
// CLR→無変換　INS→変換　DEL→かな
// z → capslock X→Win  ン→アプリケーション
uint8_t key_usage_EXPKey[72] = {
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x35, // ×　 ×   ×   ×   FUNC  R-SFT× 漢字
  0x48,0xff,0xff,0xff,0xff,0x65,0xff,0xff, // PUSE ×   ×   ×   APL   ×   ×  ×
  0x47,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // SLCK ×   ×   ×   ×    ×   ×  ×
  0x46,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // PRSC ×   ×   ×   ×    ×   ×  ×
  0x45,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // F12  ×   ×   ×   ×    ×   ×  ×
  0x44,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // F11  ×   ×   ×   ×    ×   ×  ×
  0x43,0xff,0xff,0xff,0xe3,0x39,0xff,0xff, // F10  ×   ×   ×  Win    CAPS ×  ×
  0x42,0x29,0xff,0xff,0xff,0xff,0xff,0xff, // F9   TAB  ×　 CTR L-SFT  GRPH ×  ×
  0x8b,0x4b,0x4d,0x4a,0x4e,0x2c,0x8a,0x88  // CLR  PU   END  HOME PDN   SPC  無変 変換
};
#define DELAY_LONG  __delay_us(50);
#define DELAY_SHORT __delay_us(10); 



/*
 * キーボードのマトリックスを変換する.
 *   KeyBuffer uint8_t[9]　のポインタ。　押されたキーのマトリックスが格納されている。
 * 　pModifier 押されている修飾キーのマスク
 *   keyCodes　押されているキーのキーコード
 * KeyBufferに指定されている押されたキーマトリックスをもとに、pModifierとKeyCodesを埋めていく。
 * 
 * 
 */
void ProcessMatrix (uint8_t* Buf , uint8_t* pModifier , uint8_t* KeyCodes)
{
    uint8_t mod = 0;
    //
    // 修飾キーの処理
    //
    if ((Buf[7] & 0x08) != 0) {       // CTRL→左CTRL
        mod |= ATBKEY_LEFTCTRL;
	}
  	if ((Buf[7] & 0x10) != 0) {       // 左Shift→左Shift
        mod |=ATBKEY_LEFTSHIFT;
    }
    if ((Buf[0] & 0x20) != 0) {       // 右SHIFT→右Shift
        mod |= ATBKEY_RIGHTSHIFT;
    }
    if ((Buf[7] & 0x20) != 0) {       // GRAPH→左ALT
        mod |= ATBKEY_LEFTALT;
    }
    
    if ((Buf[0] & 0x10) != 0) {       // カナキーと修飾キーの同時押しを処理する。
        if (mod & ATBKEY_LEFTCTRL) {        // かな＋CTRLで、右CTRLとする
            mod &= ~ATBKEY_LEFTCTRL;
            mod |= ATBKEY_RIGHTCTRL;
        }
        if (mod & ATBKEY_LEFTALT) {        // かな＋ALTで、右ALTとする
            mod &= ~ATBKEY_LEFTALT;
            mod |= ATBKEY_RIGHTALT;
        }
        
    }
     
    *pModifier = mod;

    //
    // カナキーをアトリビュートとして使うようにする。
    // かなキーが押されていたら、違うキーを生成する。
    //
	if ((Buf[0] & 0x10) != 0) {       // カナキーだったら、テーブルを置き換えてかなキーは食ってしまう
        Buf[0]  = Buf[0] & 0b11101111;
        KeyUsagePtr = key_usage_EXPKey;
	}  else {
        KeyUsagePtr = key_usage;
    }
     
    
    KeyRepIdx = 0;
    int UsageIdx = 0;   
    
    for (int j=0; j<9; j++) {
		uint8_t inkey = Buf[j];
		for(int i=0; i<8; i++) {
			if (KeyRepIdx < 6) {
				if ((inkey & 0x01) != 0) {
					uint8_t usg = KeyUsagePtr[UsageIdx];
					if (usg != 0xff) {
						KeyCodes[KeyRepIdx] = usg;
						KeyRepIdx++;
					}
				}
			}
			UsageIdx++;
			inkey >>= 1;
		}
	}
   
    
}





inline void KBReset(void) 
{
    IO_RESET_RB5_SetHigh();
    DELAY_SHORT
    IO_RESET_RB5_SetLow();
    DELAY_LONG
    
}
// 上位データのセレクト
inline void  KBSetHigh()
{
    IO_DATASEL_RB6_SetHigh();        // 上位のデータセレクト
    DELAY_LONG
    
}
// 下位データのセレクト
inline void  KBSetLow()
{
    IO_DATASEL_RB6_SetLow();        // 下位のデータセレクト
    DELAY_LONG
}





void ReadKey(void)
{
    // デバイスセレクト
    //    IO_DEVSEL_RB7_SetHigh();
    __delay_us(200);
    // リセット
    KBReset();

    
    // キーマトリックスから押されているキーを取得する
    for (int i = 0 ; i < 9 ; i++) {
        uint8_t wkBits = 0;
        wkBits = (PORTC & 0b00001111); 

        KBSetHigh();

        wkBits |= ((PORTC << 4) & 0b11110000);
        KeyBuffer[i] = wkBits;
        KBSetLow();
        /*
        if (i==1) {
           //if (wkBits & 0b00000001) IO_RC5_SetHigh(); else  IO_RC5_SetLow(); // 緑   F8    F7
           //if (wkBits & 0b00000010) IO_RC6_SetHigh(); else  IO_RC6_SetLow(); // 橙   RET   レ
           //if (wkBits & 0b00000100) IO_RC7_SetHigh(); else  IO_RC7_SetLow(); // 赤    [  　＊
           //if (wkBits & 0b00001000) IO_RC5_SetHigh(); else  IO_RC5_SetLow(); // 緑   ]   　モ
           //if (wkBits & 0b00010000) IO_RC6_SetHigh(); else  IO_RC6_SetLow(); // 橙  かな   ン
           //if (wkBits & 0b00100000) IO_RC7_SetHigh(); else  IO_RC7_SetLow(); // 赤 R-SFT   ヲ
           //if (wkBits & 0b01000000) IO_RC5_SetHigh(); else  IO_RC5_SetLow(); // 緑   \     ラ
           //if (wkBits & 0b10000000) IO_RC6_SetHigh(); else  IO_RC6_SetLow(); // 橙  STOP　 リ
        }
        */
    }
    
    // 読み込んだキーを押されているキー番号にしていく
   
    uint8_t mod = 0;
    for(int i=0; i< sizeof(KeyReport); i++) {
        KeyReport[i] = 0;
    }
    
    ProcessMatrix(KeyBuffer,&modifier,KeyReport);
    
    //if (KeyRepIdx != 0) IO_RC5_SetHigh(); else IO_RC5_SetLow();
    
    
}

bool isTestButton(void)
{
    if (IO_RC4_GetValue() == 0 ) {
        return true;
    }
    return false;
}