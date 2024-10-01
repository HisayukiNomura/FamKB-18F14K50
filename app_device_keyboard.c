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
 * 低レベルのプロトコル
 * USBでは、1ms単位で　Fameという単位でデータが転送されている。この Frameは、最初に
 * SOF(Start Of Frame) + データ　という単位で通信が行われる。
 * 
 * アプリケーションのレベルでは、レポートという単位でデータをやり取りする。このレポートには
 * Inputレポート	デバイス→ホスト　インタラプト転送
 * Outputレポート	ホスト→デバイス　インタラプト転送	
 * Featureレポート	デバイスのコンフィギュレーション情報を指定する双方向レポート。エンドポイント0を使用
 * が存在する。
 */


/* HIDクラスのディスクリプタ（キーボード）
 * この構造体は、usb_device_hid.cにexternされて使用されている。
 * このファイル内では使用されていない。
 * おそらく、usb_device_hid.cは、汎用的なHIDクラスを実装しているが、汎用的なライブラリ
 * としてはHIDの中での細分 * （マウス/キーボードなど）をusb_device_hid.c内で定義す
 * ることができない。
 * そこで、ユーザーアプリ側のファイルで定義させて、これをexternしてくるという構成なん
 * だと思う。
 * C++でのinterfaceのように、規約がヘッダなどで定義されていればわかりやすいが、いきなり
 * externだけされる構造体があると不気味に見えてしまう。
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
 *  HIDレポートディスクリプタ中の、inputレポートとoutputレポート作成のためのタイプ
 */
typedef struct PACKED
{
    /* Inputレポートの最初の１バイトを表現する。この情報はキーボードの修飾キーの値となる。
     * 以下のHIDレポート項目で形成されている。
     *
     *  0x19, 0xe0, //   USAGE_MINIMUM (Keyboard LeftControl)
     *  0x29, 0xe7, //   USAGE_MAXIMUM (Keyboard Right GUI)
     *  0x15, 0x00, //   LOGICAL_MINIMUM (0)
     *  0x25, 0x01, //   LOGICAL_MAXIMUM (1)
     *  0x75, 0x01, //   REPORT_SIZE (1)
     *  0x95, 0x08, //   REPORT_COUNT (8)
     *  0x81, 0x02, //   INPUT (Data,Var,Abs)
     *
     * REPORT_SIZE (1)　はエントリごとに１ビット
     * REPORT_COUNT (8)は8エントリ存在する。
     * これらのエントリは　Left Control (the usageminimum) と Right GUI (the usage maximum)　の間の使用項目を示している
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

    /*　次のアイテムで使用される、１バイトのパディング
     *  0x95, 0x01,                    //   REPORT_COUNT (1)
     *  0x75, 0x08,                    //   REPORT_SIZE (8)
     *  0x81, 0x03,                    //   INPUT (Cnst,Var,Abs)
     */
    unsigned :8;

    /* INPUTレポートの最後のINPUT項目は、arrayタイプです。 この配列は、現在押され
     * ているキーの各エントリーを配列の限界まで含んでいます（この場合、連続した6つ
     * のキー押下）。
     *
     *  0x95, 0x06,                    //   REPORT_COUNT (6)
     *  0x75, 0x08,                    //   REPORT_SIZE (8)
     *  0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
     *  0x25, 0x65,                    //   LOGICAL_MAXIMUM (101)
     *  0x05, 0x07,                    //   USAGE_PAGE (Keyboard)
     *  0x19, 0x00,                    //   USAGE_MINIMUM (Reserved (no event indicated))
     *  0x29, 0x65,                    //   USAGE_MAXIMUM (Keyboard Application)
     *
     * レポートカウントは6であり、配列の総エントリ数が6であることを示す。
     * レポートサイズは8で、アレイの各エントリーが1バイトであることを示します。
     * 使用最小値は、最小のキー値(予約/イベントなし)を示します。
     * 使用最大値は、最高値のキー（アプリケーションボタン）を示します。
     * 論理最小値は、使用最小値に対するリマップ値を示す。
     * イベントなしは、論理値0とする。
     * 論理的最大値は、使用最大値のリマップ値を示す。
     * アプリケーションボタンの論理値は101です。
     * 
     * この場合、論理的な最小値/最大値は使用時の最小値/最大値と一致するので、論理的
     * な再マッピングは実際には値を変更しない。
     * 
     * 「a」キーを押してレポートを送信する場合（使用法の値は0x04、この例の論理値も
     * 0x04）、配列入力は次のようになります。
     * LSB [0x04][0x00][0x00][0x00][0x00][0x00] MSB
     *
     * そのまま「b」を押し、「a」「b」が同時に押された場合、次のようになる。
     * the report would then look like this:
     *
     * LSB [0x04][0x05][0x00][0x00][0x00][0x00] MSB
     *
     * その後、「a」ボタンが離されたが「b」は押したまま、という場合、次のようになる。
     *
     * LSB [0x05][0x00][0x00][0x00][0x00][0x00] MSB
     *
     * 「a」が離されると、配列のすべてのアイテムはゼロになる。
     */
    uint8_t keys[6];
} KEYBOARD_INPUT_REPORT;


/* 
 *  HIDレポートディスクリプタ中の、outputレポートとoutputレポート作成のためのタイプ
*/
typedef union PACKED
{
    /* Outputレポートは１バイトのデータだけ。キーボードのランプ */
    uint8_t value;
    struct
    {
        /* 5つのLEDインジケータに関する情報
         *
         *  0x95, 0x05,                    //   REPORT_COUNT (5)
         *  0x75, 0x01,                    //   REPORT_SIZE (1)
         *  0x05, 0x08,                    //   USAGE_PAGE (LEDs)
         *  0x19, 0x01,                    //   USAGE_MINIMUM (Num Lock)
         *  0x29, 0x05,                    //   USAGE_MAXIMUM (Kana)
         *  0x91, 0x02,                    //   OUTPUT (Data,Var,Abs)
         *
         * REPORT_SIZE (1)は、それぞれのエントリーが１ビット
         * REPORT_COUNT (5)は、5ビットのLED情報。
         * これらのアイテムはLED usageページに存在する。
         * These items are all of the usages between Num Lock (the usage
         * minimum) and Kana (the usage maximum).
         */
        unsigned numLock        :1;
        unsigned capsLock       :1;
        unsigned scrollLock     :1;
        unsigned compose        :1;
        unsigned kana           :1;

        /* バイトにするための3ビットの定数領域。
         *
         *  0x95, 0x01,                    //   REPORT_COUNT (1)
         *  0x75, 0x03,                    //   REPORT_SIZE (3)
         *  0x91, 0x03,                    //   OUTPUT (Cnst,Var,Abs)
         *
         *  REPORT_COUNT (1)は１エントリ、REPORT_SIZE (3)は３ビット
         * Report size of 3 indicates the entry is 3 bits long. */
        unsigned                :3;
    } leds;
} KEYBOARD_OUTPUT_REPORT;


/* 
 * キーボードの現在の状態を管理するためのテーブル。もともとのサンプルでは、ここに
 * キーのリピートを抑制するビットがあったが、実際のキーボードではキーリピートは機能
 * として必要なので削除した。
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
// SOFCounterは、usb_events.cで定義されている。
// USER_USB_CALLBACK_EVENT_HANDLER(USB_EVENT event, void *pdata, uint16_t size)
// のコールバックで変更される。
// SOF （Start Of Frame）は、1ms間隔で送信されるので、これをカウントして時間測定に使用する。
extern volatile signed int SOFCounter;


//　アイドル率関係の変数

KEYBOARD_INPUT_REPORT oldInputReport;           // 前回送信したInputReportを保存しておく。アイドル率で指定された時間を超えたら、前回と同じリクエストを再送する。
signed int keyboardIdleRate;                    // アイドル率。デフォルトは500(ms)だが、ホストがSET_IDLEを送ると書き換えられる。
signed int LocalSOFCount;                       // 現在のSOFのカウント
static signed int OldSOFCount;                  // 前回、Input Reportを送信したときのSOFの値


/*
 * キーボードの初期化
 */
void APP_KeyboardInit(void)
{
    //　最後に通信した時のハンドルを初期化する
    keyboard.lastINTransmission = 0;
    
    keyboard.key = 4;

    // デフォルトのアイドルレートを500msに設定。 (ホストが SET_IDLE　を送信して値を変更する)
    keyboardIdleRate = 500;

    // 割り込みコンテキストSOFCounterの値をローカル変数にコピーする。
    // SOFCounterは必ずしもアトミックではないので、while()ループを使用して最低2回読み
    // 込んで値が同じになってることをを確認する必要がある。
    while(OldSOFCount != SOFCounter) {
        OldSOFCount = SOFCounter;
    }

    //　HIDのエンドポイントを有効にする
    USBEnableEndpoint(HID_EP, USB_IN_ENABLED|USB_OUT_ENABLED|USB_HANDSHAKE_ENABLED|USB_DISALLOW_SETUP);

    //　ホストからCapsLockやNumLock情報を受け取れるようにOUTエンドポイントを設定する。
    // ここで設定するのは、outputレポートの構造体の実態。
    keyboard.lastOUTTransmission = HIDRxPacket(HID_EP,(uint8_t*)&outputReport, sizeof(outputReport) );
}

/*
 * キーボードのタスク。
 * ここでは、次の処理が行われる。
 * 1. USBデバイスが接続されて、通信可能になっていなければ何もしない。
 * 2. 前回のSOFと、今回のSOFを比較してアイドル時間を計算する。
 * 3. Input Reportのデータを構成する。( KEYBOARD_INPUT_REPORT　で定義される構造体)
 * 4. 作成された Input Reportの構造が、前回送ったものと同じかどうかを調べる
 * 5. 次の場合、Input Reportを送信する。
 * 　 a) 送信するデータが、前回送ったものと異なる場合、
 *    b) 送信するデータが前回と同じでも、既定のアイドル時間を超えた場合（前回と同じものを送信）
 * 　　　これはキーリピートのため？
 */
void APP_KeyboardTasks(void)
{
    
    signed int TimeDeltaMilliseconds;
    unsigned char i;
    bool needToSendNewReportPacket;         // Input レポートを送信すべきかを示す変数

    /* 
     * USBデバイスがまだ設定されていない場合、通信するホストがないため何もせずに戻る
     */
    if( USBGetDeviceState() < CONFIGURED_STATE ) {
        return;
    }
 
    /* もし、現在サスペンド中であれば、リモートウェイクアップを発行する必要があるか
     * どうかを確認する必要があります。 どちらの場合でも、現在ホストと通信していな
     * いので、キーボードコマンドを処理すべきではありません。なにもせずに戻ります。
    */

    if( USBIsDeviceSuspended()== true ) {
        // ユーザーがプッシュボタンを押したときに、USBホストに対してリモートウェイ
        // クアップ要求をアサートすべきかどうかをチェックします。
        
        //　とりあえずはタクトスイッチにしておくけど、ここはキーボードのボタンなのかな
        if( isTestButton()) {
            //Add code here to issue a resume signal.
        }

        return;
    }
    
    // 割り込みコンテキストSOFCounterの値をローカル変数にコピーします。
    // SOFCounterは必ずしもアトミックではないので、while()ループを使用してこれを行
    // います。更新され、正しい値を取得したことを確認するために、最低2回読み込む必要
    // があります
    while(LocalSOFCount != SOFCounter) {
        LocalSOFCount = SOFCounter;
    }

    // 最後のinput レポートが送信されてからの経過時間を計算する。 ホストによって設定さ
    // れたHIDアイドルレートに適切に従うために、この情報を得ることができます)
    TimeDeltaMilliseconds = LocalSOFCount - OldSOFCount;

    // カウントがゼロに戻るラップアラウンドによる負の値をチェックします。
    if(TimeDeltaMilliseconds < 0) {
        TimeDeltaMilliseconds = (32767 - OldSOFCount) + LocalSOFCount;
    }
    //TimeDelayがかなり大きいかどうかをチェックします。 アイドルレートが== 0
    // （「無限大」を表す）の場合、キーボードで最近ボタンが押されるなどの変化がな
    // ければ、TimeDeltaMillisecondsも無限大になる（オーバーフローを引き起こす）
    // 可能性があります。
    // したがって、TimeDeltaMillisecondsが大きくなりすぎたら、たとえ最近実際に
    // パケットを送信していなくても OldSOFCountを更新して飽和させる
    // 
    if(TimeDeltaMilliseconds > 5000) {
        OldSOFCount = LocalSOFCount - 5000;
    }

  
    // INエンドポイントがビジー状態かどうかをチェックし、ビジー状態でない場合は、
    // キーストローク・データをホストに送信するかどうかをチェックします。
    
    if(HIDTxHandleBusy(keyboard.lastINTransmission) == false) {
        // INPUTレポートバッファをクリアします。 すべてゼロに設定します。
        memset(&inputReport, 0, sizeof(inputReport));

        // ここでキーボードのデータを送信
        int keyCnt = 0;         // 一回で送信する押されたキーの数（デバッグ用）

        //if (KeyRepIdx != 0) IO_RC7_SetHigh(); else IO_RC7_SetLow();
         
        if (KeyRepIdx != 0) {
            // アトリビュートキーを処理            
            inputReport.modifiers.bits.leftControl = (modifier  & ATBKEY_LEFTCTRL)  ? 1 : 0;
            inputReport.modifiers.bits.leftShift = (modifier & ATBKEY_LEFTSHIFT)  ? 1 : 0;
            inputReport.modifiers.bits.leftAlt = (modifier& ATBKEY_LEFTALT)  ? 1 : 0;
            inputReport.modifiers.bits.leftGUI = (modifier& ATBKEY_LEFTGUI)  ? 1 : 0;
            inputReport.modifiers.bits.rightControl = (modifier & ATBKEY_RIGHTCTRL)  ? 1 : 0;
            inputReport.modifiers.bits.rightShift = (modifier & ATBKEY_RIGHTSHIFT)  ? 1 : 0;
            inputReport.modifiers.bits.rightAlt = (modifier & ATBKEY_RIGHTALT)  ? 1 : 0;
            inputReport.modifiers.bits.rightGUI = (modifier & ATBKEY_RIGHTGUI)  ? 1 : 0;
            
            // 一般キーの処理
            for (int i = 0 ; i < 6;i++) {               
                if ( KeyReport[i] !=0) {
                    inputReport.keys[i] = KeyReport[i];
                    keyCnt++;
                }
            }
        }
        
        //新しいパケット内容が、最近送信されたパケット内容と何らかの違いがあるかどうかを確認する。
        needToSendNewReportPacket = false;
        for(i = 0; i < sizeof(inputReport); i++) {
            if(*((uint8_t*)&oldInputReport + i) != *((uint8_t*)&inputReport + i)) {
                needToSendNewReportPacket = true;           // Inputレポートは送信する必要がある
                break;
            }
        }
        
        
        // ホストがアイドルレートを0以外（事実上「無限」）に設定しているかどうかを
        // 確認する。アイドルレートが無限でない場合、最後のパケットを送信してから
        // 十分な時間が経過し、新しい反復パケットを送信する時間であるかどうかを
        // 確認する。
        if(keyboardIdleRate != 0) {
            // アイドルレート時間制限を満たしているかどうかを確認する。 もしそうなら、別のHID入力レポートパケットをホストに送信する必要がある
            if(TimeDeltaMilliseconds >= keyboardIdleRate) {
                needToSendNewReportPacket = true;        // Inputレポートは送信する必要がある
            }
        }

        // ここで、新しい入力レポートパケットを送信することが適切である場合、それを送信します。
        // (例: 新しいデータが存在する、またはアイドルレートの制限に達した)
        if(needToSendNewReportPacket == true) {         // Inputレポートを送信する必要があるなら
            //古い入力レポートパケットの内容を保存します。 これは、無限アイドル
            // レート設定を使用しているときに、何かが変更され、ホストに再送信する
            // 必要がある場合を判断するのに役立つレポートパケットの内容の変更を検出できるようにするためです。
            oldInputReport = inputReport;

            // ８バイトのパケット（Input レポート）をPCに送信する 
            // KEYBOARD_INPUT_REPORT型で、修飾キー(1)+1バイトの埋め草+6バイトの入力キー
            keyboard.lastINTransmission = HIDTxPacket(HID_EP, (uint8_t*)&inputReport, sizeof(inputReport));
            OldSOFCount = LocalSOFCount;    //Save the current time, so we know when to send the next packet (which depends in part on the idle rate setting)
        }

    }

    /* PCからキーボードデバイスに何らかのデータが送信されたかどうかを確認します。 
     * レポートディスクリプタにより、ホストは1バイトのデータを送信することができます。 
     * ビット0-4はLEDの状態、ビット5-7は未使用のパッドビットです。 ホストはこのOUT
     * レポートデータをHID OUTエンドポイント（EP1 OUT）を通して送信することが可能で
     * すが、その代わりに、ホストはEP0上でSET_REPORT制御転送を送信することによって
     * LED状態情報を送信しようとすることも可能です。 USBHIDCBSetReportHandler()関数
     * を参照してください。 
     */
    if(HIDRxHandleBusy(keyboard.lastOUTTransmission) == false) {
        APP_KeyboardProcessOutputReport();
        keyboard.lastOUTTransmission = HIDRxPacket(HID_EP,(uint8_t*)&outputReport,sizeof(outputReport));
    }
    return;		
}


/*
 * レポートに準じてCAPSLOCKランプを制御する。RC5を使おう。
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
 ホストから、LEDの状態データが到着(SET_REPORT)したので、これをoutput report bufferにコピーして処理する。
 */
static void USBHIDCBSetReportComplete(void)
{
    /* 1 byte of LED state data should now be in the CtrlTrfData buffer.  Copy
     * it to the OUTPUT report buffer for processing */
    outputReport.value = CtrlTrfData[0];

    /* Process the OUTPUT report. */
    APP_KeyboardProcessOutputReport();
}

/* エンドポイント0にSET_REPORT制御転送でキーボードLEDの状態データを受信する準備をする。
   ホストは、レポートディスクリプタが送信できるのはこれだけなので、 1バイトだけ送信する。
 　SET_REPORTは ホストがファンクション・デバイスにデータを送信するためのリクエスト。
  フィールド       サイズ 設定値
  bmRequestType     1 リクエスト・タイプ：0x21
  bRequest          1 リクエスト識別子：0x09（Get_Report）
  wValue            2 上位 1 バイト：レポートの種類，下位 1 バイト：レポート ID
  wIndex            2 処理対象のインタフェース番号
  wLength           2 レポート長
*/
void USBHIDCBSetReportHandler(void)
{
    USBEP0Receive((uint8_t*)&CtrlTrfData, USB_EP0_BUFF_SIZE, USBHIDCBSetReportComplete);
}


// ホストがSET_ILDEコマンドを送ってきたときにUSBスタックに呼び出されるコールバック。
// SET_IDLEはホストがファンクション・デバイスのアイドル率を設定するためのリクエスト。
// キーボード・デバイスは，このリクエストをサポートする必要がある。
// フィールド    サイズ 設定値
// bmRequestType  1 リクエスト・タイプ：0x21
// bRequest       1 リクエスト識別子：0x0A（Set_Idle）
// wValue         2 上位 1 バイト：レポート間の最大時間間隔（4 ms 単位）（0 の場合，変化があった
//                  場合のみレポートを返し，変化がないときは NAK 応答します）
//                  下位 1 バイト：レポート ID（0 の場合，すべての入力レポートに適用します）
// wIndex         2 処理対象のインタフェース番号
// wLength        2 0x0000 
void USBHIDCBSetIdleRateHandler(uint8_t reportID, uint8_t newIdleRate)
{
    // レポートIDがキーボード入力のレポートID番号と一致することを確認する。
    // ただし、ファームウェアがレポートID番号を実装/使用していない場合は、== 0とすること。
        
    if(reportID == 0) {
        keyboardIdleRate = newIdleRate;
    }
}

