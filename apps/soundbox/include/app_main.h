#ifndef APP_MAIN_H
#define APP_MAIN_H

typedef struct _APP_VAR {
    s8 music_volume;
    s8 call_volume;
    s8 wtone_volume;
    u8 opid_play_vol_sync;
    u8 aec_dac_gain;
    u8 aec_mic_gain;
    u8 aec_mode;        	//AEC模式,default:advance(diable(0), reduce(1), advance(2))
    u8 ul_eq_en;        	//上行EQ使能,default:enable(disable(0), enable(1))
    /*AGC*/
    float ndt_fade_in;  	//单端讲话淡入步进default: 1.3f(0.1 ~ 5 dB)
    float ndt_fade_out;  	//单端讲话淡出步进default: 0.7f(0.1 ~ 5 dB)
    float dt_fade_in;  		//双端讲话淡入步进default: 1.3f(0.1 ~ 5 dB)
    float dt_fade_out;  	//双端讲话淡出步进default: 0.7f(0.1 ~ 5 dB)
    float ndt_max_gain;   	//单端讲话放大上限,default: 12.f(0 ~ 24 dB)
    float ndt_min_gain;   	//单端讲话放大下限,default: 0.f(-20 ~ 24 dB)
    float ndt_speech_thr;   //单端讲话放大阈值,default: -50.f(-70 ~ -40 dB)
    float dt_max_gain;   	//双端讲话放大上限,default: 12.f(0 ~ 24 dB)
    float dt_min_gain;   	//双端讲话放大下限,default: 0.f(-20 ~ 24 dB)
    float dt_speech_thr;    //双端讲话放大阈值,default: -40.f(-70 ~ -40 dB)
    float echo_present_thr; //单端双端讲话阈值,default:-70.f(-70 ~ -40 dB)
    /*AEC*/
    float aec_dt_aggress;   //原音回音追踪等级, default: 1.0f(1 ~ 5)
    float aec_refengthr;    //进入回音消除参考值, default: -70.0f(-90 ~ -60 dB)
    /*ES*/
    float es_aggress_factor;//回音前级动态压制,越小越强,default: -3.0f(-1 ~ -5)
    float es_min_suppress;	//回音后级静态压制,越大越强,default: 4.f(0 ~ 10)
    /*ANS*/
    float ans_aggress;    	//噪声前级动态压制,越大越强default: 1.25f(1 ~ 2)
    float ans_suppress;    	//噪声后级静态压制,越小越强default: 0.04f(0 ~ 1)
    u8 ble_rf_power;
    u8 rf_power;
    u8 poweron_charge;                  //开机进充电标志
    u8 goto_poweroff_flag;
    u8 goto_poweroff_cnt;
    u8 play_poweron_tone;
    u8 remote_dev_company;
    u8 siri_stu;
    int auto_stop_page_scan_timer;     //用于1拖2时，有一台连接上后，超过三分钟自动关闭Page Scan
    volatile int auto_shut_down_timer;
    volatile int wait_exit_timer;
    u16 auto_off_time;
    u16 warning_tone_v;
    u16 poweroff_tone_v;
    u32 start_time;
    s8  usb_mic_gain;
    u8  reverb_status;                 //用于tws+混响在pc模式时，同步关闭混响, 0:非pc模式
    u8 	cycle_mode;
} APP_VAR;

typedef struct _BT_USER_PRIV_VAR {
    //phone
    u8 phone_ring_flag: 1;
    u8 phone_num_flag: 1;
    u8 phone_income_flag: 1;
    u8 phone_call_dec_begin: 1;
    u8 phone_con_sync_num_ring: 1;
    u8 phone_con_sync_ring: 1;
    u8 emitter_or_receiver: 2;
    u16 get_phone_num_timecnt;

    u8 inband_ringtone;
    u8 phone_vol;
    u16 phone_timer_id;
    u8 last_call_type;
    u8 income_phone_num[30];
    u8 income_phone_len;
    s32 auto_connection_counter;
    s32 search_counter;
    int auto_connection_timer;
    u8 auto_connection_addr[6];
    int tws_con_timer;
    u8 tws_start_con_cnt;
    u8 tws_conn_state;
    bool search_tws_ing;
    int sniff_timer;
    bool fast_test_mode;
    u8 set_call_vol_flag;  /*有些客户希望通话默认给手机设置最大音量*/
} BT_USER_PRIV_VAR;
typedef enum {
    MY_CMD_OK,                     // [0x00] 命令处理成功/应答 OK
    MY_CMD_ERR,                    // [0x01] 命令处理失败/应答 ERR
    MY_TURN_ON_BT,                 // [0x02] 开启蓝牙
    MY_TURN_OFF_BT,                // [0x03] 关闭蓝牙
    MY_SET_BT_MODE,                // [0x04] 设置蓝牙工作模式
    MY_SET_BT_RESET,               // [0x05] 复位蓝牙模块
    MY_BT_CONN_STATUS,             // [0x06] 获取/查询蓝牙连接状态
    MY_BT_SET_PIN_PAIR,            // [0x07] 设置蓝牙配对 PIN 码
    GX_BT_SET_RFCOMUUID_CONN,      // [0x08] 设置 RFCOMM UUID 连接参数
    GX_SET_UART_UPDATE,            // [0x09] 触发/设置串口固件升级
    MY_IAP_CONNECT_SUCCESS_NEW,     // 0x0A: hicar iAP 协议握手连接成功通知
    MY_IAP_GET_DATA_NEW,            // 0x0B: hicar iAP 协议读取/接收数据
    MY_IAP_SEND_DATA_NEW,           // 0x0C: hicar iAP 协议发送数据

    GX_SET_BT_PIN_FUNCTION = 0x30, // [0x30] 设置蓝牙引脚复用功能
    GX_SET_BT_GPO_POL,             // [0x31] 设置蓝牙 GPO (通用输出) 极性
    GX_SEND_BT_GPI_STAT,           // [0x32] 上报/发送蓝牙 GPI (通用输入) 状态
    GX_SET_BT_PWM_PARAM,           // [0x33] 设置蓝牙 PWM 参数

    MY_ASK_VERSION = 0xa4,         // [0xA4] 查询版本号
    MY_ANSWER_VERSION,             // [0xA5] 返回/应答版本号

    GX_SET_BLE_ADV_DATA = 0xaf,    // [0xAF] 设置 BLE 低功耗蓝牙广播数据
    GX_SET_HID_DESCRIPTOR = 0xb0,  // [0xB0] 设置 HID 设备描述符
    GX_SET_HID_MESSAGE,            // [0xB1] 发送/设置 HID 消息 (按键/数据)
    GX_SET_HID_DEVICE_NAME,        // [0xB2] 设置 HID 设备名称
    GX_GET_HID_DEVICE_NAME,        // [0xB3] 获取 HID 设备名称
    GX_SEND_HID_DEVICE_NAME,       // [0xB4] 上报/发送 HID 设备名称
    GX_SET_HID_RADIO_FREQ_START,   // [0xB5] 开启 HID 射频频点测试/设置
    GX_SET_HID_RADIO_FREQ_STOP,    // [0xB6] 停止 HID 射频频点测试
    GX_GET_HID_DEVICE_STATUS,      // [0xB7] 获取 HID 设备状态
    GX_SEND_HID_STATUS,            // [0xB8] 上报/发送 HID 设备状态
    GX_GET_HID_FW_VERSION,         // [0xB9] 获取 HID 固件版本
    GX_SEND_HID_FW_VERSION,        // [0xBA] 上报/发送 HID 固件版本
    GX_SET_HID_FACTORY_SETTINGS,   // [0xBB] 恢复 HID 出厂设置
    GX_SET_HID_DELETE_DEVICE,      // [0xBC] 删除 HID 已配对设备
    GX_GET_HID_BATTERY_LEVEL,      // [0xBD] 获取 HID 设备电量
    GX_SEND_HID_BATTERY_LEVEL,     // [0xBE] 上报/发送 HID 设备电量
    GX_SET_HID_DEVICE_DISCONNECT,  // [0xBF] 断开 HID 设备连接


    MY_IAP_CONNECT_SUCCESS = 0xc0, // [0xC0] 苹果 iAP 协议连接成功
    MY_IAP_GET_DATA,               // [0xC1] 从 iAP 读取数据
    MY_IAP_SEND_DATA,              // [0xC2] 向 iAP 发送数据
    MY_SET_BT_NAME,                // [0xC3] 设置蓝牙广播名称
    MY_GET_BT_NAME,                // [0xC4] 获取蓝牙广播名称
    MY_SEND_BT_NAME,               // [0xC5] 上报/发送蓝牙广播名称
    MY_GET_PHONE_NAME,             // [0xC6] 获取连接的手机名称
    MY_SEND_PHONE_NAME,            // [0xC7] 上报/发送连接的手机名称
    MY_GET_BT_MAC,                 // [0xC8] 获取本机蓝牙 MAC 地址
    MY_SEND_BT_MAC,                // [0xC9] 上报/发送本机蓝牙 MAC 地址
    MY_GET_PHONE_MAC,              // [0xCA] 获取连接的手机 MAC 地址
    MY_SEND_PHONE_MAC,             // [0xCB] 上报/发送连接的手机 MAC 地址

    MY_DISCONNECT_PHONE,           // [0xCC] 主动断开手机连接
    MY_RECONNECT_PHONE,            // [0xCD] 重新连接手机
    MY_DELETE_PHONE,               // [0xCE] 删除已配对手机记录
    
    MY_BT_DISCONNECT,              // [0xCF] 断开蓝牙连接
    MY_GET_BT_STATE,               // [0xD0] 获取当前蓝牙状态
    MY_BT_STATE,                   // [0xD1] 上报/设置蓝牙状态


    MY_SET_VOL,                    // [0xD2] 设置音量
    MY_GET_VOL,                    // [0xD3] 获取音量
    MY_SEND_VOL,                   // [0xD4] 上报/发送音量状态
    MY_CARPLAY_RECONN,             // [0xD5] CarPlay 重连控制

    // 蓝牙部分
    GX_GET_BT_PBAP,     // [0xD6] 电话本
    GX_SEND_BT_PBAP,               // [0xD7] 发送/上报电话本数据
    GX_GET_BT_PBAP_PART,// [0xD8] 通话记录
    GX_SEND_BT_PBAP_PART,          // [0xD9] 发送/上报通话记录数据
    GX_GET_BT_CALL_STATUS,  // [0xDA] 获取当前通话状态
    GX_SEND_BT_CALL_STATUS,        // [0xDB] 上报当前通话状态
    GX_SET_BT_CALL_BACK,           // [0xDC] 控制接听/挂断/回拨电话
    GX_SET_BT_CALL_STATUS,         // [0xDD] 设置/更新通话状态
    GX_GET_BT_CALLING_PHONE_NUMBER,// [0xDE] 获取来电/去电电话号码
    GX_SEND_BT_CALLING_PHONE_NUMBER,// [0xDF] 上报当前通话电话号码
    GX_SET_BT_MUSIC_TRACK_DETAILS,  // [0xE0] 获取蓝牙音乐信息
    GX_SEND_BT_MUSIC_TRACK_DETAILS,// [0xE1] 上报蓝牙音乐曲目详情信息
    GX_SET_BT_MUSIC_STATUS,     // [0xE2] 控制蓝牙播歌


    // fm发射部分
    GX_SET_FMTX_ON_OFF,            // [0xE3] 开关 FM 发射功能
    GX_SET_FMTX_POWER,             // [0xE4] 设置 FM 发射功率
    GX_SET_BT_FMTX_FREQUENCY,       // [0xE5] fm发射频点

    // fm接收部分
    GX_SET_FMRX_ON_OFF,            // [0xE6] 开关 FM 接收功能
    GX_SET_FMRX_AUTO_SEARCH,       // [0xE7] 开启/停止 FM 自动搜台
    GX_SET_FMRX_SEARCH_RANGE,      // [0xE8] 设置 FM 搜台频率范围

    GX_SEND_FMRX_PLAYING_FREQUENCY,// [0xE9] 上报当前播放的 FM 频率
    GX_SET_BT_FMRX_DAC_OUTPUT_LR,  // [0xEA] 设置 FM 接收的音频 DAC 左右声道输出
    GX_SET_BT_FMRX_Search_Power,   // [0xEB] 设置 FM 搜台灵敏度/门限功率
    GX_SET_BT_FMRX_DAC_EQ_MODE,    // [0xEC] 设置 FM 音频 DAC 的 EQ 均衡器模式

    //补充部分
    GX_SET_BT_DTMF_SEND,           // [0xED] 通话中发送 DTMF 双音多频拨号音
    GX_GET_PHONE_TIME,             // [0xEE] 获取手机系统时间
    GX_SEND_PHONE_TIME,            // [0xEF] 上报/同步手机系统时间
    GX_GET_A2DP_PLAYING_STATUS,    // [0xF0] 获取蓝牙音频 (A2DP) 播放状态
    GX_SEND_A2DP_PLAYING_STATUS,   // [0xF1] 上报蓝牙音频 (A2DP) 播放状态
    GX_GET_BT_MUSIC_PLAYING_TIME,  // [0xF2] 获取当前蓝牙音乐播放进度/时间
    GX_SEND_BT_MUSIC_PLAYING_TIME, // [0xF3] 上报当前蓝牙音乐播放进度/时间
    GX_SET_BT_CALLING_SCO_STATUS,  // [0xF4] 切换/设置通话音频通道 (SCO)
    GX_GET_BT_CALLING_SCO_STATUS,  // [0xF5] 获取当前通话音频链路 (SCO) 状态
    GX_SEND_BT_CALLING_SCO_STATUS, // [0xF6] 上报当前通话音频链路 (SCO) 状态
    GX_SET_BT_CONN_PINCODE,        // [0xF7] 设置蓝牙连接配对密码 (PIN Code)
    GX_GET_BT_CONN_PINCODE,        // [0xF8] 获取蓝牙连接配对密码 (PIN Code)
    GX_SEND_BT_CONN_PINCODE,       // [0xF9] 发送/上报蓝牙连接配对密码 (PIN Code)
    GX_SET_HICAR_RECONNECT,        // [0xFA] 设置hicar回连 
} Hccast_cmd;

typedef struct __hc_bt_open_statsus {
    u8 bt_open;
    u8 ble_open;
} HC_BT_OPEN;
extern HC_BT_OPEN hc_open_bt;

#define MAX_PHONE_NUM      3
typedef enum {
    CONNECTIONMODE_BT = 0,
    CONNECTIONMODE_Carplay,
    CONNECTIONMODE_Android_Auto,
    CONNECTIONMODE_Carlife,
    CONNECTIONMODE_Hicar,
    CONNECTIONMODE_Carlink,
} ConnectionMode_t;
typedef struct {
    u8 addr[6];
    u8 valid;
} phone_item_t;

typedef struct {
    phone_item_t ios[MAX_PHONE_NUM];
    phone_item_t android[MAX_PHONE_NUM];
} phone_vm_t;

typedef struct
{
    u8 type;            // IOS / ANDROID
    u8 index;           // 当前尝试第几个
    u8 running;
} reconnect_mgr_t;

typedef struct
{
    u8 aa_flag;
} disconn_message_t;

extern disconn_message_t hc_disconn_message;
extern reconnect_mgr_t reconnect_mgr;

typedef enum {
    BLE_ADV_TYPE_ADV = 0,
    BLE_ADV_TYPE_RSP,
}BLE_ADV_TYPE;

typedef struct {
    u8 adv_len;        // 数据长度
    u8 adv_data[32];   // 广播数据
} ble_adv_data_t;
extern ble_adv_data_t custom_ble_adv_data;

typedef struct {
    u8 rsp_len;        // 数据长度
    u8 rsp_data[32];   // 响应数据
} ble_rsp_data_t;
extern ble_rsp_data_t custom_ble_rsp_data;

#define    BT_EMITTER_EN     1
#define    BT_RECEIVER_EN    2

typedef struct _BT_USER_COMM_VAR {
} BT_USER_COMM_VAR;

extern APP_VAR app_var;
extern BT_USER_PRIV_VAR bt_user_priv_var;

extern void app_main();

#define earphone (&bt_user_priv_var)

#endif
