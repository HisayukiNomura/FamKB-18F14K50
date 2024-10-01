#include <stdint.h>
#include <string.h>
#include "globaldef.h"
#include "system.h"
#include "usb/usb.h"
#include "usb/usb_device_hid.h"

#include "app_led_usb_status.h"
#include "mcc_generated_files\pin_manager.h"
#include "FamKey.h"


#if defined(__XC8)
    #define PACKED
#else
    #define PACKED __attribute__((packed))
#endif

 #define NO_KEYBORAD_LEDS

/*
 * �჌�x���̃v���g�R��
 * USB�ł́A1ms�P�ʂŁ@Fame�Ƃ����P�ʂŃf�[�^���]������Ă���B���� Frame�́A�ŏ���
 * SOF(Start Of Frame) + �f�[�^�@�Ƃ����P�ʂŒʐM���s����B
 * 
 * �A�v���P�[�V�����̃��x���ł́A���|�[�g�Ƃ����P�ʂŃf�[�^������肷��B���̃��|�[�g�ɂ�
 * Input���|�[�g	�f�o�C�X���z�X�g�@�C���^���v�g�]��
 * Output���|�[�g	�z�X�g���f�o�C�X�@�C���^���v�g�]��	
 * Feature���|�[�g	�f�o�C�X�̃R���t�B�M�����[�V���������w�肷��o�������|�[�g�B�G���h�|�C���g0���g�p
 * �����݂���B
 */


/* HID�N���X�̃f�B�X�N���v�^�i�L�[�{�[�h�j
 * ���̍\���̂́Ausb_device_hid.c��extern����Ďg�p����Ă���B
 * ���̃t�@�C�����ł͎g�p����Ă��Ȃ��B
 * �����炭�Ausb_device_hid.c�́A�ėp�I��HID�N���X���������Ă��邪�A�ėp�I�ȃ��C�u����
 * �Ƃ��Ă�HID�̒��ł̍ו� * �i�}�E�X/�L�[�{�[�h�Ȃǁj��usb_device_hid.c���Œ�`��
 * �邱�Ƃ��ł��Ȃ��B
 * �����ŁA���[�U�[�A�v�����̃t�@�C���Œ�`�����āA�����extern���Ă���Ƃ����\���Ȃ�
 * ���Ǝv���B
 * C++�ł�interface�̂悤�ɁA�K�񂪃w�b�_�ȂǂŒ�`����Ă���΂킩��₷�����A�����Ȃ�
 * extern���������\���̂�����ƕs�C���Ɍ����Ă��܂��B
 */
const struct {
    uint8_t report[HID_RPT01_SIZE];
} hid_rpt01 = {
    {
        0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
        0x09, 0x06,                    // USAGE (Keyboard)
        0xa1, 0x01,                    // COLLECTION (Application)
        0x05, 0x07,                    //   USAGE_PAGE (Keyboard)
        0x19, 0xe0,                    //   USAGE_MINIMUM (Keyboard LeftControl)
        0x29, 0xe7,                    //   USAGE_MAXIMUM (Keyboard Right GUI)
        0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
        0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
        0x75, 0x01,                    //   REPORT_SIZE (1)
        0x95, 0x08,                    //   REPORT_COUNT (8)
        0x81, 0x02,                    //   INPUT (Data,Var,Abs)
        0x95, 0x01,                    //   REPORT_COUNT (1)
        0x75, 0x08,                    //   REPORT_SIZE (8)
        0x81, 0x03,                    //   INPUT (Cnst,Var,Abs)
        0x95, 0x05,                    //   REPORT_COUNT (5)
        0x75, 0x01,                    //   REPORT_SIZE (1)
        0x05, 0x08,                    //   USAGE_PAGE (LEDs)
        0x19, 0x01,                    //   USAGE_MINIMUM (Num Lock)
        0x29, 0x05,                    //   USAGE_MAXIMUM (Kana)
        0x91, 0x02,                    //   OUTPUT (Data,Var,Abs)
        0x95, 0x01,                    //   REPORT_COUNT (1)
        0x75, 0x03,                    //   REPORT_SIZE (3)
        0x91, 0x03,                    //   OUTPUT (Cnst,Var,Abs)
        0x95, 0x06,                    //   REPORT_COUNT (6)
        0x75, 0x08,                    //   REPORT_SIZE (8)
        0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
        0x25, 0x65,                    //   LOGICAL_MAXIMUM (101)
        0x05, 0x07,                    //   USAGE_PAGE (Keyboard)
        0x19, 0x00,                    //   USAGE_MINIMUM (Reserved (no event indicated))
        0x29, 0x65,                    //   USAGE_MAXIMUM (Keyboard Application)
        0x81, 0x00,                    //   INPUT (Data,Ary,Abs)
        0xc0                          // End Collection
    }
};


/*
 *  HID���|�[�g�f�B�X�N���v�^���́Ainput���|�[�g��output���|�[�g�쐬�̂��߂̃^�C�v
 */
typedef struct PACKED
{
    /* Input���|�[�g�̍ŏ��̂P�o�C�g��\������B���̏��̓L�[�{�[�h�̏C���L�[�̒l�ƂȂ�B
     * �ȉ���HID���|�[�g���ڂŌ`������Ă���B
     *
     *  0x19, 0xe0, //   USAGE_MINIMUM (Keyboard LeftControl)
     *  0x29, 0xe7, //   USAGE_MAXIMUM (Keyboard Right GUI)
     *  0x15, 0x00, //   LOGICAL_MINIMUM (0)
     *  0x25, 0x01, //   LOGICAL_MAXIMUM (1)
     *  0x75, 0x01, //   REPORT_SIZE (1)
     *  0x95, 0x08, //   REPORT_COUNT (8)
     *  0x81, 0x02, //   INPUT (Data,Var,Abs)
     *
     * REPORT_SIZE (1)�@�̓G���g�����ƂɂP�r�b�g
     * REPORT_COUNT (8)��8�G���g�����݂���B
     * �����̃G���g���́@Left Control (the usageminimum) �� Right GUI (the usage maximum)�@�̊Ԃ̎g�p���ڂ������Ă���
     */
    union PACKED
    {
        uint8_t value;
        struct PACKED
        {
            unsigned leftControl    :1;
            unsigned leftShift      :1;
            unsigned leftAlt        :1;
            unsigned leftGUI        :1;
            unsigned rightControl   :1;
            unsigned rightShift     :1;
            unsigned rightAlt       :1;
            unsigned rightGUI       :1;
        } bits;
    } modifiers;

    /*�@���̃A�C�e���Ŏg�p�����A�P�o�C�g�̃p�f�B���O
     *  0x95, 0x01,                    //   REPORT_COUNT (1)
     *  0x75, 0x08,                    //   REPORT_SIZE (8)
     *  0x81, 0x03,                    //   INPUT (Cnst,Var,Abs)
     */
    unsigned :8;

    /* INPUT���|�[�g�̍Ō��INPUT���ڂ́Aarray�^�C�v�ł��B ���̔z��́A���݉�����
     * �Ă���L�[�̊e�G���g���[��z��̌��E�܂Ŋ܂�ł��܂��i���̏ꍇ�A�A������6��
     * �̃L�[�����j�B
     *
     *  0x95, 0x06,                    //   REPORT_COUNT (6)
     *  0x75, 0x08,                    //   REPORT_SIZE (8)
     *  0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
     *  0x25, 0x65,                    //   LOGICAL_MAXIMUM (101)
     *  0x05, 0x07,                    //   USAGE_PAGE (Keyboard)
     *  0x19, 0x00,                    //   USAGE_MINIMUM (Reserved (no event indicated))
     *  0x29, 0x65,                    //   USAGE_MAXIMUM (Keyboard Application)
     *
     * ���|�[�g�J�E���g��6�ł���A�z��̑��G���g������6�ł��邱�Ƃ������B
     * ���|�[�g�T�C�Y��8�ŁA�A���C�̊e�G���g���[��1�o�C�g�ł��邱�Ƃ������܂��B
     * �g�p�ŏ��l�́A�ŏ��̃L�[�l(�\��/�C�x���g�Ȃ�)�������܂��B
     * �g�p�ő�l�́A�ō��l�̃L�[�i�A�v���P�[�V�����{�^���j�������܂��B
     * �_���ŏ��l�́A�g�p�ŏ��l�ɑ΂��郊�}�b�v�l�������B
     * �C�x���g�Ȃ��́A�_���l0�Ƃ���B
     * �_���I�ő�l�́A�g�p�ő�l�̃��}�b�v�l�������B
     * �A�v���P�[�V�����{�^���̘_���l��101�ł��B
     * 
     * ���̏ꍇ�A�_���I�ȍŏ��l/�ő�l�͎g�p���̍ŏ��l/�ő�l�ƈ�v����̂ŁA�_���I
     * �ȍă}�b�s���O�͎��ۂɂ͒l��ύX���Ȃ��B
     * 
     * �ua�v�L�[�������ă��|�[�g�𑗐M����ꍇ�i�g�p�@�̒l��0x04�A���̗�̘_���l��
     * 0x04�j�A�z����͎͂��̂悤�ɂȂ�܂��B
     * LSB [0x04][0x00][0x00][0x00][0x00][0x00] MSB
     *
     * ���̂܂܁ub�v�������A�ua�v�ub�v�������ɉ����ꂽ�ꍇ�A���̂悤�ɂȂ�B
     * the report would then look like this:
     *
     * LSB [0x04][0x05][0x00][0x00][0x00][0x00] MSB
     *
     * ���̌�A�ua�v�{�^���������ꂽ���ub�v�͉������܂܁A�Ƃ����ꍇ�A���̂悤�ɂȂ�B
     *
     * LSB [0x05][0x00][0x00][0x00][0x00][0x00] MSB
     *
     * �ua�v���������ƁA�z��̂��ׂẴA�C�e���̓[���ɂȂ�B
     */
    uint8_t keys[6];
} KEYBOARD_INPUT_REPORT;


/* 
 *  HID���|�[�g�f�B�X�N���v�^���́Aoutput���|�[�g��output���|�[�g�쐬�̂��߂̃^�C�v
*/
typedef union PACKED
{
    /* Output���|�[�g�͂P�o�C�g�̃f�[�^�����B�L�[�{�[�h�̃����v */
    uint8_t value;
    struct
    {
        /* 5��LED�C���W�P�[�^�Ɋւ�����
         *
         *  0x95, 0x05,                    //   REPORT_COUNT (5)
         *  0x75, 0x01,                    //   REPORT_SIZE (1)
         *  0x05, 0x08,                    //   USAGE_PAGE (LEDs)
         *  0x19, 0x01,                    //   USAGE_MINIMUM (Num Lock)
         *  0x29, 0x05,                    //   USAGE_MAXIMUM (Kana)
         *  0x91, 0x02,                    //   OUTPUT (Data,Var,Abs)
         *
         * REPORT_SIZE (1)�́A���ꂼ��̃G���g���[���P�r�b�g
         * REPORT_COUNT (5)�́A5�r�b�g��LED���B
         * �����̃A�C�e����LED usage�y�[�W�ɑ��݂���B
         * These items are all of the usages between Num Lock (the usage
         * minimum) and Kana (the usage maximum).
         */
        unsigned numLock        :1;
        unsigned capsLock       :1;
        unsigned scrollLock     :1;
        unsigned compose        :1;
        unsigned kana           :1;

        /* �o�C�g�ɂ��邽�߂�3�r�b�g�̒萔�̈�B
         *
         *  0x95, 0x01,                    //   REPORT_COUNT (1)
         *  0x75, 0x03,                    //   REPORT_SIZE (3)
         *  0x91, 0x03,                    //   OUTPUT (Cnst,Var,Abs)
         *
         *  REPORT_COUNT (1)�͂P�G���g���AREPORT_SIZE (3)�͂R�r�b�g
         * Report size of 3 indicates the entry is 3 bits long. */
        unsigned                :3;
    } leds;
} KEYBOARD_OUTPUT_REPORT;


/* 
 * �L�[�{�[�h�̌��݂̏�Ԃ��Ǘ����邽�߂̃e�[�u���B���Ƃ��Ƃ̃T���v���ł́A������
 * �L�[�̃��s�[�g��}������r�b�g�����������A���ۂ̃L�[�{�[�h�ł̓L�[���s�[�g�͋@�\
 * �Ƃ��ĕK�v�Ȃ̂ō폜�����B
 */
typedef struct
{
    USB_HANDLE lastINTransmission;
    USB_HANDLE lastOUTTransmission;
    unsigned char key;                  // ?
} KEYBOARD;
static KEYBOARD keyboard;


#if !defined(KEYBOARD_INPUT_REPORT_DATA_BUFFER_ADDRESS_TAG)
    #define KEYBOARD_INPUT_REPORT_DATA_BUFFER_ADDRESS_TAG
#endif
static KEYBOARD_INPUT_REPORT inputReport KEYBOARD_INPUT_REPORT_DATA_BUFFER_ADDRESS_TAG;

#if !defined(KEYBOARD_OUTPUT_REPORT_DATA_BUFFER_ADDRESS_TAG)
    #define KEYBOARD_OUTPUT_REPORT_DATA_BUFFER_ADDRESS_TAG
#endif
static volatile KEYBOARD_OUTPUT_REPORT outputReport KEYBOARD_OUTPUT_REPORT_DATA_BUFFER_ADDRESS_TAG;


static void APP_KeyboardProcessOutputReport(void);


//Exteranl variables declared in other .c files
// SOFCounter�́Ausb_events.c�Œ�`����Ă���B
// USER_USB_CALLBACK_EVENT_HANDLER(USB_EVENT event, void *pdata, uint16_t size)
// �̃R�[���o�b�N�ŕύX�����B
// SOF �iStart Of Frame�j�́A1ms�Ԋu�ő��M�����̂ŁA������J�E���g���Ď��ԑ���Ɏg�p����B
extern volatile signed int SOFCounter;


//�@�A�C�h�����֌W�̕ϐ�

KEYBOARD_INPUT_REPORT oldInputReport;           // �O�񑗐M����InputReport��ۑ����Ă����B�A�C�h�����Ŏw�肳�ꂽ���Ԃ𒴂�����A�O��Ɠ������N�G�X�g���đ�����B
signed int keyboardIdleRate;                    // �A�C�h�����B�f�t�H���g��500(ms)�����A�z�X�g��SET_IDLE�𑗂�Ə�����������B
signed int LocalSOFCount;                       // ���݂�SOF�̃J�E���g
static signed int OldSOFCount;                  // �O��AInput Report�𑗐M�����Ƃ���SOF�̒l


/*
 * �L�[�{�[�h�̏�����
 */
void APP_KeyboardInit(void)
{
    //�@�Ō�ɒʐM�������̃n���h��������������
    keyboard.lastINTransmission = 0;
    
    keyboard.key = 4;

    // �f�t�H���g�̃A�C�h�����[�g��500ms�ɐݒ�B (�z�X�g�� SET_IDLE�@�𑗐M���Ēl��ύX����)
    keyboardIdleRate = 500;

    // ���荞�݃R���e�L�X�gSOFCounter�̒l�����[�J���ϐ��ɃR�s�[����B
    // SOFCounter�͕K�������A�g�~�b�N�ł͂Ȃ��̂ŁAwhile()���[�v���g�p���čŒ�2��ǂ�
    // ����Œl�������ɂȂ��Ă邱�Ƃ����m�F����K�v������B
    while(OldSOFCount != SOFCounter) {
        OldSOFCount = SOFCounter;
    }

    //�@HID�̃G���h�|�C���g��L���ɂ���
    USBEnableEndpoint(HID_EP, USB_IN_ENABLED|USB_OUT_ENABLED|USB_HANDSHAKE_ENABLED|USB_DISALLOW_SETUP);

    //�@�z�X�g����CapsLock��NumLock�����󂯎���悤��OUT�G���h�|�C���g��ݒ肷��B
    // �����Őݒ肷��̂́Aoutput���|�[�g�̍\���̂̎��ԁB
    keyboard.lastOUTTransmission = HIDRxPacket(HID_EP,(uint8_t*)&outputReport, sizeof(outputReport) );
}

/*
 * �L�[�{�[�h�̃^�X�N�B
 * �����ł́A���̏������s����B
 * 1. USB�f�o�C�X���ڑ�����āA�ʐM�\�ɂȂ��Ă��Ȃ���Ή������Ȃ��B
 * 2. �O���SOF�ƁA�����SOF���r���ăA�C�h�����Ԃ��v�Z����B
 * 3. Input Report�̃f�[�^���\������B( KEYBOARD_INPUT_REPORT�@�Œ�`�����\����)
 * 4. �쐬���ꂽ Input Report�̍\�����A�O�񑗂������̂Ɠ������ǂ����𒲂ׂ�
 * 5. ���̏ꍇ�AInput Report�𑗐M����B
 * �@ a) ���M����f�[�^���A�O�񑗂������̂ƈقȂ�ꍇ�A
 *    b) ���M����f�[�^���O��Ɠ����ł��A����̃A�C�h�����Ԃ𒴂����ꍇ�i�O��Ɠ������̂𑗐M�j
 * �@�@�@����̓L�[���s�[�g�̂��߁H
 */
void APP_KeyboardTasks(void)
{
    
    signed int TimeDeltaMilliseconds;
    unsigned char i;
    bool needToSendNewReportPacket;         // Input ���|�[�g�𑗐M���ׂ����������ϐ�

    /* 
     * USB�f�o�C�X���܂��ݒ肳��Ă��Ȃ��ꍇ�A�ʐM����z�X�g���Ȃ����߉��������ɖ߂�
     */
    if( USBGetDeviceState() < CONFIGURED_STATE ) {
        return;
    }
 
    /* �����A���݃T�X�y���h���ł���΁A�����[�g�E�F�C�N�A�b�v�𔭍s����K�v�����邩
     * �ǂ������m�F����K�v������܂��B �ǂ���̏ꍇ�ł��A���݃z�X�g�ƒʐM���Ă���
     * ���̂ŁA�L�[�{�[�h�R�}���h���������ׂ��ł͂���܂���B�Ȃɂ������ɖ߂�܂��B
    */

    if( USBIsDeviceSuspended()== true ) {
        // ���[�U�[���v�b�V���{�^�����������Ƃ��ɁAUSB�z�X�g�ɑ΂��ă����[�g�E�F�C
        // �N�A�b�v�v�����A�T�[�g���ׂ����ǂ������`�F�b�N���܂��B
        
        //�@�Ƃ肠�����̓^�N�g�X�C�b�`�ɂ��Ă������ǁA�����̓L�[�{�[�h�̃{�^���Ȃ̂���
        if( isTestButton()) {
            //Add code here to issue a resume signal.
        }

        return;
    }
    
    // ���荞�݃R���e�L�X�gSOFCounter�̒l�����[�J���ϐ��ɃR�s�[���܂��B
    // SOFCounter�͕K�������A�g�~�b�N�ł͂Ȃ��̂ŁAwhile()���[�v���g�p���Ă�����s
    // ���܂��B�X�V����A�������l���擾�������Ƃ��m�F���邽�߂ɁA�Œ�2��ǂݍ��ޕK�v
    // ������܂�
    while(LocalSOFCount != SOFCounter) {
        LocalSOFCount = SOFCounter;
    }

    // �Ō��input ���|�[�g�����M����Ă���̌o�ߎ��Ԃ��v�Z����B �z�X�g�ɂ���Đݒ肳
    // �ꂽHID�A�C�h�����[�g�ɓK�؂ɏ]�����߂ɁA���̏��𓾂邱�Ƃ��ł��܂�)
    TimeDeltaMilliseconds = LocalSOFCount - OldSOFCount;

    // �J�E���g���[���ɖ߂郉�b�v�A���E���h�ɂ�镉�̒l���`�F�b�N���܂��B
    if(TimeDeltaMilliseconds < 0) {
        TimeDeltaMilliseconds = (32767 - OldSOFCount) + LocalSOFCount;
    }
    //TimeDelay�����Ȃ�傫�����ǂ������`�F�b�N���܂��B �A�C�h�����[�g��== 0
    // �i�u������v��\���j�̏ꍇ�A�L�[�{�[�h�ōŋ߃{�^�����������Ȃǂ̕ω�����
    // ����΁ATimeDeltaMilliseconds��������ɂȂ�i�I�[�o�[�t���[�������N�����j
    // �\��������܂��B
    // ���������āATimeDeltaMilliseconds���傫���Ȃ肷������A���Ƃ��ŋߎ��ۂ�
    // �p�P�b�g�𑗐M���Ă��Ȃ��Ă� OldSOFCount���X�V���ĖO�a������
    // 
    if(TimeDeltaMilliseconds > 5000) {
        OldSOFCount = LocalSOFCount - 5000;
    }

  
    // IN�G���h�|�C���g���r�W�[��Ԃ��ǂ������`�F�b�N���A�r�W�[��ԂłȂ��ꍇ�́A
    // �L�[�X�g���[�N�E�f�[�^���z�X�g�ɑ��M���邩�ǂ������`�F�b�N���܂��B
    
    if(HIDTxHandleBusy(keyboard.lastINTransmission) == false) {
        // INPUT���|�[�g�o�b�t�@���N���A���܂��B ���ׂă[���ɐݒ肵�܂��B
        memset(&inputReport, 0, sizeof(inputReport));

        // �����ŃL�[�{�[�h�̃f�[�^�𑗐M
        int keyCnt = 0;         // ���ő��M���鉟���ꂽ�L�[�̐��i�f�o�b�O�p�j

        //if (KeyRepIdx != 0) IO_RC7_SetHigh(); else IO_RC7_SetLow();
         
        if (KeyRepIdx != 0) {
            // �A�g���r���[�g�L�[������            
            inputReport.modifiers.bits.leftControl = (modifier  & ATBKEY_LEFTCTRL)  ? 1 : 0;
            inputReport.modifiers.bits.leftShift = (modifier & ATBKEY_LEFTSHIFT)  ? 1 : 0;
            inputReport.modifiers.bits.leftAlt = (modifier& ATBKEY_LEFTALT)  ? 1 : 0;
            inputReport.modifiers.bits.leftGUI = (modifier& ATBKEY_LEFTGUI)  ? 1 : 0;
            inputReport.modifiers.bits.rightControl = (modifier & ATBKEY_RIGHTCTRL)  ? 1 : 0;
            inputReport.modifiers.bits.rightShift = (modifier & ATBKEY_RIGHTSHIFT)  ? 1 : 0;
            inputReport.modifiers.bits.rightAlt = (modifier & ATBKEY_RIGHTALT)  ? 1 : 0;
            inputReport.modifiers.bits.rightGUI = (modifier & ATBKEY_RIGHTGUI)  ? 1 : 0;
            
            // ��ʃL�[�̏���
            for (int i = 0 ; i < 6;i++) {               
                if ( KeyReport[i] !=0) {
                    inputReport.keys[i] = KeyReport[i];
                    keyCnt++;
                }
            }
        }
        
        //�V�����p�P�b�g���e���A�ŋߑ��M���ꂽ�p�P�b�g���e�Ɖ��炩�̈Ⴂ�����邩�ǂ������m�F����B
        needToSendNewReportPacket = false;
        for(i = 0; i < sizeof(inputReport); i++) {
            if(*((uint8_t*)&oldInputReport + i) != *((uint8_t*)&inputReport + i)) {
                needToSendNewReportPacket = true;           // Input���|�[�g�͑��M����K�v������
                break;
            }
        }
        
        
        // �z�X�g���A�C�h�����[�g��0�ȊO�i������u�����v�j�ɐݒ肵�Ă��邩�ǂ�����
        // �m�F����B�A�C�h�����[�g�������łȂ��ꍇ�A�Ō�̃p�P�b�g�𑗐M���Ă���
        // �\���Ȏ��Ԃ��o�߂��A�V���������p�P�b�g�𑗐M���鎞�Ԃł��邩�ǂ�����
        // �m�F����B
        if(keyboardIdleRate != 0) {
            // �A�C�h�����[�g���Ԑ����𖞂����Ă��邩�ǂ������m�F����B ���������Ȃ�A�ʂ�HID���̓��|�[�g�p�P�b�g���z�X�g�ɑ��M����K�v������
            if(TimeDeltaMilliseconds >= keyboardIdleRate) {
                needToSendNewReportPacket = true;        // Input���|�[�g�͑��M����K�v������
            }
        }

        // �����ŁA�V�������̓��|�[�g�p�P�b�g�𑗐M���邱�Ƃ��K�؂ł���ꍇ�A����𑗐M���܂��B
        // (��: �V�����f�[�^�����݂���A�܂��̓A�C�h�����[�g�̐����ɒB����)
        if(needToSendNewReportPacket == true) {         // Input���|�[�g�𑗐M����K�v������Ȃ�
            //�Â����̓��|�[�g�p�P�b�g�̓��e��ۑ����܂��B ����́A�����A�C�h��
            // ���[�g�ݒ���g�p���Ă���Ƃ��ɁA�������ύX����A�z�X�g�ɍđ��M����
            // �K�v������ꍇ�𔻒f����̂ɖ𗧂��|�[�g�p�P�b�g�̓��e�̕ύX�����o�ł���悤�ɂ��邽�߂ł��B
            oldInputReport = inputReport;

            // �W�o�C�g�̃p�P�b�g�iInput ���|�[�g�j��PC�ɑ��M���� 
            // KEYBOARD_INPUT_REPORT�^�ŁA�C���L�[(1)+1�o�C�g�̖��ߑ�+6�o�C�g�̓��̓L�[
            keyboard.lastINTransmission = HIDTxPacket(HID_EP, (uint8_t*)&inputReport, sizeof(inputReport));
            OldSOFCount = LocalSOFCount;    //Save the current time, so we know when to send the next packet (which depends in part on the idle rate setting)
        }

    }

    /* PC����L�[�{�[�h�f�o�C�X�ɉ��炩�̃f�[�^�����M���ꂽ���ǂ������m�F���܂��B 
     * ���|�[�g�f�B�X�N���v�^�ɂ��A�z�X�g��1�o�C�g�̃f�[�^�𑗐M���邱�Ƃ��ł��܂��B 
     * �r�b�g0-4��LED�̏�ԁA�r�b�g5-7�͖��g�p�̃p�b�h�r�b�g�ł��B �z�X�g�͂���OUT
     * ���|�[�g�f�[�^��HID OUT�G���h�|�C���g�iEP1 OUT�j��ʂ��đ��M���邱�Ƃ��\��
     * �����A���̑���ɁA�z�X�g��EP0���SET_REPORT����]���𑗐M���邱�Ƃɂ����
     * LED��ԏ��𑗐M���悤�Ƃ��邱�Ƃ��\�ł��B USBHIDCBSetReportHandler()�֐�
     * ���Q�Ƃ��Ă��������B 
     */
    if(HIDRxHandleBusy(keyboard.lastOUTTransmission) == false) {
        APP_KeyboardProcessOutputReport();
        keyboard.lastOUTTransmission = HIDRxPacket(HID_EP,(uint8_t*)&outputReport,sizeof(outputReport));
    }
    return;		
}


/*
 * ���|�[�g�ɏ�����CAPSLOCK�����v�𐧌䂷��BRC5���g�����B
 */
static void APP_KeyboardProcessOutputReport(void)
{
     //IO_RC5_SetHigh();
#ifdef USE_KEYBORAD_LED
    if(outputReport.leds.capsLock) {
        IO_RC5_SetHigh();
    } else {
        IO_RC5_SetLow();
    }
    if(outputReport.leds.numLock) {
        IO_RC7_SetHigh();
    } else {
        IO_RC7_SetLow();
    }
#ifndef USE_USBLED
    if(outputReport.leds.scrollLock) {
        IO_RC6_SetHigh();
    } else {
        IO_RC6_SetLow();
    }        
#endif    
#endif
     //IO_RC5_SetLow();    
}

/*
 �z�X�g����ALED�̏�ԃf�[�^������(SET_REPORT)�����̂ŁA�����output report buffer�ɃR�s�[���ď�������B
 */
static void USBHIDCBSetReportComplete(void)
{
    /* 1 byte of LED state data should now be in the CtrlTrfData buffer.  Copy
     * it to the OUTPUT report buffer for processing */
    outputReport.value = CtrlTrfData[0];

    /* Process the OUTPUT report. */
    APP_KeyboardProcessOutputReport();
}

/* �G���h�|�C���g0��SET_REPORT����]���ŃL�[�{�[�hLED�̏�ԃf�[�^����M���鏀��������B
   �z�X�g�́A���|�[�g�f�B�X�N���v�^�����M�ł���̂͂��ꂾ���Ȃ̂ŁA 1�o�C�g�������M����B
 �@SET_REPORT�� �z�X�g���t�@���N�V�����E�f�o�C�X�Ƀf�[�^�𑗐M���邽�߂̃��N�G�X�g�B
  �t�B�[���h       �T�C�Y �ݒ�l
  bmRequestType     1 ���N�G�X�g�E�^�C�v�F0x21
  bRequest          1 ���N�G�X�g���ʎq�F0x09�iGet_Report�j
  wValue            2 ��� 1 �o�C�g�F���|�[�g�̎�ށC���� 1 �o�C�g�F���|�[�g ID
  wIndex            2 �����Ώۂ̃C���^�t�F�[�X�ԍ�
  wLength           2 ���|�[�g��
*/
void USBHIDCBSetReportHandler(void)
{
    USBEP0Receive((uint8_t*)&CtrlTrfData, USB_EP0_BUFF_SIZE, USBHIDCBSetReportComplete);
}


// �z�X�g��SET_ILDE�R�}���h�𑗂��Ă����Ƃ���USB�X�^�b�N�ɌĂяo�����R�[���o�b�N�B
// SET_IDLE�̓z�X�g���t�@���N�V�����E�f�o�C�X�̃A�C�h������ݒ肷�邽�߂̃��N�G�X�g�B
// �L�[�{�[�h�E�f�o�C�X�́C���̃��N�G�X�g���T�|�[�g����K�v������B
// �t�B�[���h    �T�C�Y �ݒ�l
// bmRequestType  1 ���N�G�X�g�E�^�C�v�F0x21
// bRequest       1 ���N�G�X�g���ʎq�F0x0A�iSet_Idle�j
// wValue         2 ��� 1 �o�C�g�F���|�[�g�Ԃ̍ő厞�ԊԊu�i4 ms �P�ʁj�i0 �̏ꍇ�C�ω���������
//                  �ꍇ�̂݃��|�[�g��Ԃ��C�ω����Ȃ��Ƃ��� NAK �������܂��j
//                  ���� 1 �o�C�g�F���|�[�g ID�i0 �̏ꍇ�C���ׂĂ̓��̓��|�[�g�ɓK�p���܂��j
// wIndex         2 �����Ώۂ̃C���^�t�F�[�X�ԍ�
// wLength        2 0x0000 
void USBHIDCBSetIdleRateHandler(uint8_t reportID, uint8_t newIdleRate)
{
    // ���|�[�gID���L�[�{�[�h���͂̃��|�[�gID�ԍ��ƈ�v���邱�Ƃ��m�F����B
    // �������A�t�@�[���E�F�A�����|�[�gID�ԍ�������/�g�p���Ă��Ȃ��ꍇ�́A== 0�Ƃ��邱�ƁB
        
    if(reportID == 0) {
        keyboardIdleRate = newIdleRate;
    }
}

