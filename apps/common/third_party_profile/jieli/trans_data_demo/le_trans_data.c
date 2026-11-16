/*********************************************************************************************
    *   Filename        : le_server_module.c

    *   Description     :

    *   Author          :

    *   Email           : zh-jieli.com

    *   Last modifiled  : 2017-01-17 11:14

    *   Copyright:(c)JIELI  2011-2016  @ , All Rights Reserved.
*********************************************************************************************/

// *****************************************************************************
/* EXAMPLE_START(le_counter): LE Peripheral - Heartbeat Counter over GATT
 *
 * @text All newer operating systems provide GATT Client functionality.
 * The LE Counter examples demonstrates how to specify a minimal GATT Database
 * with a custom GATT Service and a custom Characteristic that sends periodic
 * notifications.
 */
// *****************************************************************************
#include "system/app_core.h"
#include "system/includes.h"

#include "app_config.h"
#include "app_action.h"

#include "btstack/btstack_task.h"
#include "btstack/bluetooth.h"
#include "user_cfg.h"
#include "vm.h"
#include "btcontroller_modules.h"
#include "bt_common.h"
#include "3th_profile_api.h"

#include "le_trans_data.h"
#include "le_common.h"

#include "rcsp_bluetooth.h"
#include "JL_rcsp_api.h"
#include "custom_cfg.h"

#include "app_main.h"

#if (TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_TRANS_DATA)

//TRANS +HID
#define TRANS_HID_EN                 1

//TRANS ANCS
#define TRANS_ANCS_EN  			  	 0
#if TRANS_ANCS_EN
#include "btstack/btstack_event.h"
#endif

#define TEST_SEND_DATA_RATE          0  //测试上行发送数据
#define TEST_SEND_HANDLE_VAL         ATT_CHARACTERISTIC_ae02_01_VALUE_HANDLE
/* #define TEST_SEND_HANDLE_VAL         ATT_CHARACTERISTIC_ae05_01_VALUE_HANDLE */
#define EXT_ADV_MODE_EN              0

#define TEST_AUDIO_DATA_UPLOAD       0 //测试文件上传


#if 1
extern void printf_buf(u8 *buf, u32 len);
#define log_info          printf
#define log_info_hexdump  printf_buf
#else
#define log_info(...)
#define log_info_hexdump(...)
#endif


/* #define LOG_TAG_CONST       BT_BLE */
/* #define LOG_TAG             "[LE_S_DEMO]" */
/* #define LOG_ERROR_ENABLE */
/* #define LOG_DEBUG_ENABLE */
/* #define LOG_INFO_ENABLE */
/* #define LOG_DUMP_ENABLE */
/* #define LOG_CLI_ENABLE */
/* #include "debug.h" */

//------
//ATT发送的包长,    note: 20 <=need >= MTU
#define ATT_LOCAL_PAYLOAD_SIZE    (200)                   //
//ATT缓存的buffer大小,  note: need >= 20,可修改
#define ATT_SEND_CBUF_SIZE        (512)                   //

//共配置的RAM
#define ATT_RAM_BUFSIZE           (ATT_CTRL_BLOCK_SIZE + ATT_LOCAL_PAYLOAD_SIZE + ATT_SEND_CBUF_SIZE)                   //note:
static u8 att_ram_buffer[ATT_RAM_BUFSIZE] __attribute__((aligned(4)));
//---------------

#if TEST_SEND_DATA_RATE
static u32 test_data_count;
static u32 server_timer_handle = 0;
static u8 test_data_start;
#endif

//---------------
// 广播周期 (unit:0.625ms)
#define ADV_INTERVAL_MIN          (160*1)//(160*5)

#define HOLD_LATENCY_CNT_MIN  (3)  //(0~0xffff)
#define HOLD_LATENCY_CNT_MAX  (15) //(0~0xffff)
#define HOLD_LATENCY_CNT_ALL  (0xffff)

static volatile hci_con_handle_t con_handle;

//加密设置
static const uint8_t sm_min_key_size = 7;

//连接参数更新请求设置
//是否使能参数请求更新,0--disable, 1--enable
static const uint8_t connection_update_enable = 1; ///0--disable, 1--enable
//当前请求的参数表index
static uint8_t connection_update_cnt = 0; //

//参数表
static const struct conn_update_param_t connection_param_table[] = {
    // {16, 24, 10, 600},//11
    // {12, 28, 10, 600},//3.7
    // {8,  20, 10, 600},
    /* {12, 28, 4, 600},//3.7 */
    /* {12, 24, 30, 600},//3.05 */
    {6,  10, 0, 100},
    {10, 14, 0, 100},
    {14, 20, 0, 100},
};

//共可用的参数组数
#define CONN_PARAM_TABLE_CNT      (sizeof(connection_param_table)/sizeof(struct conn_update_param_t))

#if (ATT_RAM_BUFSIZE < 64)
#error "adv_data & rsp_data buffer error!!!!!!!!!!!!"
#endif

//用户可配对的，这是样机跟客户开发的app配对的秘钥
const u8 link_key_data[16] = {0x06, 0x77, 0x5f, 0x87, 0x91, 0x8d, 0xd4, 0x23, 0x00, 0x5d, 0xf1, 0xd8, 0xcf, 0x0c, 0x14, 0x2b};
/* const u8 link_key_data[16] = {0x06, 0x77, 0x5f, 0x87, 0x91, 0x8d, 0xd4, 0x23, 0x00, 0x5d, 0xf1, 0xd8, 0xcf, 0x0c, 0x14, 0x2b}; */
#define EIR_TAG_STRING   0xd6, 0x05, 0x08, 0x00, 'J', 'L', 'A', 'I', 'S', 'D','K'
static const char user_tag_string[] = {EIR_TAG_STRING};

static u8 adv_data_len;
static u8 adv_data[ADV_RSP_PACKET_MAX];//max is 31
static u8 scan_rsp_data_len;
static u8 scan_rsp_data[ADV_RSP_PACKET_MAX];//max is 31

/* #define adv_data       &att_ram_buffer[0] */
/* #define scan_rsp_data  &att_ram_buffer[32] */

static char gap_device_name[BT_NAME_LEN_MAX] = "jl_ble_test";
static u8 gap_device_name_len = 0; //名字长度，不包含结束符
static u8 ble_work_state = 0;      //ble 状态变化
static u8 adv_ctrl_en;             //广播控制

static u8 test_read_write_buf[4];

static void (*app_recieve_callback)(void *priv, void *buf, u16 len) = NULL;
static void (*app_ble_state_callback)(void *priv, ble_state_e state) = NULL;
static void (*ble_resume_send_wakeup)(void) = NULL;
static u32 channel_priv;

static int app_send_user_data_check(u16 len);
static int app_send_user_data_do(void *priv, u8 *data, u16 len);
static int app_send_user_data(u16 handle, u8 *data, u16 len, u8 handle_type);

// Complete Local Name  默认的蓝牙名字

//------------------------------------------------------
//广播参数设置
static void advertisements_setup_init();
static int set_adv_enable(void *priv, u32 en);
static int get_buffer_vaild_len(void *priv);
extern const char *bt_get_local_name();
extern void clr_wdt(void);
extern void sys_auto_shut_down_disable(void);
extern void sys_auto_shut_down_enable(void);
extern u8 get_total_connect_dev(void);

//------------------------------------------------------

#if TRANS_HID_EN
#if 0
static const u8 hid_report_map[] = {
    0x05, 0x0C,        // Usage Page (Consumer)
    0x09, 0x01,        // Usage (Consumer Control)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x01,        //   Report ID (1)
    0x09, 0xE9,        //   Usage (Volume Increment)
    0x09, 0xEA,        //   Usage (Volume Decrement)
    0x09, 0xCD,        //   Usage (Play/Pause)
    0x09, 0xE2,        //   Usage (Mute)
    0x09, 0xB6,        //   Usage (Scan Previous Track)
    0x09, 0xB5,        //   Usage (Scan Next Track)
    0x09, 0xB3,        //   Usage (Fast Forward)
    0x09, 0xB4,        //   Usage (Rewind)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x10,        //   Report Count (16)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              // End Collection
    // 35 bytes
};
#else
u8 hid_report_map[512] = {0};
u16 hid_map_len = 0;
#endif

#define  PNP_VID_SOURCE   0x02
#define  PNP_VID          0x05ac
#define  PNP_PID          0x12ad //0x022C
#define  PNP_PID_VERSION  0x011b
//profile 支持的report id
static const u16 report_id_handle_table[] = {
    0,
    HID_REPORT_ID_01_SEND_HANDLE,
    HID_REPORT_ID_02_SEND_HANDLE,
    HID_REPORT_ID_03_SEND_HANDLE,
    HID_REPORT_ID_04_SEND_HANDLE,
    HID_REPORT_ID_05_SEND_HANDLE,
    HID_REPORT_ID_06_SEND_HANDLE,
};
static const u8 PnP_ID[] = {PNP_VID_SOURCE, PNP_VID & 0xFF, PNP_VID >> 8, PNP_PID & 0xFF, PNP_PID >> 8, PNP_PID_VERSION & 0xFF, PNP_PID_VERSION >> 8};
static const u8 hid_information[] = {0x01, 0x01, 0x00, 0x03};
static const u8 Appearance[] = {BLE_APPEARANCE_HID_KEYBOARD & 0xff, BLE_APPEARANCE_HID_KEYBOARD >> 8};
static u8 first_pair_flag = 0;     //第一次配对标记
static u8 cur_peer_addr_info[7];//当前连接对方地址信息
static int app_send_user_data(u16 handle, u8 *data, u16 len, u8 handle_type);
// static int hid_send_data(u8 *data, u16 len)
static int hid_send_data(u8 report_id,u8 *data, u16 len)
{
    // u16 handle = ATT_CHARACTERISTIC_2a4d_01_VALUE_HANDLE;
    u16 handle = report_id_handle_table[report_id];
    return app_send_user_data(handle, data, len, ATT_OP_AUTO_READ_CCC);
}
/*
void hid_send_pp(void)
{
    u8 buf[2] = {0x04, 0x00};
    hid_send_data(buf, sizeof(buf));
    os_time_dly(1);
    buf[0] = 0;
    hid_send_data(buf, sizeof(buf));
}
*/

u8 g_hudi_bluetooth_hid_kbd_mouse_disc[] =
{
    /* mouse */
    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x02,        // Usage (Mouse)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x01,        //   report id (1)
    0x09, 0x01,        //   Usage (Pointer)
    0xA1, 0x00,        //   Collection (Physical)
    0x05, 0x09,        //     Usage Page (Button)
    0x19, 0x01,        //     Usage Minimum (0x01)
    0x29, 0x05,        //     Usage Maximum (0x05)
    0x15, 0x00,        //     Logical Minimum (0)
    0x25, 0x01,        //     Logical Maximum (1)
    0x95, 0x05,        //     Report Count (5)
    0x75, 0x01,        //     Report Size (1)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x01,        //     Report Count (1)
    0x75, 0x03,        //     Report Size (3)
    0x81, 0x01,        //     Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x01,        //     Usage Page (Generic Desktop Ctrls)
    0x09, 0x30,        //     Usage (X)
    0x09, 0x31,        //     Usage (Y)
    0x09, 0x38,        //     Usage (Wheel)
    0x15, 0x81,        //     Logical Minimum (-127)
    0x25, 0x7F,        //     Logical Maximum (127)
    0x75, 0x08,        //     Report Size (8)
    0x95, 0x03,        //     Report Count (3)
    0x81, 0x06,        //     Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              //   End Collection
    0xC0,              // End Collection

    // 54 bytes

    /* kbd */
    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x06,        // Usage (Keyboard)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x02,        //   Report ID (2)
    0x05, 0x07,        //   Usage Page (Kbrd/Keypad)
    0x19, 0xE0,        //   Usage Minimum (0xE0)
    0x29, 0xE7,        //   Usage Maximum (0xE7)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x08,        //   Report Count (8)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x08,        //   Report Size (8)
    0x81, 0x01,        //   Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x03,        //   Report Count (3)
    0x75, 0x01,        //   Report Size (1)
    0x05, 0x08,        //   Usage Page (LEDs)
    0x19, 0x01,        //   Usage Minimum (Num Lock)
    0x29, 0x03,        //   Usage Maximum (Scroll Lock)
    0x91, 0x02,        //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x95, 0x05,        //   Report Count (5)
    0x75, 0x01,        //   Report Size (1)
    0x91, 0x01,        //   Output (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x95, 0x06,        //   Report Count (6)
    0x75, 0x08,        //   Report Size (8)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x05, 0x07,        //   Usage Page (Kbrd/Keypad)
    0x19, 0x00,        //   Usage Minimum (0x00)
    0x2A, 0xFF, 0x00,  //   Usage Maximum (0xFF)
    0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              // End Collection
    0x05, 0x0C,        // Usage Page (Consumer)
    0x09, 0x01,        // Usage (Consumer Control)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x05,        //   Report ID (5)
    0x75, 0x10,        //   Report Size (16)
    0x95, 0x01,        //   Report Count (1)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0x8C, 0x02,  //   Logical Maximum (652)
    0x19, 0x00,        //   Usage Minimum (Unassigned)
    0x2A, 0x8C, 0x02,  //   Usage Maximum (AC Send)
    0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              // End Collection

    // 92 bytes
};

#endif

//NACS
#if TRANS_ANCS_EN
#define ANCS_SUBEVENT_CLIENT_NOTIFICATION                           0xF1
void ancs_client_init(void);
void ancs_client_register_callback(btstack_packet_handler_t callback);
const char *ancs_client_attribute_name_for_id(int id);
void ancs_set_notification_buffer(u8 *buffer, u16 buffer_size);

//ancs info buffer
#define ANCS_INFO_BUFFER_SIZE  (1024)
static u8 ancs_info_buffer[ANCS_INFO_BUFFER_SIZE];
#endif

//------------------------------------------------------
#if TEST_AUDIO_DATA_UPLOAD
static const u8 test_audio_data_file[1024] = {
    1, 2, 3, 4, 5, 6, 7, 8, 9
};

#define AUDIO_ONE_PACKET_LEN  128
static void test_send_audio_data(int init_flag)
{
    static u32 send_pt = 0;
    static u32 start_flag = 0;

    if (!con_handle) {
        return;
    }

    if (init_flag) {
        log_info("audio send init\n");
        send_pt = 0;
        start_flag = 1;
    }

    if (!start_flag) {
        return;
    }

    u32 file_size = sizeof(test_audio_data_file);
    u8 *file_ptr = test_audio_data_file;

    if (send_pt >= file_size) {
        log_info("audio send Complete\n");
        start_flag = 0;
        return;
    }

    u32 send_len = file_size - send_pt;
    if (send_len > AUDIO_ONE_PACKET_LEN) {
        send_len = AUDIO_ONE_PACKET_LEN;
    }

    while (1) {
        if (app_send_user_data_check(send_len)) {
            log_info("audio send %08x\n", send_pt);
            if (app_send_user_data(ATT_CHARACTERISTIC_ae3c_01_VALUE_HANDLE, &file_ptr[send_pt], send_len, ATT_OP_AUTO_READ_CCC)) {
                log_info("audio send fail!\n");
                break;
            } else {
                send_pt += send_len;
            }
        } else {
            break;
        }
    }
}

#endif


static void send_request_connect_parameter(u8 table_index)
{
    struct conn_update_param_t *param = (void *)&connection_param_table[table_index];//static ram

    log_info("update_request:-%d-%d-%d-%d-\n", param->interval_min, param->interval_max, param->latency, param->timeout);
    if (con_handle) {
        ble_op_conn_param_request(con_handle, param);
    }
}

static void check_connetion_updata_deal(void)
{
    if (connection_update_enable) {
        if (connection_update_cnt < CONN_PARAM_TABLE_CNT) {
            send_request_connect_parameter(connection_update_cnt);
        }
    }
}

static void connection_update_complete_success(u8 *packet)
{
    int con_handle, conn_interval, conn_latency, conn_timeout;

    con_handle = hci_subevent_le_connection_update_complete_get_connection_handle(packet);
    conn_interval = hci_subevent_le_connection_update_complete_get_conn_interval(packet);
    conn_latency = hci_subevent_le_connection_update_complete_get_conn_latency(packet);
    conn_timeout = hci_subevent_le_connection_update_complete_get_supervision_timeout(packet);

    log_info("conn_interval = %d\n", conn_interval);
    log_info("conn_latency = %d\n", conn_latency);
    log_info("conn_timeout = %d\n", conn_timeout);
}


static void set_ble_work_state(ble_state_e state)
{
    if (state != ble_work_state) {
        log_info("ble_work_st:%x->%x\n", ble_work_state, state);
        ble_work_state = state;
        if (app_ble_state_callback) {
            app_ble_state_callback((void *)channel_priv, state);
        }
    }
}

static ble_state_e get_ble_work_state(void)
{
    return ble_work_state;
}

static void cbk_sm_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    sm_just_event_t *event = (void *)packet;
    u32 tmp32;
    switch (packet_type) {
    case HCI_EVENT_PACKET:
        switch (hci_event_packet_get_type(packet)) {
        case SM_EVENT_JUST_WORKS_REQUEST:
            sm_just_works_confirm(sm_event_just_works_request_get_handle(packet));
            log_info("Just Works Confirmed.\n");
            first_pair_flag = 1;
            break;
        case SM_EVENT_PASSKEY_DISPLAY_NUMBER:
            log_info_hexdump(packet, size);
            memcpy(&tmp32, event->data, 4);
            log_info("Passkey display: %06u.\n", tmp32);
            break;
        }
        break;
    }
}


#if TEST_SEND_DATA_RATE
static void server_timer_handler(void)
{
    if (!con_handle) {
        test_data_count = 0;
        test_data_start = 0;
        return;
    }

    log_info("peer_rssi = %d\n", ble_vendor_get_peer_rssi(con_handle));

    if (test_data_count) {
        log_info("\n%d bytes send: %d.%02d KB/s \n", test_data_count, test_data_count / 1000, test_data_count % 1000);
        test_data_count = 0;
    }
}

static void server_timer_start(void)
{
    if (server_timer_handle) {
        return;
    }

    server_timer_handle  = sys_timer_add(NULL, server_timer_handler, 1000);
}

static void server_timer_stop(void)
{
    if (server_timer_handle) {
        sys_timeout_del(server_timer_handle);
        server_timer_handle = 0;
    }
}

void test_data_send_packet(void)
{
    u32 vaild_len = get_buffer_vaild_len(0);//获取发送buffer可写入的数据
    if (!test_data_start) {
        return;
    }

    if (vaild_len) {
        if (!app_send_user_data(TEST_SEND_HANDLE_VAL, (void *)&test_data_count, vaild_len, ATT_OP_AUTO_READ_CCC)) {
            test_data_count += vaild_len;
        }
    }
    clr_wdt();
}
#endif


static void can_send_now_wakeup(void)
{
    /* putchar('E'); */
    if (ble_resume_send_wakeup) {
        ble_resume_send_wakeup();
    }

#if TEST_SEND_DATA_RATE
    test_data_send_packet();
#endif

#if TEST_AUDIO_DATA_UPLOAD
    test_send_audio_data(0);
#endif

}

static void ble_auto_shut_down_enable(u8 enable)
{
#if TCFG_AUTO_SHUT_DOWN_TIME
    if (enable) {
        if (get_total_connect_dev() == 0) {    //已经没有设备连接
            sys_auto_shut_down_enable();
        }
    } else {
        sys_auto_shut_down_disable();
    }
#endif
}

const char *const phy_result[] = {
    "None",
    "1M",
    "2M",
    "Coded",
};

static void set_connection_data_length(u16 tx_octets, u16 tx_time)
{
    if (con_handle) {
        ble_op_set_data_length(con_handle, tx_octets, tx_time);
    }
}

static void set_connection_data_phy(u8 tx_phy, u8 rx_phy)
{
    if (0 == con_handle) {
        return;
    }

    u8 all_phys = 0;
    u16 phy_options = CONN_SET_PHY_OPTIONS_S8;

    ble_op_set_ext_phy(con_handle, all_phys, tx_phy, rx_phy, phy_options);
}

static void server_profile_start(u16 con_handle)
{
#if BT_FOR_APP_EN
    set_app_connect_type(TYPE_BLE);
#endif
    ble_op_att_send_init(con_handle, att_ram_buffer, ATT_RAM_BUFSIZE, ATT_LOCAL_PAYLOAD_SIZE);
    set_ble_work_state(BLE_ST_CONNECT);
    ble_auto_shut_down_enable(0);

    /* set_connection_data_phy(CONN_SET_CODED_PHY, CONN_SET_CODED_PHY); */
}

_WEAK_
u8 ble_update_get_ready_jump_flag(void)
{
    return 0;
}

#if TRANS_HID_EN
//回连状态，主动使能通知
static void resume_all_ccc_enable(u8 update_request)
{
    log_info("resume_all_ccc_enable\n");
    att_set_ccc_config(ATT_CHARACTERISTIC_2a4d_01_CLIENT_CONFIGURATION_HANDLE, ATT_OP_NOTIFY);
    att_set_ccc_config(ATT_CHARACTERISTIC_2a4d_02_CLIENT_CONFIGURATION_HANDLE, ATT_OP_NOTIFY);

    att_set_ccc_config(ATT_CHARACTERISTIC_2a4d_04_CLIENT_CONFIGURATION_HANDLE, ATT_OP_NOTIFY);
    att_set_ccc_config(ATT_CHARACTERISTIC_2a4d_05_CLIENT_CONFIGURATION_HANDLE, ATT_OP_NOTIFY);
    att_set_ccc_config(ATT_CHARACTERISTIC_2a4d_06_CLIENT_CONFIGURATION_HANDLE, ATT_OP_NOTIFY);
    att_set_ccc_config(ATT_CHARACTERISTIC_2a4d_07_CLIENT_CONFIGURATION_HANDLE, ATT_OP_NOTIFY);

    att_set_ccc_config(ATT_CHARACTERISTIC_2a05_01_CLIENT_CONFIGURATION_HANDLE, ATT_OP_NOTIFY);

    att_set_ccc_config(ATT_CHARACTERISTIC_2a05_01_CLIENT_CONFIGURATION_HANDLE, ATT_OP_NOTIFY);
    att_set_ccc_config(ATT_CHARACTERISTIC_2a19_01_CLIENT_CONFIGURATION_HANDLE, ATT_OP_NOTIFY);
    att_set_ccc_config(ATT_CHARACTERISTIC_2a33_01_CLIENT_CONFIGURATION_HANDLE, ATT_OP_NOTIFY);


    set_ble_work_state(BLE_ST_NOTIFY_IDICATE);
    if (update_request) {
        check_connetion_updata_deal();
        ble_op_latency_skip(con_handle, 10);
    }
}

#endif

/*
 * @section Packet Handler
 *
 * @text The packet handler is used to:
 *        - stop the counter after a disconnect
 *        - send a notification when the requested ATT_EVENT_CAN_SEND_NOW is received
 */

/* LISTING_START(packetHandler): Packet Handler */
static void cbk_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    int mtu;
    u32 tmp;
    u8 status;
    const char *attribute_name;

    switch (packet_type) {
    case HCI_EVENT_PACKET:
        switch (hci_event_packet_get_type(packet)) {

        /* case DAEMON_EVENT_HCI_PACKET_SENT: */
        /* break; */
        case ATT_EVENT_HANDLE_VALUE_INDICATION_COMPLETE:
            log_info("ATT_EVENT_HANDLE_VALUE_INDICATION_COMPLETE\n");
        case ATT_EVENT_CAN_SEND_NOW:
            can_send_now_wakeup();
            break;

        case HCI_EVENT_LE_META:
            switch (hci_event_le_meta_get_subevent_code(packet)) {
            case HCI_SUBEVENT_LE_ENHANCED_CONNECTION_COMPLETE:
                status = hci_subevent_le_enhanced_connection_complete_get_status(packet);
                if (status) {
                    log_info("LE_SLAVE CONNECTION FAIL!!! %0x\n", status);
                    set_ble_work_state(BLE_ST_DISCONN);
                    break;
                }
                con_handle = hci_subevent_le_enhanced_connection_complete_get_connection_handle(packet);
                log_info("HCI_SUBEVENT_LE_ENHANCED_CONNECTION_COMPLETE : %0x\n", con_handle);
                log_info("conn_interval = %d\n", hci_subevent_le_enhanced_connection_complete_get_conn_interval(packet));
                log_info("conn_latency = %d\n", hci_subevent_le_enhanced_connection_complete_get_conn_latency(packet));
                log_info("conn_timeout = %d\n", hci_subevent_le_enhanced_connection_complete_get_supervision_timeout(packet));
                server_profile_start(con_handle);
                break;

            case HCI_SUBEVENT_LE_CONNECTION_COMPLETE:
                con_handle = hci_subevent_le_connection_complete_get_connection_handle(packet);
                log_info("HCI_SUBEVENT_LE_CONNECTION_COMPLETE: %0x\n", con_handle);
                connection_update_complete_success(packet + 8);
                server_profile_start(con_handle);
#if RCSP_BTMATE_EN
#if (defined(BT_CONNECTION_VERIFY) && (0 == BT_CONNECTION_VERIFY))
                JL_rcsp_auth_reset();
#endif
                //rcsp_dev_select(RCSP_BLE);
                rcsp_init();
#endif
                first_pair_flag = 0;
                memcpy(cur_peer_addr_info, packet + 7, 7);
                u8 answer_data = 0x01;
                my_cmd_data_send(GX_SEND_HID_STATUS, &answer_data, 1);

                break;

            case HCI_SUBEVENT_LE_CONNECTION_UPDATE_COMPLETE:
                connection_update_complete_success(packet);
                break;

            case HCI_SUBEVENT_LE_DATA_LENGTH_CHANGE:
                log_info("APP HCI_SUBEVENT_LE_DATA_LENGTH_CHANGE\n");
                /* set_connection_data_phy(CONN_SET_CODED_PHY, CONN_SET_CODED_PHY); */
                break;

            case HCI_SUBEVENT_LE_PHY_UPDATE_COMPLETE:
                log_info("APP HCI_SUBEVENT_LE_PHY_UPDATE %s\n", hci_event_le_meta_get_phy_update_complete_status(packet) ? "Fail" : "Succ");
                log_info("Tx PHY: %s\n", phy_result[hci_event_le_meta_get_phy_update_complete_tx_phy(packet)]);
                log_info("Rx PHY: %s\n", phy_result[hci_event_le_meta_get_phy_update_complete_rx_phy(packet)]);
                break;
            }
            break;

        case HCI_EVENT_DISCONNECTION_COMPLETE:
            log_info("HCI_EVENT_DISCONNECTION_COMPLETE: %0x\n", packet[5]);
#if RCSP_BTMATE_EN
            rcsp_exit();
#endif
            con_handle = 0;
            ble_op_att_send_init(con_handle, 0, 0, 0);
            set_ble_work_state(BLE_ST_DISCONN);

            if (!ble_update_get_ready_jump_flag()) {
                bt_ble_adv_enable(1);
            }
            connection_update_cnt = 0;
#if BT_FOR_APP_EN
            set_app_connect_type(TYPE_NULL);
#endif
        ble_auto_shut_down_enable(1);

            u8 answer_data = 0x00;
            my_cmd_data_send(GX_SEND_HID_STATUS, &answer_data, 1);
            break;

        case ATT_EVENT_MTU_EXCHANGE_COMPLETE:
            mtu = att_event_mtu_exchange_complete_get_MTU(packet) - 3;
            log_info("ATT MTU = %u\n", mtu);
            ble_op_att_set_send_mtu(mtu);
            /* set_connection_data_length(251, 2120); */
            break;

        case HCI_EVENT_VENDOR_REMOTE_TEST:
            log_info("--- HCI_EVENT_VENDOR_REMOTE_TEST\n");
            break;

        case L2CAP_EVENT_CONNECTION_PARAMETER_UPDATE_RESPONSE:
            tmp = little_endian_read_16(packet, 4);
            log_info("-update_rsp: %02x\n", tmp);
            if (tmp) {
                connection_update_cnt++;
                log_info("remoter reject!!!\n");
                check_connetion_updata_deal();
            } else {
                connection_update_cnt = CONN_PARAM_TABLE_CNT;
            }
            break;

        case HCI_EVENT_ENCRYPTION_CHANGE:
            log_info("HCI_EVENT_ENCRYPTION_CHANGE= %d\n", packet[2]);
            if (!packet[2]) {
                if (first_pair_flag) {
                    //只在配对时启动检查
                } else {
                    resume_all_ccc_enable(1);
                }
            }
            break;

#if TRANS_ANCS_EN
        case HCI_EVENT_ANCS_META:
            switch (hci_event_ancs_meta_get_subevent_code(packet)) {
            case ANCS_SUBEVENT_CLIENT_NOTIFICATION:
                printf("ANCS_SUBEVENT_CLIENT_NOTIFICATION \n");
                attribute_name = ancs_client_attribute_name_for_id(ancs_subevent_client_notification_get_attribute_id(packet));
                if (!attribute_name) {
                    printf("ancs unknow attribute_id :%d \n", ancs_subevent_client_notification_get_attribute_id(packet));
                    break;
                } else {
                    u16 attribute_strlen = little_endian_read_16(packet, 7);
                    u8 *attribute_str = (void *)little_endian_read_32(packet, 9);
                    printf("Notification: %s - %s \n", attribute_name, attribute_str);
                }
                break;
            default:
                break;
            }

            break;
#endif

        }
        break;
    }
}


/* LISTING_END */

/*
 * @section ATT Read
 *
 * @text The ATT Server handles all reads to constant data. For dynamic data like the custom characteristic, the registered
 * att_read_callback is called. To handle long characteristics and long reads, the att_read_callback is first called
 * with buffer == NULL, to request the total value length. Then it will be called again requesting a chunk of the value.
 * See Listing attRead.
 */

/* LISTING_START(attRead): ATT Read */

// ATT Client Read Callback for Dynamic Data
// - if buffer == NULL, don't copy data, just return size of value
// - if buffer != NULL, copy data and return number bytes copied
// @param offset defines start of attribute value
static uint16_t att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{

    uint16_t  att_value_len = 0;
    uint16_t handle = att_handle;

    log_info("read_callback, handle= 0x%04x,buffer= %08x\n", handle, (u32)buffer);

    switch (handle) {
        
#if TRANS_HID_EN
    case ATT_CHARACTERISTIC_2a50_01_VALUE_HANDLE:
        log_info("read PnP_ID\n");
        att_value_len = sizeof(PnP_ID);
        if ((offset >= att_value_len) || (offset + buffer_size) > att_value_len) {
            break;
        }
        if (buffer) {
            memcpy(buffer, &PnP_ID[offset], buffer_size);
            att_value_len = buffer_size;
        }
        break;
    case ATT_CHARACTERISTIC_2a4a_01_VALUE_HANDLE:
        log_info("read hid info\n");
        att_value_len = sizeof(hid_information);
        if ((offset >= att_value_len) || (offset + buffer_size) > att_value_len) {
            break;
        }
        if (buffer) {
            memcpy(buffer, &hid_information[offset], buffer_size);
            att_value_len = buffer_size;
        }
        break;
    case ATT_CHARACTERISTIC_2a4b_01_VALUE_HANDLE:
        log_info("read hid map\n");
        // att_value_len = sizeof(hid_report_map);
        // if ((offset >= att_value_len) || (offset + buffer_size) > att_value_len) {
        //     break;
        // }
        // if (buffer) {
        //     memcpy(buffer, &hid_report_map[offset], buffer_size);
        //     att_value_len = buffer_size;
        // }
        if(hid_map_len == 0) //没设置过描述符
        {
            printf("1");
            att_value_len = sizeof(g_hudi_bluetooth_hid_kbd_mouse_disc);
            if ((offset >= att_value_len) || (offset + buffer_size) > att_value_len) {
                break;
            }
            if (buffer) {
                memcpy(buffer, &g_hudi_bluetooth_hid_kbd_mouse_disc[offset], buffer_size);
                att_value_len = buffer_size;
            }

        }else
        {
            printf("hid_map_len : [%d]", hid_map_len);
            att_value_len = hid_map_len;
            if ((offset >= att_value_len) || (offset + buffer_size) > att_value_len) {
                break;
            }
            if (buffer) {
                memcpy(buffer, &hid_report_map[offset], buffer_size);
                att_value_len = buffer_size;
            }
        }
        break;
    case ATT_CHARACTERISTIC_2a05_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_2a4d_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_2a4d_02_CLIENT_CONFIGURATION_HANDLE:
        if (buffer) {
            buffer[0] = att_get_ccc_config(handle);
            buffer[1] = 0;
        }
        att_value_len = 2;
        break;
#endif

    case ATT_CHARACTERISTIC_2a00_01_VALUE_HANDLE:
        att_value_len = gap_device_name_len;

        if ((offset >= att_value_len) || (offset + buffer_size) > att_value_len) {
            break;
        }

        if (buffer) {
            memcpy(buffer, &gap_device_name[offset], buffer_size);
            att_value_len = buffer_size;
            log_info("\n------read gap_name: %s \n", gap_device_name);
        }
        break;

    case ATT_CHARACTERISTIC_ae10_01_VALUE_HANDLE:
        att_value_len = sizeof(test_read_write_buf);
        if ((offset >= att_value_len) || (offset + buffer_size) > att_value_len) {
            break;
        }

        if (buffer) {
            memcpy(buffer, &test_read_write_buf[offset], buffer_size);
            att_value_len = buffer_size;
        }
        break;

    case ATT_CHARACTERISTIC_ae04_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_ae02_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_ae05_01_CLIENT_CONFIGURATION_HANDLE:
    // case ATT_CHARACTERISTIC_ae3c_01_CLIENT_CONFIGURATION_HANDLE:
        if (buffer) {
            buffer[0] = att_get_ccc_config(handle);
            buffer[1] = 0;
        }
        att_value_len = 2;
        break;

    default:
        break;
    }

    log_info("att_value_len= %d\n", att_value_len);
    return att_value_len;

}


/* LISTING_END */
/*
 * @section ATT Write
 *
 * @text The only valid ATT write in this example is to the Client Characteristic Configuration, which configures notification
 * and indication. If the ATT handle matches the client configuration handle, the new configuration value is stored and used
 * in the heartbeat handler to decide if a new value should be sent. See Listing attWrite.
 */

/* LISTING_START(attWrite): ATT Write */
static int att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    int result = 0;
    u16 tmp16;

    u16 handle = att_handle;

    log_info("write_callback, handle= 0x%04x,size = %d\n", handle, buffer_size);

    switch (handle) {

    case ATT_CHARACTERISTIC_2a00_01_VALUE_HANDLE:
        break;

    case ATT_CHARACTERISTIC_ae02_01_CLIENT_CONFIGURATION_HANDLE:
#if TEST_SEND_DATA_RATE
        if (buffer[0]) {
            server_timer_start();
        } else {
            server_timer_stop();
            test_data_start = 0;
        }
#endif
    case ATT_CHARACTERISTIC_ae04_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_ae05_01_CLIENT_CONFIGURATION_HANDLE:
    // case ATT_CHARACTERISTIC_ae3c_01_CLIENT_CONFIGURATION_HANDLE:
        set_ble_work_state(BLE_ST_NOTIFY_IDICATE);
        check_connetion_updata_deal();
        log_info("\n------write ccc:%04x,%02x\n", handle, buffer[0]);
        att_set_ccc_config(handle, buffer[0]);

#if TEST_SEND_DATA_RATE
        test_data_start = 1;//start
        if (buffer[0]) {
            test_data_send_packet();
        }
#endif
        break;

    case ATT_CHARACTERISTIC_ae10_01_VALUE_HANDLE:
        tmp16 = sizeof(test_read_write_buf);
        if ((offset >= tmp16) || (offset + buffer_size) > tmp16) {
            break;
        }
        memcpy(&test_read_write_buf[offset], buffer, buffer_size);
        break;

    case ATT_CHARACTERISTIC_ae01_01_VALUE_HANDLE:
        printf("\n-ae01_rx(%d):", buffer_size);
        printf_buf(buffer, buffer_size);

        //收发测试，自动发送收到的数据;for test
        if (app_send_user_data_check(buffer_size)) {
            app_send_user_data(ATT_CHARACTERISTIC_ae02_01_VALUE_HANDLE, buffer, buffer_size, ATT_OP_AUTO_READ_CCC);
        }

#if TEST_SEND_DATA_RATE
        if ((buffer[0] == 'A') && (buffer[1] == 'F')) {
            test_data_start = 1;//start
        } else if ((buffer[0] == 'A') && (buffer[1] == 'A')) {
            test_data_start = 0;//stop
        }
#endif
        break;

    case ATT_CHARACTERISTIC_ae03_01_VALUE_HANDLE:
        printf("\n-ae_rx(%d):", buffer_size);
        printf_buf(buffer, buffer_size);

        //收发测试，自动发送收到的数据;for test
        if (app_send_user_data_check(buffer_size)) {
            app_send_user_data(ATT_CHARACTERISTIC_ae05_01_VALUE_HANDLE, buffer, buffer_size, ATT_OP_AUTO_READ_CCC);
        }
        break;

#if RCSP_BTMATE_EN
    case ATT_CHARACTERISTIC_ae02_02_CLIENT_CONFIGURATION_HANDLE:
        ble_op_latency_skip(con_handle, HOLD_LATENCY_CNT_ALL); //
        set_ble_work_state(BLE_ST_NOTIFY_IDICATE);
#endif
        /* if ((cur_conn_latency == 0) */
        /*     && (connection_update_cnt == CONN_PARAM_TABLE_CNT) */
        /*     && (Peripheral_Preferred_Connection_Parameters[0].latency != 0)) { */
        /*     connection_update_cnt = 0; */
        /* } */
        check_connetion_updata_deal();
        log_info("\n------write ccc:%04x,%02x\n", handle, buffer[0]);
        att_set_ccc_config(handle, buffer[0]);
        break;
#if RCSP_BTMATE_EN
    case ATT_CHARACTERISTIC_ae01_02_VALUE_HANDLE:
        printf("rcsp_read:%x\n", buffer_size);
        if (app_recieve_callback) {
            app_recieve_callback(0, buffer, buffer_size);
        }
        break;
#endif

//     case ATT_CHARACTERISTIC_ae3b_01_VALUE_HANDLE:
//         printf("\n-ae3b_rx(%d):", buffer_size);
//         printf_buf(buffer, buffer_size);

// #if TEST_AUDIO_DATA_UPLOAD
//         if (0 == memcmp(buffer, "start", 5)) {
//             test_send_audio_data(1);
//         }
// #endif
//         break;

#if TRANS_HID_EN
    case ATT_CHARACTERISTIC_2a05_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_2a4d_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_2a4d_02_CLIENT_CONFIGURATION_HANDLE:
        log_info("\n------write ccc:%04x,%02x\n", handle, buffer[0]);
        att_set_ccc_config(handle, buffer[0]);
        check_connetion_updata_deal();
        break;
#endif

    default:
        break;
    }

    return 0;
}

static int app_send_user_data(u16 handle, u8 *data, u16 len, u8 handle_type)
{
    u32 ret = APP_BLE_NO_ERROR;

    if (!con_handle) {
        return APP_BLE_OPERATION_ERROR;
    }

    if (!att_get_ccc_config(handle + 1)) {
        log_info("fail,no write ccc!!!,%04x\n", handle + 1);
        return APP_BLE_NO_WRITE_CCC;
    }

    ret = ble_op_att_send_data(handle, data, len, handle_type);
    if (ret == BLE_BUFFER_FULL) {
        ret = APP_BLE_BUFF_FULL;
    }

    if (ret) {
        log_info("app_send_fail:%d !!!!!!\n", ret);
    }
    return ret;
}

#include "app_main.h"

void custom_ble_adv_data_init()
{
    u8 offset = 0;
    u8 *buf = custom_ble_adv_data.adv_data;
    offset += make_eir_packet_val(&buf[offset], offset, HCI_EIR_DATATYPE_FLAGS, 0x06, 1);
    offset += make_eir_packet_val(&buf[offset], offset, HCI_EIR_DATATYPE_COMPLETE_16BIT_SERVICE_UUIDS, 0x1812, 2);
    offset += make_eir_packet_data(&buf[offset], offset, HCI_EIR_DATATYPE_APPEARANCE_DATA, Appearance, 2);

    log_info_hexdump(buf, offset);
    custom_ble_adv_data.adv_len = offset;
}

void custom_ble_rsp_data_init()
{
    u8 offset = 0;
    u8 *buf = custom_ble_rsp_data.rsp_data;

    u8 name_len = gap_device_name_len;
    u8 vaild_len = ADV_RSP_PACKET_MAX - (offset + 2);
    if (name_len > vaild_len) {
        name_len = vaild_len;
    }
    offset += make_eir_packet_data(&buf[offset], offset, HCI_EIR_DATATYPE_COMPLETE_LOCAL_NAME, (void *)gap_device_name, name_len);

    log_info_hexdump(buf, offset);
    custom_ble_rsp_data.rsp_len = offset;
}

//------------------------------------------------------
static int make_set_adv_data(void)
{
    u8 offset = 0;
    u8 *buf = adv_data;

    memcpy(buf, custom_ble_adv_data.adv_data, custom_ble_adv_data.adv_len);
    offset += custom_ble_adv_data.adv_len;

    #if 0
    /* offset += make_eir_packet_val(&buf[offset], offset, HCI_EIR_DATATYPE_FLAGS, 0x18, 1); */
    /* offset += make_eir_packet_val(&buf[offset], offset, HCI_EIR_DATATYPE_FLAGS, 0x1A, 1); */
    offset += make_eir_packet_val(&buf[offset], offset, HCI_EIR_DATATYPE_FLAGS, 0x06, 1);
    // offset += make_eir_packet_val(&buf[offset], offset, HCI_EIR_DATATYPE_COMPLETE_16BIT_SERVICE_UUIDS, 0xAF30, 2);
    offset += make_eir_packet_val(&buf[offset], offset, HCI_EIR_DATATYPE_COMPLETE_16BIT_SERVICE_UUIDS, 0x1812, 2);
    offset += make_eir_packet_data(&buf[offset], offset, HCI_EIR_DATATYPE_APPEARANCE_DATA, Appearance, 2);
    #endif

    if (offset > ADV_RSP_PACKET_MAX) {
        puts("***adv_data overflow!!!!!!\n");
        return -1;
    }
    log_info("adv_data(%d):", offset);
    log_info_hexdump(buf, offset);
    adv_data_len = offset;
    ble_op_set_adv_data(offset, buf);
    return 0;
}

static int make_set_rsp_data(void)
{
    u8 offset = 0;
    u8 *buf = scan_rsp_data;

    memcpy(buf, custom_ble_rsp_data.rsp_data, custom_ble_rsp_data.rsp_len);
    offset += custom_ble_rsp_data.rsp_len;

#if RCSP_BTMATE_EN
    u8  tag_len = sizeof(user_tag_string);
    offset += make_eir_packet_data(&buf[offset], offset, HCI_EIR_DATATYPE_MANUFACTURER_SPECIFIC_DATA, (void *)user_tag_string, tag_len);
#endif

#if 0
    u8 name_len = gap_device_name_len;
    u8 vaild_len = ADV_RSP_PACKET_MAX - (offset + 2);
    if (name_len > vaild_len) {
        name_len = vaild_len;
    }
    offset += make_eir_packet_data(&buf[offset], offset, HCI_EIR_DATATYPE_COMPLETE_LOCAL_NAME, (void *)gap_device_name, name_len);
#endif

    if (offset > ADV_RSP_PACKET_MAX) {
        puts("***rsp_data overflow!!!!!!\n");
        return -1;
    }
    log_info("rsp_data(%d):", offset);
    log_info_hexdump(buf, offset);
    scan_rsp_data_len = offset;
    ble_op_set_rsp_data(offset, buf);
    return 0;
}

//广播参数设置
static void advertisements_setup_init()
{
    uint8_t adv_type = ADV_IND;
    uint8_t adv_channel = ADV_CHANNEL_ALL;
    int   ret = 0;

    ble_op_set_adv_param(ADV_INTERVAL_MIN, adv_type, adv_channel);

    ret |= make_set_adv_data();
    ret |= make_set_rsp_data();

    if (ret) {
        puts("advertisements_setup_init fail !!!!!!\n");
        return;
    }

}

#define PASSKEY_ENTER_ENABLE      0 //输入passkey使能，可修改passkey
//重设passkey回调函数，在这里可以重新设置passkey
//passkey为6个数字组成，十万位、万位。。。。个位 各表示一个数字 高位不够为0
static void reset_passkey_cb(u32 *key)
{
#if 1
    u32 newkey = rand32();//获取随机数

    newkey &= 0xfffff;
    if (newkey > 999999) {
        newkey = newkey - 999999; //不能大于999999
    }
    *key = newkey; //小于或等于六位数
    printf("set new_key= %06u\n", *key);
#else
    *key = 123456; //for debug
#endif
}


int ble_list_smp_pair_set(u8 mode)
{
    log_info("%s[mode:%d  get_count:%d]", __func__, mode, ble_list_get_count());
    put_buf(cur_peer_addr_info, 7); //0--public address,1--random address
    int ret = 0;
    switch(mode){
    case 0:
        ble_list_pair_accept(0);//此接口有VM记忆,影响到下次开机
        break;
    case 1:
        ble_list_pair_accept(1);
        break;
    case 2:
        ret = ble_list_bonding_remote(cur_peer_addr_info + 1, cur_peer_addr_info[0]);
        break;
    case 3:
        ret = ble_list_delete_device(cur_peer_addr_info + 1, cur_peer_addr_info[0]);
        break;
    case 4:
        ble_list_clear_all();
        ble_list_pair_accept(1);
        break;
    }
    return 0;
}

void ble_sm_setup_init(io_capability_t io_type, u8 auth_req, uint8_t min_key_size, u8 security_en)
{
    //setup SM: Display only
    sm_init();
    sm_set_io_capabilities(io_type);
    sm_set_authentication_requirements(auth_req);
    sm_set_encryption_key_size_range(min_key_size, 16);
    // sm_set_request_security(security_en);
    sm_event_callback_set(&cbk_sm_packet_handler);

    if (io_type == IO_CAPABILITY_DISPLAY_ONLY) {
        reset_PK_cb_register(reset_passkey_cb);
    }
}


void ble_profile_init(void)
{
    printf("ble profile init\n");
    le_device_db_init();

#if PASSKEY_ENTER_ENABLE
    ble_sm_setup_init(IO_CAPABILITY_DISPLAY_ONLY, SM_AUTHREQ_MITM_PROTECTION, 7, TCFG_BLE_SECURITY_EN);
#else
    ble_sm_setup_init(IO_CAPABILITY_NO_INPUT_NO_OUTPUT, SM_AUTHREQ_BONDING, 7, TCFG_BLE_SECURITY_EN);
#endif

    /* setup ATT server */
    att_server_init(profile_data, att_read_callback, att_write_callback);
    att_server_register_packet_handler(cbk_packet_handler);
    /* gatt_client_register_packet_handler(packet_cbk); */

    // register for HCI events
    hci_event_callback_set(&cbk_packet_handler);
    /* ble_l2cap_register_packet_handler(packet_cbk); */
    /* sm_event_packet_handler_register(packet_cbk); */
    le_l2cap_register_packet_handler(&cbk_packet_handler);

#if TRANS_ANCS_EN
    //setup GATT client
    gatt_client_init();

    //setup ANCS clent
    ancs_client_init();
    ancs_set_notification_buffer(ancs_info_buffer, sizeof(ancs_info_buffer));
    ancs_client_register_callback(&cbk_packet_handler);
#endif

}

#if EXT_ADV_MODE_EN


#define EXT_ADV_NAME                    'J', 'L', '_', 'E', 'X', 'T', '_', 'A', 'D', 'V'
/* #define EXT_ADV_NAME                    "JL_EXT_ADV" */
#define BYTE_LEN(x...)                  sizeof((u8 []) {x})
#define EXT_ADV_DATA                    \
    0x02, 0x01, 0x06, \
    0x03, 0x02, 0xF0, 0xFF, \
    BYTE_LEN(EXT_ADV_NAME) + 1, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME, EXT_ADV_NAME

struct ext_advertising_param {
    u8 Advertising_Handle;
    u16 Advertising_Event_Properties;
    u8 Primary_Advertising_Interval_Min[3];
    u8 Primary_Advertising_Interval_Max[3];
    u8 Primary_Advertising_Channel_Map;
    u8 Own_Address_Type;
    u8 Peer_Address_Type;
    u8 Peer_Address[6];
    u8 Advertising_Filter_Policy;
    u8 Advertising_Tx_Power;
    u8 Primary_Advertising_PHY;
    u8 Secondary_Advertising_Max_Skip;
    u8 Secondary_Advertising_PHY;
    u8 Advertising_SID;
    u8 Scan_Request_Notification_Enable;
} _GNU_PACKED_;

struct ext_advertising_data  {
    u8 Advertising_Handle;
    u8 Operation;
    u8 Fragment_Preference;
    u8 Advertising_Data_Length;
    u8 Advertising_Data[BYTE_LEN(EXT_ADV_DATA)];
} _GNU_PACKED_;

struct ext_advertising_enable {
    u8  Enable;
    u8  Number_of_Sets;
    u8  Advertising_Handle;
    u16 Duration;
    u8  Max_Extended_Advertising_Events;
} _GNU_PACKED_;

const struct ext_advertising_param ext_adv_param = {
    .Advertising_Handle = 0,
    .Advertising_Event_Properties = 1,
    .Primary_Advertising_Interval_Min = {30, 0, 0},
    .Primary_Advertising_Interval_Max = {30, 0, 0},
    .Primary_Advertising_Channel_Map = 7,
    .Primary_Advertising_PHY = ADV_SET_1M_PHY,
    .Secondary_Advertising_PHY = ADV_SET_1M_PHY,
};

const struct ext_advertising_data ext_adv_data = {
    .Advertising_Handle = 0,
    .Operation = 3,
    .Fragment_Preference = 0,
    .Advertising_Data_Length = BYTE_LEN(EXT_ADV_DATA),
    .Advertising_Data = EXT_ADV_DATA,
};

const struct ext_advertising_enable ext_adv_enable = {
    .Enable = 1,
    .Number_of_Sets = 1,
    .Advertising_Handle = 0,
    .Duration = 0,
    .Max_Extended_Advertising_Events = 0,
};

const struct ext_advertising_enable ext_adv_disable = {
    .Enable = 0,
    .Number_of_Sets = 1,
    .Advertising_Handle = 0,
    .Duration = 0,
    .Max_Extended_Advertising_Events = 0,
};

#endif /* EXT_ADV_MODE_EN */

static int set_adv_enable(void *priv, u32 en)
{
    ble_state_e next_state, cur_state;

    // 检查广播控制使能标志，如果未允许控制广播，则返回操作错误
    if (!adv_ctrl_en) {
        return APP_BLE_OPERATION_ERROR;
    }

    // 如果当前已有连接（con_handle 不为 0），则不允许在此状态下更改广播使能，返回错误
    if (con_handle) {
        return APP_BLE_OPERATION_ERROR;
    }

    // 根据传入的 en（使能/禁用）参数，设定目标状态：开启对应广播状态，关闭对应空闲状态
    if (en) {
        next_state = BLE_ST_ADV;
    } else {
        next_state = BLE_ST_IDLE;
    }

    // 获取当前 BLE 模块的工作状态
    cur_state =  get_ble_work_state();
    switch (cur_state) {
    case BLE_ST_ADV:
    case BLE_ST_IDLE:
    case BLE_ST_INIT_OK:
    case BLE_ST_NULL:
    case BLE_ST_DISCONN:
        break; // 如果当前状态在上述允许范围内，则继续执行
    default:
        return APP_BLE_OPERATION_ERROR; // 处于其他不允许的状态时，返回操作错误
        break;
    }

    // 如果当前状态与目标状态一致，则无需重复操作，直接返回无错误
    if (cur_state == next_state) {
        return APP_BLE_NO_ERROR;
    }
    log_info("adv_en:%d\n", en); // 打印广播使能状态变更日志
    set_ble_work_state(next_state); // 更新 BLE 模块的工作状态

#if EXT_ADV_MODE_EN
    // 如果启用了扩展广播模式（Extended Advertising）
    if (en) {
        ble_op_set_ext_adv_param(&ext_adv_param, sizeof(ext_adv_param)); // 设置扩展广播参数

        log_info_hexdump(&ext_adv_data, sizeof(ext_adv_data));
        ble_op_set_ext_adv_data(&ext_adv_data, sizeof(ext_adv_data));       // 设置扩展广播数据并打印内存转储
        
        ble_op_set_ext_adv_enable(&ext_adv_enable, sizeof(ext_adv_enable)); // 使能扩展广播
    } else {
        ble_op_set_ext_adv_enable(&ext_adv_disable, sizeof(ext_adv_disable)); // 禁用扩展广播
    }
#else
    // 普通广播模式
    if (en) {
        advertisements_setup_init(); // 如果是使能广播，则初始化广播相关配置
    }
    ble_op_adv_enable(en); // 调用底层接口开启或关闭普通广播
#endif /* EXT_ADV_MODE_EN */

    return APP_BLE_NO_ERROR; // 操作成功，返回无错误码
}

static int ble_disconnect(void *priv)
{
    if (con_handle) {
        if (BLE_ST_SEND_DISCONN != get_ble_work_state()) {
            log_info(">>>ble send disconnect\n");
            set_ble_work_state(BLE_ST_SEND_DISCONN);
            ble_op_disconnect(con_handle);
        } else {
            log_info(">>>ble wait disconnect...\n");
        }
        return APP_BLE_NO_ERROR;
    } else {
        return APP_BLE_OPERATION_ERROR;
    }
}


static int get_buffer_vaild_len(void *priv)
{
    u32 vaild_len = 0;
    ble_op_att_get_remain(&vaild_len);
    return vaild_len;
}

static int app_send_user_data_do(void *priv, u8 *data, u16 len)
{
#if PRINT_DMA_DATA_EN
    if (len < 128) {
        log_info("-le_tx(%d):");
        log_info_hexdump(data, len);
    } else {
        putchar('L');
    }
#endif
    return app_send_user_data(ATT_CHARACTERISTIC_ae02_01_VALUE_HANDLE, data, len, ATT_OP_AUTO_READ_CCC);
}

static int app_send_user_data_check(u16 len)
{
    u32 buf_space = get_buffer_vaild_len(0);
    if (len <= buf_space) {
        return 1;
    }
    return 0;
}


static int regiest_wakeup_send(void *priv, void *cbk)
{
    ble_resume_send_wakeup = cbk;
    return APP_BLE_NO_ERROR;
}

static int regiest_recieve_cbk(void *priv, void *cbk)
{
    channel_priv = (u32)priv;
    app_recieve_callback = cbk;
    return APP_BLE_NO_ERROR;
}

static int regiest_state_cbk(void *priv, void *cbk)
{
    channel_priv = (u32)priv;
    app_ble_state_callback = cbk;
    return APP_BLE_NO_ERROR;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------


u8 *ble_get_scan_rsp_ptr(u16 *len)
{
    if (len) {
        *len = scan_rsp_data_len;
    }
    return scan_rsp_data;
}

u8 *ble_get_adv_data_ptr(u16 *len)
{
    if (len) {
        *len = adv_data_len;
    }
    return adv_data;
}

u8 *ble_get_gatt_profile_data(u16 *len)
{
    *len = sizeof(profile_data);
    return (u8 *)profile_data;
}


void bt_ble_adv_enable(u8 enable)
{
    set_adv_enable(0, enable);
}

u16 bt_ble_is_connected(void)
{
    return con_handle; // 返回当前的蓝牙连接句柄（如果不为 0 则表示已连接，为 0 表示未连接）
}

void ble_module_enable(u8 en)
{
    log_info("mode_en:%d\n", en); // 打印当前 BLE 模块使能状态日志
    if (en) {
        adv_ctrl_en = 1;          // 开启广播控制标志
        bt_ble_adv_enable(1);     // 使能并启动蓝牙 BLE 广播
    } else {
        if (con_handle) {
            adv_ctrl_en = 0;      // 清除广播控制标志
            printf("ble disconnect\n");
            ble_disconnect(NULL); // 如果当前处于连接状态，则断开蓝牙连接
        } else {
            printf("ble adv disable\n");
            bt_ble_adv_enable(0); // 如果未连接，则直接关闭蓝牙 BLE 广播
            adv_ctrl_en = 0;      // 清除广播控制标志
        }
    }
}
static const char ble_ext_name[] = "(BLE)";

void bt_ble_init(void)
{
    log_info("***** ble_init******\n");
    char *name_p;
    u8 ext_name_len = sizeof(ble_ext_name) - 1;

    // name_p = bt_get_local_name();
    name_p =ble_get_local_name();
    gap_device_name_len = strlen(name_p);
    if (gap_device_name_len > BT_NAME_LEN_MAX - ext_name_len) {
        gap_device_name_len = BT_NAME_LEN_MAX - ext_name_len;
    }

    //增加后缀，区分名字
    memcpy(gap_device_name, name_p, gap_device_name_len);
    memcpy(&gap_device_name[gap_device_name_len], "(BLE)", ext_name_len);
    gap_device_name_len += ext_name_len;

    log_info("ble name(%d): %s \n", gap_device_name_len, gap_device_name);

    set_ble_work_state(BLE_ST_INIT_OK);
    extern u8 hc_reset_source;
    syscfg_read(CFG_HC_BT_OPEN_STATSUS, &hc_open_bt, sizeof(HC_BT_OPEN));

    custom_ble_adv_data_init();
    custom_ble_rsp_data_init();
    
    if(hc_open_bt.ble_open == 1 && hc_reset_source & BIT(6))
    {
        ble_module_enable(1);
    }else
    {
        ble_module_enable(0);
    }


#if TEST_SEND_DATA_RATE
    server_timer_start();
#endif
}

void bt_ble_exit(void)
{
    log_info("***** ble_exit******\n");

    ble_module_enable(0);

#if TEST_SEND_DATA_RATE
    server_timer_stop();
#endif

}


void ble_app_disconnect(void)
{
    ble_disconnect(NULL);
}

#if RCSP_BTMATE_EN
static int rcsp_send_user_data_do(void *priv, u8 *data, u16 len)
{
    log_info("rcsp_tx:%x\n", len);
#if PRINT_DMA_DATA_EN
    if (len < 128) {
        log_info("-dma_tx(%d):");
        log_info_hexdump(data, len);
    } else {
        putchar('L');
    }
#endif
    return app_send_user_data(ATT_CHARACTERISTIC_ae02_02_VALUE_HANDLE, data, len, ATT_OP_AUTO_READ_CCC);
}
#endif

#if RCSP_BTMATE_EN
static const struct ble_server_operation_t mi_ble_operation = {
    .adv_enable = set_adv_enable,
    .disconnect = ble_disconnect,
    .get_buffer_vaild = get_buffer_vaild_len,
    .send_data = (void *)rcsp_send_user_data_do,
    .regist_wakeup_send = regiest_wakeup_send,
    .regist_recieve_cbk = regiest_recieve_cbk,
    .regist_state_cbk = regiest_state_cbk,
};
#else
static const struct ble_server_operation_t mi_ble_operation = {
    .adv_enable = set_adv_enable,
    .disconnect = ble_disconnect,
    .get_buffer_vaild = get_buffer_vaild_len,
    .send_data = (void *)app_send_user_data_do,
    .regist_wakeup_send = regiest_wakeup_send,
    .regist_recieve_cbk = regiest_recieve_cbk,
    .regist_state_cbk = regiest_state_cbk,
};
#endif

void ble_get_server_operation_table(struct ble_server_operation_t **interface_pt)
{
    *interface_pt = (void *)&mi_ble_operation;
}

void ble_server_send_test_key_num(u8 key_num)
{
    ;
}

void user_hid_data_send(u8 report_id,u8*buff,u16 len)
{
    printf("report id : [%d]\r\n", report_id);
    put_buf(buff, len);
    int ret =  hid_send_data(report_id, buff, len);
    printf("user_hid_data_send -- ret: %d\n",ret);
}


void gx_ble_name_set()    
{
    BI_OFF_APP();
    char *name_p;
    u8 ext_name_len = sizeof(ble_ext_name) - 1; // 获取扩展名长度
    name_p = ble_get_local_name();              // 获取 BLE 本地名称
    gap_device_name_len = strlen(name_p);       // 计算名称长度
    if (gap_device_name_len > BT_NAME_LEN_MAX - ext_name_len) { // 检查长度限制
        gap_device_name_len = BT_NAME_LEN_MAX - ext_name_len;   // 截断
    }
    //增加后缀，区分名字
    memcpy(gap_device_name, name_p, gap_device_name_len);                       // 复制原名称
    memcpy(&gap_device_name[gap_device_name_len], "(BLE)", ext_name_len);       // 复制后缀
    gap_device_name_len += ext_name_len;                                        // 更新总长度

    log_info("ble name(%d): %s \n", gap_device_name_len, gap_device_name);      // 打印日志
    BT_ON_APP();
}
#endif


