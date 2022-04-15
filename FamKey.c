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
 * �L�[�{�[�h����l��ǂݍ���ŁA�O���[�o���ϐ�KeyReport�ɃL�[�R�[�h�Amodifier�ɁA
 * �C���L�[�iSHIFT�Ƃ��j�̉�����Ԃ�ݒ肷��
 */
uint8_t modifier;
uint8_t KeyReport[6];       // ������Ă���L�[�̏��
uint8_t KeyRepIdx;          // �i�[����Ă���L�[�R�[�h�̐�



uint8_t KeyBuffer[9];           // �L�[�}�g���b�N�X�̏�Ԃ������ϐ�

/*
 * USB�L�[�{�[�h�̃L�[�R�[�h�\�ƁA�t�@�~���[�x�[�V�b�N�L�[�{�[�h�̃}�g���b�N�X�Ή�
 *  http://www2d.biglobe.ne.jp/~msyk/keyboard/layout/usbkeycode.html
 * �@�@���@USB�L�[�R�[�h�\
 *  http://www43.tok2.com/home/cmpslv/Famic/Fambas.htm�@
 * �@�@���@�L�[�{�[�h�̃}�g���b�N�X
 * �@
 * ���̔z��́A�L�[�{�[�h�}�g���b�N�X�i8x9�j�̑傫���ŁA���ꂼ���USB�L�[�{�[�h�̃R�[�h
 * �������Ă���B0xFF�́A�L�[�R�[�h�𔭐������Ȃ��B
 */

// �W����Ԃ̃}�g���b�N�X�Ή��B
// ��{�I�ɂ́A�L�[�g�b�v�ƑΉ������L�[�Ɋ��蓖�Ă����A�ꕔ�ύX�������Ă���B�l�I��
// �K�{�Ǝv����L�[�����݂��Ȃ����߁B
// �E���ȃL�[�́A���݂��Ȃ��L�[��\�����邽�߂̏C���L�[�Ƃ��đ��삳���悤�ɂ����̂ŁA
//   �P�̂ŉ����Ă��L�[�R�[�h�͔������Ȃ��B
// �EESC(29)�L�[�́ATAB(2B)�ɂ��Ă���B���ȁ{ESC��ESC�����������悤�ɂȂ��Ă���B
//   �L�[�g�b�v�Ƌt�����BQ�̉��ɂ�TAB���~�������ATAB�̂����ESC�������ƃV���b�N���傫���̂�
// �ECLR��CAPSLock�B�L�[�g�b�v��CLR/HOME �Ȃ̂ŁAHome�Ɋ��蓖�Ă悤�Ǝv�������AEND��
//   �y�A�Ŋ��蓖�Ă����B�����ŁA���ȁ{���A���ȁ{�E��HOME/END�ɂ����B�]����CLR��CapsLock�ɁB
// �E���̑��̃}�b�s���O�ύX
//    GRP����ALT�ASTOP��BS
uint8_t *KeyUsagePtr;
uint8_t key_usage[72] = {
  0x41,0x28,0x30,0x32,0x35,0xff,0x89,0x2a, // F8  RET   [   ]   FUNC  R-SFT �� STOP
  0x40,0x2f,0x34,0x33,0x87,0x38,0x2d,0x2e, // F7   ��   ��  ��    ��    ��  ��   ��
  0x3f,0x12,0x0f,0x0e,0x37,0x36,0x13,0x27, // F6   o    l   k    ,     .    p   0
  0x3e,0x0c,0x18,0x0d,0x10,0x11,0x26,0x25, // F5   i    u   j    m     n    9   8
  0x3d,0x1c,0x0a,0x0b,0x05,0x19,0x24,0x23, // F4   y    g   h    b     v    7   6
  0x3c,0x17,0x15,0x07,0x09,0x06,0x22,0x21, // F3   t    r   d    f     c    5   4
  0x3b,0x1a,0x16,0x04,0x1b,0x1d,0x08,0x20, // F2   w    s   a    x     z    e   3
  0x3a,0x2B,0x14,0xff,0xff,0xff,0x1e,0x1f, // F1 ESC    Q  CTR R-SFT  GRPH  1  2
  0x39,0x52,0x4f,0x50,0x51,0x2c,0x4c,0x49 // CLR ��   ��   ��   ��   SPC  DEL INS
};
//
//���Ȃ������Ȃ���́A�Ⴄ�R�[�h�𐶐�������
// F1-F4��F9-F12�@ F5-F7��PRSC-PAUSE�@�@STOP������
// ESC��ESC �とPageUp ����PageDWN ����HOME�@�E��END
// CLR�����ϊ��@INS���ϊ��@DEL������
// z �� capslock X��Win  �����A�v���P�[�V����
uint8_t key_usage_EXPKey[72] = {
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x35, // �~�@ �~   �~   �~   FUNC  R-SFT�~ ����
  0x48,0xff,0xff,0xff,0xff,0x65,0xff,0xff, // PUSE �~   �~   �~   APL   �~   �~  �~
  0x47,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // SLCK �~   �~   �~   �~    �~   �~  �~
  0x46,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // PRSC �~   �~   �~   �~    �~   �~  �~
  0x45,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // F12  �~   �~   �~   �~    �~   �~  �~
  0x44,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // F11  �~   �~   �~   �~    �~   �~  �~
  0x43,0xff,0xff,0xff,0xe3,0x39,0xff,0xff, // F10  �~   �~   �~  Win    CAPS �~  �~
  0x42,0x29,0xff,0xff,0xff,0xff,0xff,0xff, // F9   TAB  �~�@ CTR L-SFT  GRPH �~  �~
  0x8b,0x4b,0x4d,0x4a,0x4e,0x2c,0x8a,0x88  // CLR  PU   END  HOME PDN   SPC  ���� �ϊ�
};
#define DELAY_LONG  __delay_us(50);
#define DELAY_SHORT __delay_us(10); 



/*
 * �L�[�{�[�h�̃}�g���b�N�X��ϊ�����.
 *   KeyBuffer uint8_t[9]�@�̃|�C���^�B�@�����ꂽ�L�[�̃}�g���b�N�X���i�[����Ă���B
 * �@pModifier ������Ă���C���L�[�̃}�X�N
 *   keyCodes�@������Ă���L�[�̃L�[�R�[�h
 * KeyBuffer�Ɏw�肳��Ă��鉟���ꂽ�L�[�}�g���b�N�X�����ƂɁApModifier��KeyCodes�𖄂߂Ă����B
 * 
 * 
 */
void ProcessMatrix (uint8_t* Buf , uint8_t* pModifier , uint8_t* KeyCodes)
{
    uint8_t mod = 0;
    //
    // �C���L�[�̏���
    //
    if ((Buf[7] & 0x08) != 0) {       // CTRL����CTRL
        mod |= ATBKEY_LEFTCTRL;
	}
  	if ((Buf[7] & 0x10) != 0) {       // ��Shift����Shift
        mod |=ATBKEY_LEFTSHIFT;
    }
    if ((Buf[0] & 0x20) != 0) {       // �ESHIFT���EShift
        mod |= ATBKEY_RIGHTSHIFT;
    }
    if ((Buf[7] & 0x20) != 0) {       // GRAPH����ALT
        mod |= ATBKEY_LEFTALT;
    }
    
    if ((Buf[0] & 0x10) != 0) {       // �J�i�L�[�ƏC���L�[�̓�����������������B
        if (mod & ATBKEY_LEFTCTRL) {        // ���ȁ{CTRL�ŁA�ECTRL�Ƃ���
            mod &= ~ATBKEY_LEFTCTRL;
            mod |= ATBKEY_RIGHTCTRL;
        }
        if (mod & ATBKEY_LEFTALT) {        // ���ȁ{ALT�ŁA�EALT�Ƃ���
            mod &= ~ATBKEY_LEFTALT;
            mod |= ATBKEY_RIGHTALT;
        }
        
    }
     
    *pModifier = mod;

    //
    // �J�i�L�[���A�g���r���[�g�Ƃ��Ďg���悤�ɂ���B
    // ���ȃL�[��������Ă�����A�Ⴄ�L�[�𐶐�����B
    //
	if ((Buf[0] & 0x10) != 0) {       // �J�i�L�[��������A�e�[�u����u�������Ă��ȃL�[�͐H���Ă��܂�
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
// ��ʃf�[�^�̃Z���N�g
inline void  KBSetHigh()
{
    IO_DATASEL_RB6_SetHigh();        // ��ʂ̃f�[�^�Z���N�g
    DELAY_LONG
    
}
// ���ʃf�[�^�̃Z���N�g
inline void  KBSetLow()
{
    IO_DATASEL_RB6_SetLow();        // ���ʂ̃f�[�^�Z���N�g
    DELAY_LONG
}





void ReadKey(void)
{
    // �f�o�C�X�Z���N�g
    //    IO_DEVSEL_RB7_SetHigh();
    __delay_us(200);
    // ���Z�b�g
    KBReset();

    
    // �L�[�}�g���b�N�X���牟����Ă���L�[���擾����
    for (int i = 0 ; i < 9 ; i++) {
        uint8_t wkBits = 0;
        wkBits = (PORTC & 0b00001111); 

        KBSetHigh();

        wkBits |= ((PORTC << 4) & 0b11110000);
        KeyBuffer[i] = wkBits;
        KBSetLow();
        /*
        if (i==1) {
           //if (wkBits & 0b00000001) IO_RC5_SetHigh(); else  IO_RC5_SetLow(); // ��   F8    F7
           //if (wkBits & 0b00000010) IO_RC6_SetHigh(); else  IO_RC6_SetLow(); // ��   RET   ��
           //if (wkBits & 0b00000100) IO_RC7_SetHigh(); else  IO_RC7_SetLow(); // ��    [  �@��
           //if (wkBits & 0b00001000) IO_RC5_SetHigh(); else  IO_RC5_SetLow(); // ��   ]   �@��
           //if (wkBits & 0b00010000) IO_RC6_SetHigh(); else  IO_RC6_SetLow(); // ��  ����   ��
           //if (wkBits & 0b00100000) IO_RC7_SetHigh(); else  IO_RC7_SetLow(); // �� R-SFT   ��
           //if (wkBits & 0b01000000) IO_RC5_SetHigh(); else  IO_RC5_SetLow(); // ��   \     ��
           //if (wkBits & 0b10000000) IO_RC6_SetHigh(); else  IO_RC6_SetLow(); // ��  STOP�@ ��
        }
        */
    }
    
    // �ǂݍ��񂾃L�[��������Ă���L�[�ԍ��ɂ��Ă���
   
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