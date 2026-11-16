
/*************************************************************
   此文件函数主要是蓝牙模式各种状态处理

	void app_bt_task()
   bt模式主函数

	static int bt_sys_event_handler(struct sys_event *event)
   bt模式系统事件所有处理入口

	static void bt_task_close(void)
	bt模式退出


	void bt_function_select_init()
    蓝牙功能配置

	void bredr_handle_register()
 	蓝牙协议栈回调函数

	static int bt_connction_status_event_handler(struct bt_event *bt)
	蓝牙状态事件处理函数

	static int bt_hci_event_handler(struct bt_event *bt)
	蓝牙协议栈事件回调，app处理函数

	static int bt_ai_event_handler(struct bt_event *bt)
	蓝牙ai事件处理函数

	int bt_background_event_handler_filter(struct sys_event *event)
	蓝牙后台事件过滤处理
	int bt_background_event_handler(struct sys_event *event)
	蓝牙后台事件处理函数

	int bt_key_event_handler(struct sys_event *event)
	蓝牙按键处理函数

	avctp_user.h 有蓝牙支持的命令介绍

**************************************************************/



#include "system/includes.h"
#include "media/includes.h"

#include "app_config.h"
#include "app_task.h"

#include "btstack/avctp_user.h"
#include "btstack/btstack_task.h"
#include "btstack/bluetooth.h"
#include "btstack/btstack_error.h"
#include "btctrler/btctrler_task.h"
#include "classic/hci_lmp.h"

#include "bt/bt_tws.h"
#include "bt/bt_ble.h"
#include "bt/bt.h"
#include "bt/vol_sync.h"
#include "bt/bt_emitter.h"
#include "bt_common.h"
#include "aec_user.h"

#include "math.h"
#include "spp_user.h"


#include "app_protocol_common.h"

#include "app_chargestore.h"
#include "app_charge.h"
#include "app_main.h"
#include "app_power_manage.h"
#include "user_cfg.h"

#include "asm/pwm_led.h"
#include "asm/timer.h"
#include "asm/hwi.h"
#include "cpu.h"

#include "ui/ui_api.h"
#include "ui_manage.h"
#include "ui/ui_style.h"

#include "key_event_deal.h"
#include "clock_cfg.h"
#include "gSensor/gSensor_manage.h"
#include "soundcard/soundcard.h"

#include "audio_dec.h"
#include "tone_player.h"
#include "dac.h"
#include "soundbox.h"
#include "le_client_demo.h"
#include "tuya_multi/tuya_le_multi_common.h"
#include "multi_demo/le_multi_common.h"


#define LOG_TAG_CONST        BT
#define LOG_TAG             "[BT]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"


#if TCFG_APP_BT_EN

struct app_bt_opr app_bt_hdl = {
    .exit_flag = 1,
    .replay_tone_flag = 1,
    .esco_dump_packet = ESCO_DUMP_PACKET_CALL,
    .hid_mode = 0,
    .wait_exit = 0,
};

#define __this 	(&app_bt_hdl)

BT_USER_PRIV_VAR bt_user_priv_var;



/*----------------------------------------------------------------------------*/
/**@brief    蓝牙模式变量初始化
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_var_init()
{
    memset((u8 *)&bt_user_priv_var, 0, sizeof(BT_USER_PRIV_VAR));
}


/*----------------------------------------------------------------------------*/
/**@brief    蓝牙模式协议栈回调函数
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
void bredr_handle_register()
{
#if (USER_SUPPORT_PROFILE_SPP==1)
#if APP_ONLINE_DEBUG
    extern void online_spp_init(void);
    spp_data_deal_handle_register(user_spp_data_handler);
    online_spp_init();
#else
    spp_data_deal_handle_register(spp_data_handler);
#endif
#endif

#if BT_SUPPORT_MUSIC_VOL_SYNC
    ///蓝牙音乐和通话音量同步
    music_vol_change_handle_register(bt_set_music_device_volume, phone_get_device_vol);
#endif

#if BT_SUPPORT_DISPLAY_BAT
    ///电量显示获取电量的接口
    get_battery_value_register(bt_get_battery_value);
#endif

    ///被测试盒链接上进入快速测试回调
    bt_fast_test_handle_register(bt_fast_test_api);

    ///样机进入dut被测试仪器链接上回调
    bt_dut_test_handle_register(bt_dut_api);

    ///获取远端设备蓝牙名字回调
    read_remote_name_handle_register(bt_read_remote_name);

    ////获取歌曲信息回调
    printf("蓝牙歌词获取初始化 -------- \r\n");
    bt_music_info_handle_register(user_get_bt_music_info);

    BT_BREDR_HANDLE_REG();
}


#if (TRANS_MULTI_BLE_EN || TUYA_MULTI_BLE_EN) && TRANS_MULTI_BLE_MASTER_NUMS
//指定搜索uuid
//指定搜索uuid
static const target_uuid_t  jl_search_uuid_table[] = {

    // for uuid16
    // PRIMARY_SERVICE, ae30
    // CHARACTERISTIC,  ae01, WRITE_WITHOUT_RESPONSE | DYNAMIC,
    // CHARACTERISTIC,  ae02, NOTIFY,

    {
        .services_uuid16 = 0xae30,
        .characteristic_uuid16 = 0xae01,
        .opt_type = ATT_PROPERTY_WRITE_WITHOUT_RESPONSE,
    },

    {
        .services_uuid16 = 0xae30,
        .characteristic_uuid16 = 0xae02,
        .opt_type = ATT_PROPERTY_NOTIFY,
    },

    //for uuid128,sample
    //	PRIMARY_SERVICE, 0000F530-1212-EFDE-1523-785FEABCD123
    //	CHARACTERISTIC,  0000F531-1212-EFDE-1523-785FEABCD123, NOTIFY,
    //	CHARACTERISTIC,  0000F532-1212-EFDE-1523-785FEABCD123, WRITE_WITHOUT_RESPONSE | DYNAMIC,
    /*
       {
       .services_uuid16 = 0,
       .services_uuid128 =       {0x00,0x00,0xF5,0x30 ,0x12,0x12 ,0xEF, 0xDE ,0x15,0x23 ,0x78,0x5F,0xEA ,0xBC,0xD1,0x23} ,
       .characteristic_uuid16 = 0,
       .characteristic_uuid128 = {0x00,0x00,0xF5,0x31 ,0x12,0x12 ,0xEF, 0xDE ,0x15,0x23 ,0x78,0x5F,0xEA ,0xBC,0xD1,0x23},
       .opt_type = ATT_PROPERTY_NOTIFY,
       },

       {
       .services_uuid16 = 0,
       .services_uuid128 =       {0x00,0x00,0xF5,0x30 ,0x12,0x12 ,0xEF, 0xDE ,0x15,0x23 ,0x78,0x5F,0xEA ,0xBC,0xD1,0x23} ,
       .characteristic_uuid16 = 0,
       .characteristic_uuid128 = {0x00,0x00,0xF5,0x32 ,0x12,0x12 ,0xEF, 0xDE ,0x15,0x23 ,0x78,0x5F,0xEA ,0xBC,0xD1,0x23},
       .opt_type = ATT_PROPERTY_WRITE_WITHOUT_RESPONSE,
       },
     */

};


static void ble_report_data_deal(att_data_report_t *report_data, target_uuid_t *search_uuid)
{
    log_info("conn_handle:%04x,report_data:%02x,%02x,%d,len(%d)", report_data->conn_handle, report_data->packet_type,
             report_data->value_handle, report_data->value_offset, report_data->blob_length);

    log_info_hexdump(report_data->blob, report_data->blob_length);

    /* if (search_uuid == NULL) { */
    /* log_info("not_match handle"); */
    /* return; */
    /* } */

    switch (report_data->packet_type) {
    case GATT_EVENT_NOTIFICATION:  //notify
        break;

    case GATT_EVENT_INDICATION://indicate
    case GATT_EVENT_CHARACTERISTIC_VALUE_QUERY_RESULT://read
        break;

    case GATT_EVENT_LONG_CHARACTERISTIC_VALUE_QUERY_RESULT://read long
        break;

    default:
        break;
    }
}

static struct ble_client_operation_t *ble_client_api;
static const u8 test_remoter_name1[] = "AC897N_MX(BLE)";//
/* static const u8 test_remoter_name2[] = "AC630N_HID567(BLE)";// */
static u16 ble_client_write_handle;
static u16 ble_client_timer = 0;

static const client_match_cfg_t match_dev01 = {
    .create_conn_mode = BIT(CLI_CREAT_BY_NAME),
    .compare_data_len = sizeof(test_remoter_name1) - 1, //去结束符
    .compare_data = test_remoter_name1,
    .bonding_flag = 0,
};

/* static const client_match_cfg_t match_dev02 = { */
/* .create_conn_mode = BIT(CLI_CREAT_BY_NAME), */
/* .compare_data_len = sizeof(test_remoter_name2) - 1, //去结束符 */
/* .compare_data = test_remoter_name2, */
/* .bonding_flag = 1, */
/* }; */

static void client_test_write(void)
{
    static u32 count = 0;
    int i, ret = 0;
    u16 tmp_handle;

    count++;
    for (i = 0; i < TRANS_MULTI_BLE_MASTER_NUMS; i++) {
        tmp_handle = mul_dev_get_conn_handle(i, MULTI_ROLE_CLIENT);

        if (tmp_handle && ble_client_write_handle) {
            ret = ble_client_api->opt_comm_send_ext(tmp_handle, ble_client_write_handle, &count, 16, ATT_OP_WRITE_WITHOUT_RESPOND);
            log_info("test_write:%04x,%d", tmp_handle, ret);
        }
    }

    /* int ret = ble_client_api->opt_comm_send(ble_client_write_handle, &count, 16, ATT_OP_WRITE_WITHOUT_RESPOND); */
    /* log_info("test_write:%x", ret); */
}


static void client_event_callback(le_client_event_e event, u8 *packet, int size)
{
    switch (event) {
    case CLI_EVENT_MATCH_DEV: {
        client_match_cfg_t *match_dev = packet;
        log_info("match_name:%s\n", match_dev->compare_data);
    }
    break;

    case CLI_EVENT_MATCH_UUID: {
        opt_handle_t *opt_hdl = packet;
        if (opt_hdl->search_uuid == &jl_search_uuid_table[0]) {
            ble_client_write_handle = opt_hdl->value_handle;
            log_info("match_uuid22\n");
        }
    }
    break;

    case CLI_EVENT_SEARCH_PROFILE_COMPLETE:
        log_info("CLI_EVENT_SEARCH_PROFILE_COMPLETE\n");
        if ((!ble_client_timer) && ble_client_write_handle) {
            log_info("test timer_add\n");
            ble_client_timer = sys_timer_add(0, client_test_write, 500);
        }
        break;

    case CLI_EVENT_CONNECTED:
        break;

    case CLI_EVENT_DISCONNECT:
        if (ble_client_timer && TRANS_MULTI_BLE_MASTER_NUMS == 1) {
            sys_timeout_del(ble_client_timer);
            ble_client_write_handle = 0;
            ble_client_timer = 0;
        }
        break;

    default:
        break;
    }
}


static const client_conn_cfg_t client_conn_config = {
    .match_dev_cfg[0] = &match_dev01,
    .match_dev_cfg[1] = NULL,
    .match_dev_cfg[2] = NULL,
    /* .match_dev_cfg[1] = &match_dev02, */
    .report_data_callback = ble_report_data_deal,
    /* .search_uuid_cnt = 0, //配置不搜索profile，加快回连速度 */
    .search_uuid_cnt = (sizeof(jl_search_uuid_table) / sizeof(target_uuid_t)),
    .search_uuid_table = jl_search_uuid_table,
    .security_en = 0, //支持加密使能,对应配置 config_le_sm_support_enable
    .event_callback = client_event_callback,
};

static void ble_client_config_init(void)
{
    ble_client_api = ble_get_client_operation_table();
    ble_client_api->init_config(0, &client_conn_config);
    /* client_clear_bonding_info();//for test */
}

#endif

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙模式协议栈功能配置
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_function_select_init()
{
    // set_idle_period_slot(1600);
    set_idle_period_slot(100);
    ////设置协议栈支持设备数
    __set_user_ctrl_conn_num(TCFG_BD_NUM);
    ////msbc功能使能
    __set_support_msbc_flag(1);
#if TCFG_BT_SUPPORT_AAC
    ////AAC功能使能
    __set_support_aac_flag(1);
#else
    __set_support_aac_flag(0);
#endif

#if BT_SUPPORT_DISPLAY_BAT
    ////设置更新电池电量的时间间隔
    __bt_set_update_battery_time(60);
#else
    __bt_set_update_battery_time(0);
#endif
    // __set_disable_sco_flag(1);

    ////回连搜索时间长度设置,可使用该函数注册使用，ms单位,u16
    // __set_page_timeout_value(4000);
    __set_page_timeout_value(8000);

    ////回连时超时参数设置。ms单位。做主机有效
    // __set_super_timeout_value(4000);
    __set_super_timeout_value(8000);

#if (TCFG_BD_NUM == 2)
    ////设置开机回链的设备个数
    __set_auto_conn_device_num(2);
#endif

#if (TCFG_SPI_LCD_ENABLE)
#if TCFG_USER_EMITTER_ENABLE//带有屏幕的方案根据UI选项连接
    ////设置开机回链的设备个数
    __set_auto_conn_device_num(10);//彩屏方案支持10个设备的连接
#endif
#endif

#if BT_SUPPORT_MUSIC_VOL_SYNC
    ////设置音乐音量同步的表
    vol_sys_tab_init();
#endif

    ////设置蓝牙是否跑后台
    __set_user_background_goback(BACKGROUND_GOBACK); // 后台链接是否跳回蓝牙 1:跳回

    ////设置蓝牙加密的level
    //io_capabilities ; /*0: Display only 1: Display YesNo 2: KeyboardOnly 3: NoInputNoOutput*/
    //authentication_requirements: 0:not protect  1 :protect
    // __set_simple_pair_param(3, 0, 2);
    // __set_simple_pair_param(1, 0, 1);
    __set_simple_pair_param(3, 0, 0);


#if (USER_SUPPORT_PROFILE_PBAP==1)
    ////设置蓝牙设备类型
    __change_hci_class_type(BD_CLASS_CAR_AUDIO);
#endif

#if (TCFG_BT_SNIFF_ENABLE == 0)
    void lmp_set_sniff_disable(void);
    lmp_set_sniff_disable();
#endif


    /*
                TX     RX
       AI800x   PA13   PA12
       AC692x   PA13   PA12
       AC693x   PA8    PA9
       AC695x   PA9    PA10
       AC696x   PA9    PA10
       AC694x   PB1    PB2
       AC697x   PC2    PC3
       AC631x   PA7    PA8

    */
    ////设置蓝牙接收状态io输出，可以外接pa
    /* bt_set_rxtx_status_enable(1); */

#if TCFG_USER_BLE_ENABLE
    {
        u8 tmp_ble_addr[6];
#if (TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_ADV \
		|| TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_GMA)
        /* bt_set_tx_power(9);//ble txpwer level:0~9 */
        memcpy(tmp_ble_addr, (void *)bt_get_mac_addr(), 6);
#else
        lib_make_ble_address(tmp_ble_addr, (void *)bt_get_mac_addr());//ble mac 与 edr mac 一致
        // memcpy(tmp_ble_addr, (void *)bt_get_mac_addr(), 6);//ble mac 与 edr mac bu
#endif //
        le_controller_set_mac((void *)tmp_ble_addr);
        printf("\n-----edr + ble 's address-----");
        printf_buf((void *)bt_get_mac_addr(), 6);
        printf_buf((void *)tmp_ble_addr, 6);
    }

#if (TRANS_MULTI_BLE_EN || TUYA_MULTI_BLE_EN) && TRANS_MULTI_BLE_MASTER_NUMS
    ble_client_config_init();
#endif


#endif // TCFG_USER_BLE_ENABLE

#if (CONFIG_BT_MODE != BT_NORMAL)
    set_bt_enhanced_power_control(1);
#endif

    static u8 hc_pair_flag = 0, ret = 0;
    ret = syscfg_read(CFG_HC_PAIR_PIN_CODE, &hc_pair_flag, 1);
    if(!hc_pair_flag)
    {
        extern void __set_simple_pair_flag(u8 flag); 
        __set_simple_pair_flag(1);
    }else
    {

    }
}

/**
 * @brief 根据当前模式动态更新 BLE MAC 地址
 * @param is_hicar_mode: 1 - HiCar模式(BLE与EDR一致); 0 - CarPlay/AA模式(使用派生地址)
 */
void bt_update_ble_mac_address(u8 is_hicar_mode)
{
    u8 tmp_ble_addr[6];
    u8 *edr_addr = (u8 *)bt_get_mac_addr();

    if (is_hicar_mode) {
        /* HiCar 要求 BLE 地址与 EDR 地址完全一致 */
        memcpy(tmp_ble_addr, edr_addr, 6);
    } else {
        /* CarPlay / Android Auto 使用算法派生的 BLE 地址 */
        lib_make_ble_address(tmp_ble_addr, edr_addr);
    }

    /* 设置协议栈 BLE 控制器 MAC 地址 */
    le_controller_set_mac((void *)tmp_ble_addr);

    printf("\n[BT_MAC] Updated BLE Address (Mode: %s)", is_hicar_mode ? "HiCar" : "CarPlay/AA");
    printf(" EDR MAC:");
    printf_buf(edr_addr, 6);
    printf(" BLE MAC:");
    printf_buf(tmp_ble_addr, 6);
}
u8 switch_ble_mac_hicar = 0;
void switch_to_hicar_mode(void)
{
    /* 1. 停止当前的 BLE 广播 */
    bt_ble_adv_enable(0);

    /* 2. 更新 BLE 地址为与 EDR 完全一致 */
    bt_update_ble_mac_address(1);

    /* 3. 更新 HiCar 对应的广播数据 (ADV Data / Scan Response Data) */
    // hicar_update_adv_data();

    /* 4. 重新开启 BLE 广播 */
    bt_ble_adv_enable(1);
    printf("Switched to HiCar mode, BLE MAC updated to match EDR.\n");
    switch_ble_mac_hicar = 1;
}

void switch_to_carplay_mode(void)
{
    /* 1. 停止当前的 BLE 广播 */
    bt_ble_adv_enable(0);

    /* 2. 更新 BLE 地址为派生地址 */
    bt_update_ble_mac_address(0);

    /* 3. 更新 CarPlay / AA 的广播数据 */
    // carplay_update_adv_data();

    /* 4. 重新开启 BLE 广播 */
    bt_ble_adv_enable(1);
    switch_ble_mac_hicar = 0;
    printf("Switched to CarPlay/AA mode, BLE MAC updated to derived address.\n");
}

void my_cmd_data_send(u8 cmd, u8 *data, u16 data_len);

#if 1
static u8 sbc_data_en = 0;

/*a2dp 解码屏蔽， 1 开解码， 0 关解码*/
u8 gx_set_a2dp_decode_handle(u8 enable)
{
    sbc_data_en = !enable;
}


u8 app_get_curr_task();
int  filter_out_sbc_data_en(void)
{
    printf("filter_out_sbc_data_en %d ...\r\n", app_get_curr_task());
    
    if(app_get_curr_task() != APP_BT_TASK)
    {
        sbc_data_en = 1;    //
    }else
    {
        sbc_data_en = 0;
    }

    return sbc_data_en;
}
#endif

void gx_hfp_get_time_test()
{
    printf("Ios conn time get\r\n");
    user_send_cmd_prepare(USER_CTRL_HFP_GET_PHONE_DATE_TIME, 0, NULL);
}

void user_avrcp_status_callback(u8 *addr,u8 flag)
{
    //flag = 0 是暂停，1是播放
    printf("user_avrcp_status_callback\r\n");
    put_buf(addr,6);
    printf("flag %d", flag);


    u8 answer_data[2] = {0x01};
    if(flag == 1)
    {
        answer_data[1] = 0x2f;
    }
    else
    {
        answer_data[1] = 0x2D;
    }
    my_cmd_data_send(GX_SEND_A2DP_PLAYING_STATUS, answer_data, 2);
}

extern u8 gx_get_conn_remote_type();
void user_send_connect_diy_profile(u8 *addr);
extern u8 reconnect_flag;
u8 phone_addr_temp1[6] = {0};
// HC_BT_CONN hc_conn_status;
void timer_test_hfp(){
    //打印get_curr_channel_state() & HFP_CH
    printf("get_curr_channel_state: %d, HFP_CH: %d\r\n", get_curr_channel_state(), HFP_CH);
}
u16 stu_rfcomm_reconn_timer = 0;
static u16 stu_rfcomm_deadline_timer;
static u8 stu_rfcomm_reconn_addr[6];

/* 同时清理周期重试和一分钟截止定时器。 */
void stu_rfcomm_reconn_stop(void)
{
    if (stu_rfcomm_reconn_timer) {
        sys_timer_del(stu_rfcomm_reconn_timer);
        stu_rfcomm_reconn_timer = 0;
    }
    if (stu_rfcomm_deadline_timer) {
        sys_timeout_del(stu_rfcomm_deadline_timer);
        stu_rfcomm_deadline_timer = 0;
    }
}

static void stu_rfcomm_reconn_expired(void *priv)
{
    stu_rfcomm_deadline_timer = 0;
    stu_rfcomm_reconn_stop();
    printf("[RFCOMM] Reconnect stopped after 60 seconds.\n");
}

void stu_rfcomm_reconn_handle(u8 *addr)
{
    if (!addr) {
        return;
    }
    stu_rfcomm_reconn_stop();
    /* 保存地址，避免周期回调引用调用方的临时内存。 */
    memcpy(stu_rfcomm_reconn_addr, addr, sizeof(stu_rfcomm_reconn_addr));
    stu_rfcomm_deadline_timer = sys_timeout_add(NULL, stu_rfcomm_reconn_expired, 60000);
    if (!stu_rfcomm_deadline_timer) {
        printf("[RFCOMM] Failed to create reconnect deadline.\n");
        return;
    }
    stu_rfcomm_reconn_timer = sys_timer_add(stu_rfcomm_reconn_addr,
                                           user_send_connect_diy_profile, 1000);
    if (!stu_rfcomm_reconn_timer) {
        stu_rfcomm_reconn_stop();
    }
}
/*----------------------------------------------------------------------------*/
/**@brief    蓝牙模式协议栈对应状态处理函数
   @param    bt:事件
   @return
   @note     蓝牙初始化完成、链接、通话播歌等状态
*/
/*----------------------------------------------------------------------------*/
//经典蓝牙连接成功
u8 BT_CONNCTION_FLAG = 0;
static int bt_connction_status_event_handler(struct bt_event *bt)
{

    log_debug("-----------------------bt_connction_status_event_handler %d", bt->event);
    u8 gx_phone_call_status[2] = {1, 0}, gx_tmp_data[2] = {1, 0};
    if (bt_status_event_filter(bt) == false) {
        return false;
    }

    switch (bt->event) {
    case BT_STATUS_EXIT_OK:
        log_info("BT_STATUS_EXIT_OK\n");
        break;
    case BT_STATUS_INIT_OK:
        log_info("BT_STATUS_INIT_OK\n");
        bt_status_init_ok(bt);
#if(TCFG_TEST_BOX_ENABLE || TCFG_CHARGESTORE_ENABLE)
        chargestore_set_bt_init_ok(1);
#endif
        break;
    case BT_STATUS_START_CONNECTED:
        log_info(" BT_STATUS_START_CONNECTED\n");
#if CONFIG_TWS_DISCONN_NO_RECONN
        extern u8 tws_detach_by_remove_key;
        tws_detach_by_remove_key = 0;
#endif
        break;
    case BT_STATUS_ENCRY_COMPLETE:
        log_info(" BT_STATUS_ENCRY_COMPLETE\n");
        break;
    case BT_STATUS_SECOND_CONNECTED:
        log_info(" BT_STATUS_SECOND_CONNECTED\n");
        clear_current_poweron_memory_search_index(0);
    case BT_STATUS_FIRST_CONNECTED:
        log_info("BT_STATUS_CONNECTED\n");
        bt_status_connect(bt);
        break;
    case BT_STATUS_FIRST_DISCONNECT:
    case BT_STATUS_SECOND_DISCONNECT:
        log_info(" BT_STATUS_SECOND_DISCONNECT\n");
        bt_status_disconnect(bt);
        user_send_cmd_prepare(USER_CTRL_DISCONNECTION_HCI, 0, NULL);
        
        break;
    case BT_STATUS_PHONE_INCOME:
        log_info("BT_STATUS_PHONE_INCOME\n");
        // 电话打入
        gx_phone_call_status[1] = 2;
        my_cmd_data_send(GX_SEND_BT_CALL_STATUS, gx_phone_call_status, 2);
        bt_status_phone_income(bt);
        break;
    case BT_STATUS_PHONE_OUT:
        log_info("BT_STATUS_PHONE_OUT\n");
        // 电话打出
        gx_phone_call_status[1] = 3;
        my_cmd_data_send(GX_SEND_BT_CALL_STATUS, gx_phone_call_status, 2);

        bt_status_phone_out(bt);
        break;
    case BT_STATUS_PHONE_ACTIVE:
        log_info("BT_STATUS_PHONE_ACTIVE\n");
        gx_phone_call_status[1] = 4;
        my_cmd_data_send(GX_SEND_BT_CALL_STATUS, gx_phone_call_status, 2);
        bt_status_phone_active(bt);
        break;
    case BT_STATUS_PHONE_HANGUP:
        log_info(" BT_STATUS_PHONE_HANGUP\n");
        gx_phone_call_status[1] = 1;
        my_cmd_data_send(GX_SEND_BT_CALL_STATUS, gx_phone_call_status, 2);
        bt_status_phone_hangup(bt);
        break;
    case BT_STATUS_PHONE_NUMBER:
        log_info("BT_STATUS_PHONE_NUMBER\n");
        bt_status_phone_number(bt);
        break;
    case BT_STATUS_INBAND_RINGTONE:
        log_info("BT_STATUS_INBAND_RINGTONE\n");
        bt_status_inband_ringtone(bt);
        break;
    case BT_STATUS_BEGIN_AUTO_CON:
        log_info("BT_STATUS_BEGIN_AUTO_CON\n");
        break;
    case BT_STATUS_A2DP_MEDIA_START:
        log_info(" BT_STATUS_A2DP_MEDIA_START\n");
        bt_status_a2dp_media_start(bt);

        log_info("a2dp start .....\r\n");
        gx_tmp_data[1] = get_bt_connect_status();
        my_cmd_data_send(GX_SEND_A2DP_PLAYING_STATUS, gx_tmp_data, 2);
        break;
    case BT_STATUS_A2DP_MEDIA_STOP:
        log_info(" BT_STATUS_A2DP_MEDIA_STOP");
        bt_status_a2dp_media_stop(bt);

        log_info("a2dp stop .....\r\n");
        gx_tmp_data[1] = get_bt_connect_status();
        my_cmd_data_send(GX_SEND_A2DP_PLAYING_STATUS, gx_tmp_data, 2);

        break;
    case BT_STATUS_SCO_STATUS_CHANGE:
        log_info(" BT_STATUS_SCO_STATUS_CHANGE");
        bt_status_sco_change(bt);

        extern u8 gx_sco_conn_status;
        if(bt->value == 0xff)
        {
            printf("SCO or ESCO disconn\r\n");
            printf("SCO or ESCO disconn\r\n");
            gx_sco_conn_status = 0;
        }else{
            printf("SCO or ESCO conn\r\n");
            printf("SCO or ESCO conn\r\n");
            gx_sco_conn_status = 1;
        }
        gx_tmp_data[1] = gx_get_sco_conn_status();
        my_cmd_data_send(GX_SEND_BT_CALLING_SCO_STATUS, gx_tmp_data, 2);

        break;
    case BT_STATUS_CALL_VOL_CHANGE:
        log_info(" BT_STATUS_CALL_VOL_CHANGE ");
        bt_status_call_vol_change(bt);
        break;
    case BT_STATUS_SNIFF_STATE_UPDATE:
        log_info(" BT_STATUS_SNIFF_STATE_UPDATE \n");    //0退出SNIFF
        bt_status_sniff_state_update(bt);
        break;
    case BT_STATUS_LAST_CALL_TYPE_CHANGE:
        log_info("BT_STATUS_LAST_CALL_TYPE_CHANGE\n");
        bt_status_last_call_type_change(bt);
        break;
    case BT_STATUS_CONN_A2DP_CH:
        log_info("BT_STATUS_CONN_A2DP_CH\n");
        printf("??????????????\r\n");
        u8 phone_addr_temp[6] = {0};
        memcpy(phone_addr_temp, bt->args, 6);
        
        void carlife_conn_start(u8 *addr);
        carlife_conn_start(phone_addr_temp);

        bt_status_conn_a2dp_ch(bt);
    /* break; */
    case BT_STATUS_CONN_HFP_CH:
        bt_status_conn_hfp_ch(bt);
        break;
    case BT_STATUS_PHONE_MANUFACTURER:
        log_info("BT_STATUS_PHONE_MANUFACTURER\n");
        bt_status_phone_menufactuer(bt);
        break;
    case BT_STATUS_VOICE_RECOGNITION:
        log_info(" BT_STATUS_VOICE_RECOGNITION \n");
        bt_status_voice_recognition(bt);
        break;
    case BT_STATUS_AVRCP_INCOME_OPID:
        log_info("  BT_STATUS_AVRCP_INCOME_OPID \n");
        bt_status_avrcp_income_opid(bt);
        break;
    case  BT_STATUS_RECONN_OR_CONN:
        log_info("  BT_STATUS_RECONN_OR_CONN \n");
        BT_CONNCTION_FLAG = 1;
        #if 1
        printf("??????????????\r\n");
        memcpy(phone_addr_temp1, bt->args, 6);
        printf("[BT] Target Phone MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
               phone_addr_temp1[0], phone_addr_temp1[1], phone_addr_temp1[2],
               phone_addr_temp1[3], phone_addr_temp1[4], phone_addr_temp1[5]);

        
        printf("[bt->value %d] -- [reconnect_flag %d]", bt->value, reconnect_flag);
        if(1 == gx_get_conn_remote_type())//1 安卓
        {
            printf("gx_get_conn_remote_type 11111\r\n");
            user_send_cmd_prepare(USER_CTRL_HID_CONN, 0, NULL);
        }
        if(bt->value && reconnect_flag == 1)
        {
            printf("my reconnect flag\r\n");
            reconnect_flag = 0;
            stu_rfcomm_reconn_handle(phone_addr_temp1);
        }
#endif
        gx_tmp_data[0] = 1;
        my_cmd_data_send(MY_BT_CONN_STATUS, gx_tmp_data, 1);
        printf("PHONE type : %d", gx_get_conn_remote_type());

        if(app_get_curr_task() != APP_BT_TASK)
        {
            printf("TASK ERR\r\n");
            // my_cmd_err_send(MY_CMD_ERR, 0x06, 1);
            break;
        }

        extern u8 time_get_active_reporting;
        time_get_active_reporting = 1;
        
        #if 0 
        if(gx_get_conn_remote_type() == 1)
        {
            printf("Android conn time get\r\n");
            user_send_cmd_prepare(USER_CTRL_MAP_READ_TIME, 0, NULL);
        }else if(gx_get_conn_remote_type() == 2)
        {
            sys_timeout_add(NULL, gx_hfp_get_time_test, 2*1000);
        }
        #endif

        break;
    default:
        log_info(" BT STATUS DEFAULT\n");
        break;
    }
    return 0;
}
extern u8 rfcomm_connect_flag;
void user_send_connect_diy_profile(u8 *addr)
{
    if(rfcomm_connect_flag == 1){
        return;
    }
    printf(">>>>>>>>>>>>>>>>>> user_send_connect_diy_profile\n");
    u8 rfcomm_uuid[16] = {0};
        if(2 == gx_get_conn_remote_type())
        {
            //IOS
            u8 iap_uuid[16] = {0x00, 0x00, 0x00, 0x00, 0xde, 0xca, 0xfa, 0xde, 0xde, 0xca, 0xde, 0xaf, 0xde, 0xca, 0xca, 0xfe};
            memcpy(rfcomm_uuid, iap_uuid, 16);
            RFCOMM_IF_connect(addr, rfcomm_uuid, 16);
        }else if(1 == gx_get_conn_remote_type())
        {
            u8 AA_uuid[16] = {0x4d, 0xe1, 0x7a, 0x00, 0x52, 0xcb, 0x11, 0xe6, 0xbd, 0xf4, 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66};
            memcpy(rfcomm_uuid, AA_uuid, 16);
        }
    // u8 android_iap_uuid[16] = {0x4d, 0xe1, 0x7a, 0x00, 0x52, 0xcb, 0x11, 0xe6, 0xbd, 0xf4, 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66};
    // static const u8 iap2_re_establish[]={0x00, 0x00,0x00, 0XDE, 0XCA,BXFA,0XDE, 0XDE, OXCA,BXDE, BXAF, OXDE, BXCA, 0XCA, 0XFE};
}



/*----------------------------------------------------------------------------*/
/**@brief    蓝牙模式协议栈事件
   @param    bt:事件
   @return
   @note     蓝牙底层事件,通过app层处理
*/
/*----------------------------------------------------------------------------*/
static int bt_hci_event_handler(struct bt_event *bt)
{
    //对应原来的蓝牙连接上断开处理函数  ,bt->value=reason
    log_debug("------------------------bt_hci_event_handler reason %x %x", bt->event, bt->value);

    if (bt_hci_event_filter(bt) == 0) {
        return 0;
    }

    u8 gx_tmp_data = 0;
    switch (bt->event) {
    case HCI_EVENT_INQUIRY_COMPLETE:
        log_info(" HCI_EVENT_INQUIRY_COMPLETE \n");
        bt_hci_event_inquiry(bt);
        break;
    case HCI_EVENT_IO_CAPABILITY_REQUEST:
        log_info(" HCI_EVENT_IO_CAPABILITY_REQUEST \n");
        clock_add_set(BT_CONN_CLK);
        break;
    case HCI_EVENT_USER_CONFIRMATION_REQUEST:
        log_info(" HCI_EVENT_USER_CONFIRMATION_REQUEST %d \n", bt->value);
        ///<可通过按键来确认是否配对 1：配对   0：取消
        bt_send_pair(1);
        clock_remove_set(BT_CONN_CLK);
        break;
    case HCI_EVENT_USER_PASSKEY_REQUEST:
        log_info(" HCI_EVENT_USER_PASSKEY_REQUEST \n");
        ///<可以开始输入6位passkey
        break;
    case HCI_EVENT_USER_PRESSKEY_NOTIFICATION:
        log_info(" HCI_EVENT_USER_PRESSKEY_NOTIFICATION %x\n", bt->value);
        ///<可用于显示输入passkey位置 value 0:start  1:enrer  2:earse   3:clear  4:complete
        break;
    case HCI_EVENT_PIN_CODE_REQUEST :
        log_info("HCI_EVENT_PIN_CODE_REQUEST  \n");
        bt_send_pair(0);
        break;
    case HCI_EVENT_VENDOR_NO_RECONN_ADDR :
        log_info("HCI_EVENT_VENDOR_NO_RECONN_ADDR \n");
        bt_hci_event_disconnect(bt) ;
        break;
    case HCI_EVENT_DISCONNECTION_COMPLETE :
        stu_rfcomm_reconn_stop();
        log_info("HCI_EVENT_DISCONNECTION_COMPLETE \n");
        bt_hci_event_disconnect(bt) ;
        clock_remove_set(BT_CONN_CLK);

        BT_CONNCTION_FLAG = 0;
        gx_tmp_data = 0;
        my_cmd_data_send(MY_BT_CONN_STATUS, &gx_tmp_data, 1);

        my_remote_clean();
        
        break;
    case BTSTACK_EVENT_HCI_CONNECTIONS_DELETE:
    case HCI_EVENT_CONNECTION_COMPLETE:
        log_info(" HCI_EVENT_CONNECTION_COMPLETE \n");
        log_info(" HCI_EVENT_CONNECTION_COMPLETE %x \n", bt->value);
        switch (bt->value) {
        case ERROR_CODE_SUCCESS :
            log_info("ERROR_CODE_SUCCESS  \n");
            bt_hci_event_connection(bt);
            break;
        case ERROR_CODE_PIN_OR_KEY_MISSING:
            log_info(" ERROR_CODE_PIN_OR_KEY_MISSING \n");
            bt_hci_event_linkkey_missing(bt);
        case ERROR_CODE_SYNCHRONOUS_CONNECTION_LIMIT_TO_A_DEVICE_EXCEEDED :
        case ERROR_CODE_CONNECTION_REJECTED_DUE_TO_LIMITED_RESOURCES:
        case ERROR_CODE_CONNECTION_REJECTED_DUE_TO_UNACCEPTABLE_BD_ADDR:
        case ERROR_CODE_CONNECTION_ACCEPT_TIMEOUT_EXCEEDED  :
        case ERROR_CODE_REMOTE_USER_TERMINATED_CONNECTION   :
        case ERROR_CODE_CONNECTION_TERMINATED_BY_LOCAL_HOST :
        case ERROR_CODE_AUTHENTICATION_FAILURE :
        case CUSTOM_BB_AUTO_CANCEL_PAGE:
            bt_hci_event_disconnect(bt) ;
            break;
        case ERROR_CODE_PAGE_TIMEOUT:
            log_info(" ERROR_CODE_PAGE_TIMEOUT \n");
            bt_hci_event_page_timeout(bt);
            break;
        case ERROR_CODE_CONNECTION_TIMEOUT:
            log_info(" ERROR_CODE_CONNECTION_TIMEOUT \n");
            bt_hci_event_connection_timeout(bt);
            break;
        case ERROR_CODE_ACL_CONNECTION_ALREADY_EXISTS  :
            log_info("ERROR_CODE_ACL_CONNECTION_ALREADY_EXISTS   \n");
            bt_hci_event_connection_exist(bt);
            break;
        default:
            printf("default ..... \r\n");
            
            bt_hci_event_disconnect(bt) ;
            break;
        }
        break;
    default:
        break;
    }
    return 0;
}



/*----------------------------------------------------------------------------*/
/**@brief    蓝牙后台事件过滤处理
   @param    event: 事件
   @return   0:不需要切换模式， 1:需要切换模式，2:通话导致需要切换
   @note
*/
/*----------------------------------------------------------------------------*/
int bt_background_event_handler_filter(struct sys_event *event)
{
    u8 ret = 0;
#if TCFG_BLUETOOTH_BACK_MODE
    if ((u32)event->arg == SYS_BT_EVENT_TYPE_CON_STATUS) {
        log_info("bt con event: %d \n", event->u.bt.event);
        switch (event->u.bt.event) {
        // 需要切换蓝牙的命令
        case BT_STATUS_A2DP_MEDIA_START:
            if (__this->sbc_packet_step != 0) {
                /* log_info("sbc_packet_step : %d \n", __this->sbc_packet_step); */
                break;
            }

        case BT_STATUS_FIRST_DISCONNECT:
        case BT_STATUS_SECOND_DISCONNECT:
            //关机导致的断开不可以回去蓝牙，否则后台关机会有问题
            if (app_var.goto_poweroff_flag) {
                break;
            }
#if BACKGROUND_GOBACK
#if USER_SUPPORT_DUAL_A2DP_SOURCE
            /* ret = 1; */
#else
            ret = 1;
#endif

#else
            //判断断开的是sink设备，默认切换蓝牙
            if (event->u.bt.value) {
                ret = 1;
            }
            if (ret == 0) {
                bt_status_disconnect_background(&event->u.bt);
            }
#endif
            break;

        case BT_STATUS_SECOND_CONNECTED:
        case BT_STATUS_FIRST_CONNECTED:
#if BACKGROUND_GOBACK
#if USER_SUPPORT_DUAL_A2DP_SOURCE
            /* ret = 1; */
#else
            ret = 1;
#endif
#else
            bt_status_connect_background(&event->u.bt);
#endif
            break;

        case BT_STATUS_START_CONNECTED:
#if BACKGROUND_GOBACK
#if USER_SUPPORT_DUAL_A2DP_SOURCE
            /* ret = 1; */
#else
            ret = 1;
#endif
#endif
            break;

        case  BT_STATUS_ENCRY_COMPLETE:
            break;
        case BT_STATUS_SCO_STATUS_CHANGE:
            ret = 1;
            break;
        case BT_STATUS_LAST_CALL_TYPE_CHANGE:
            bt_status_last_call_type_change(&event->u.bt);
        case BT_STATUS_VOICE_RECOGNITION:
        case BT_STATUS_PHONE_INCOME:
        case BT_STATUS_PHONE_NUMBER:
        /* case BT_STATUS_PHONE_MANUFACTURER: */
        case BT_STATUS_PHONE_OUT:
        case BT_STATUS_PHONE_ACTIVE:
            /* case BT_STATUS_PHONE_HANGUP: */
            ret = 2;
            break;
        // 不需要处理的命令
        case BT_STATUS_INIT_OK:
            bt_status_init_ok_background(&event->u.bt);
            break;
        case BT_STATUS_A2DP_MEDIA_STOP:
            bt_drop_a2dp_frame_stop();
        case BT_STATUS_CALL_VOL_CHANGE:
            break;
        // 按原方式处理的命令
        default:
            bt_connction_status_event_handler(&event->u.bt);
            break;
        }
    } else if ((u32)event->arg == SYS_BT_EVENT_TYPE_HCI_STATUS) {
        /* log_info("bt hci event: %d \n", event->u.bt.event); */
        switch (event->u.bt.event) {
        case HCI_EVENT_IO_CAPABILITY_REQUEST:
            clock_add_set(BT_CONN_CLK);
#if BACKGROUND_GOBACK
#if USER_SUPPORT_DUAL_A2DP_SOURCE
            /* ret = 1; */
#else
            ret = 1;
#endif
#endif
            break;
        default:
            bt_hci_event_handler(&event->u.bt);
            break;
        }
    }
#if TCFG_USER_TWS_ENABLE
    else if (((u32)event->arg == SYS_BT_EVENT_FROM_TWS)) {
        /* log_info("bt tws event: %d \n", event->u.bt.event); */
        switch (event->u.bt.event) {
        /* case TWS_EVENT_CONNECTED: */
        /* ret = 1; */
        /* break; */
        default:
            bt_tws_connction_status_event_handler(&event->u.bt);
#if (TCFG_DEC2TWS_ENABLE)
            localtws_bt_event_deal(&event->u.bt);
#endif
            break;
        }
    }
#endif
    else if ((u32)event->arg == SYS_EVENT_FROM_CTRLER) {
        switch (event->u.bt.event) {
        case  BTCTRLER_EVENT_RESUME_REQ:
            /*log_info("-------   BTCTRLER_EVENT_RESUME_REQ \n"); */
            ret = 2;
            break;
        }
    }
#endif
    return ret;
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙后台事件处理
   @param    event: 事件
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
int bt_background_event_handler(struct sys_event *event)
{
    int ret = bt_background_event_handler_filter(event);
    if (ret) {
        if (false == app_check_curr_task(APP_BT_TASK)) {
            __this->cmd_flag = 1;
            if (ret == 2) {
                __this->call_flag = 1;
            } else {
                __this->call_flag = 0;
            }
            app_task_switch_to(APP_BT_TASK);
            sys_event_notify(event);
        }
    }
    return 0;
}


//#if (USER_SUPPORT_PROFILE_HID==1)
////重写bt_get_hid_name
//const char * bt_get_hid_name()
//{
//	return "HID_USER";
//}
//#endif

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙模式按键响应处理
   @param    event: 按键
   @return   1:事件处理  0:事件无处理
   @note
*/
/*----------------------------------------------------------------------------*/
int bt_key_event_handler(struct sys_event *event)
{
    int ret = true;
    struct key_event *key = &event->u.key;

    if (bt_key_event_filter_before() == false) {
        return false;
    }

    int key_event = event->u.key.event;
    int key_value = event->u.key.value;


    log_debug("bt key_event:%d %d %d %d\n", key_event, key->value, key->event, key->init);

    if (bt_key_event_filter_after(key_event) == true) {
        return true;
    }

    switch (key_event) {

    case  KEY_MUSIC_PP:
        log_info("   KEY_MUSIC_PP  \n");
        bt_key_music_pp();
        break;
    case  KEY_MUSIC_PREV:
        log_info("    KEY_MUSIC_PREV \n");
        bt_key_music_prev();
        break;
    case  KEY_MUSIC_NEXT:
        log_info("    KEY_MUSIC_NEXT \n");
        bt_key_music_next();
        break;
    case  KEY_VOL_UP:
        log_info("    KEY_VOL_UP \n");
        bt_key_vol_up();
        break;
    case  KEY_VOL_DOWN:
        log_info("    KEY_VOL_DOWN \n");
        bt_key_vol_down();
        break;
    case  KEY_CALL_LAST_NO:
        log_info("    KEY_CALL_LAST_NO \n");
        bt_key_call_last_on();
        break;
    case  KEY_CALL_HANG_UP:
        log_info("    KEY_CALL_HANG_UP \n");
        bt_key_call_hand_up();
        break;
    case  KEY_CALL_ANSWER:
        log_info("    KEY_CALL_ANSWER \n");
        bt_key_call_answer();
        break;
    case  KEY_OPEN_SIRI:
        log_info("    KEY_OPEN_SIRI \n");
        bt_key_call_siri();
        break;
    case  KEY_HID_CONTROL:
        log_info("    KEY_HID_CONTROL \n");
        bt_key_hid_control();
        break;
    case KEY_THIRD_CLICK:
        log_info("    KEY_THIRD_CLICK \n");
        bt_key_third_click(event);
        break;
    case KEY_LOW_LANTECY:
        log_info("    KEY_LOW_LANTECY \n");
        bt_key_low_lantecy();
        break;
    case  KEY_NULL:
        log_info("    KEY_NULL \n");
        ret = false;
#if TCFG_USER_TWS_ENABLE
        if ((u32)event->arg == KEY_EVENT_FROM_TWS) {
            break;
        }
#endif
        break;

    case KEY_HID_MODE_SWITCH:
#if (USER_SUPPORT_PROFILE_HID==1)
        __this->hid_mode = !__this->hid_mode;
        printf("KEY_HID_TAKE_PICTURE, %d\n", __this->hid_mode);
        user_change_profile_mode(__this->hid_mode);///HID设备名称， 默认蓝牙名称追加_HID
        //如果要修改名称， 可以重写重写这个函数,上面有重写例子
        //const char *__attribute__((weak)) bt_get_hid_name()
#endif
        break;
    case KEY_HID_TAKE_PICTURE:
        if (__this->hid_mode) {
            printf("KEY_HID_TAKE_PICTURE\n");
            user_send_cmd_prepare(USER_CTRL_HID_BOTH, 0, NULL);
        }
        break;

#if TCFG_USER_TWS_ENABLE
    case KEY_CHANGE_MODE:
#if (TCFG_DEC2TWS_ENABLE)
        ret = false;
#else
        if (tws_api_get_tws_state() & TWS_STA_SIBLING_CONNECTED) {
            if (tws_api_get_role() == TWS_ROLE_MASTER) {
                bt_tws_api_push_cmd(SYNC_CMD_MODE_CHANGE, 400);
            }
        } else {
            ret = false;
            break;
        }

#endif
        break;
#endif

    default:
        ret = false;
        break;
    }
#if (SMART_BOX_EN)
    extern int smartbox_bt_key_event_deal(int key_event, int ret);
    ret = smartbox_bt_key_event_deal(key_event, ret);
#endif
    return ret;
}


/*----------------------------------------------------------------------------*/
/**@brief    蓝牙模式协议栈状态事件、hci事件、对箱事件
   @param    event: 事件
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
int bt_sys_event_office(struct sys_event *event)
{
    u8 ret = false;
    if ((u32)event->arg == SYS_BT_EVENT_TYPE_CON_STATUS) {
        bt_connction_status_event_handler(&event->u.bt);
    } else if ((u32)event->arg == SYS_BT_EVENT_TYPE_HCI_STATUS) {
        bt_hci_event_handler(&event->u.bt);
    } else if ((u32)event->arg == SYS_BT_EVENT_FORM_SELF) {
        bt_reverb_status_change(&event->u.bt);
    }
#if TCFG_USER_TWS_ENABLE
    else if (((u32)event->arg == SYS_BT_EVENT_FROM_TWS)) {
        ret = bt_tws_connction_status_event_handler(&event->u.bt);
    }
#endif
    else if (((u32)event->arg == SYS_BT_EVENT_FROM_KEY)) {
        switch (event->u.key.event) {
        case KEY_CHANGE_MODE:
            app_task_switch_next();
            break;
        }
    }
    return ret;
}


/*----------------------------------------------------------------------------*/
/**@brief    蓝牙模式事件消息处理
   @param    event: 事件
   @return   1:处理完成,消息不需要进入common处理  0:common defult处理
   @note
*/
/*----------------------------------------------------------------------------*/
static int bt_sys_event_handler(struct sys_event *event)
{
    int ret = false;
    switch (event->type) {
    case SYS_KEY_EVENT:
        ret = bt_key_event_handler(event);
        break;
    case SYS_BT_EVENT:
        ret = bt_sys_event_office(event);
        break;
    case SYS_DEVICE_EVENT:
#if APP_ONLINE_DEBUG
        if ((u32)event->arg == DEVICE_EVENT_ONLINE_DATA) {
            //putchar('%');
            extern void app_online_event_handle(int evt_value);
            app_online_event_handle(event->u.dev.value);
        }
#endif /* #if TCFG_EARTCH_EVENT_HANDLE_ENABLE */
        break;
    default:
        return false;
    }
    // SYS_EVENT_HANDLER_SPECIFIC(event);
    app_protocol_sys_event_handler(event);
    return ret;
}

//*----------------------------------------------------------------------------*/
/**@brief    BT 模式提示音播放结束处理
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void  bt_tone_play_end_callback(void *priv, int flag)
{
    u32 index = (u32)priv;

    if (APP_BT_TASK != app_get_curr_task()) {
        log_error("tone callback task out \n");
        return;
    }

    switch (index) {
    case IDEX_TONE_BT_MODE:
        ///提示音播放结束， 启动播放器播放
        bt_task_start();
        break;
    default:
        break;
    }
}

void BI_OFF_APP(){
        user_send_cmd_prepare(USER_CTRL_DISCONNECTION_HCI, 0, NULL); // 断开当前所有蓝牙连接

        user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_DISABLE, 0, NULL); // 关闭蓝牙可发现
        user_send_cmd_prepare(USER_CTRL_WRITE_CONN_DISABLE, 0, NULL); // 关闭蓝牙可连接
}
void BT_ON_APP(){
    user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_ENABLE, 0, NULL);
    user_send_cmd_prepare(USER_CTRL_WRITE_CONN_ENABLE, 0, NULL);
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙模式
   @param
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
void app_bt_task()
{
    int res;
    int msg[32];
    ui_update_status(STATUS_EXIT_LOWPOWER);

    bt_task_init();//初始化变量、时钟、显示(未进行协议栈初始化)

#if TCFG_TONE2TWS_ENABLE
    extern void tone2tws_bt_task_start(u8 tone_play);
    tone2tws_bt_task_start(!__this->cmd_flag);
#endif

    extern u8 get_tws_background_connected_flag();
    if (!__this->cmd_flag && (!get_tws_background_connected_flag())) { //蓝牙后台拉回蓝牙模式不播放提示音
        tone_play_with_callback_by_name(tone_table[IDEX_TONE_BT_MODE], 1, bt_tone_play_end_callback, (void *)IDEX_TONE_BT_MODE);
        //协议栈初始化在提示音结束进行
    } else {
        //后台返回的情况没有播放提示音，需要在这里进行协议栈的resume
        bt_task_start();
    }
#if TCFG_DEC2TWS_ENABLE
    extern void set_tws_background_connected_flag(u8 flag);
    if (get_tws_background_connected_flag()) {
        int state = tws_api_get_tws_state();
        if (state & TWS_STA_SIBLING_CONNECTED) {
            if (tws_api_get_role() == TWS_ROLE_MASTER) {
                app_task_switch_back();
            } else {
                set_tws_background_connected_flag(0);
            }
        }
    }
#endif

    while (1) {
        app_task_get_msg(msg, ARRAY_SIZE(msg), 1);

        switch (msg[0]) {
        case APP_MSG_SYS_EVENT:
            if (bt_sys_event_handler((struct sys_event *)(msg + 1)) == false) {
                app_default_event_deal((struct sys_event *)(&msg[1]));
            }
            break;
        default:
            break;
        }



#if TCFG_BLUETOOTH_BACK_MODE
        if (!__this->init_ok && !app_var.goto_poweroff_flag) {
            //蓝牙后台方案必须要等待协议栈初始化完成
            continue;
        }
#else
        if (__this->init_start && (!__this->init_ok)) { //协议栈已经初始化 但没有初始化完成
            //需要等待初始化完成才可以走退出流程
            continue;
        }
#endif
            mem_stats();
        if (app_task_exitting()) {
            if (__this->init_start) {
                bt_task_close();
                __this->wait_exit = 1;
            } else { //蓝牙协议栈没有初始化，可以直接退出
                bt_task_close();
                return;
            }
        }

        if (__this->wait_exit) { //开始退出
            if (!__this->exiting) { ///等待蓝牙退出处理完成
                __this->wait_exit = 0;
                return;
            }
        }
    }
}

u8 bt_app_exit_check()
{
    return	bt_app_switch_exit_check();
}

#else

void app_bt_task()
{
}

u8 bt_app_exit_check()
{
    return 1;
}

int bt_background_event_handler_filter(struct sys_event *event)
{
    return 0;
}


u8 get_call_status()
{
    return BT_CALL_HANGUP;
}

u32 bt_tws_master_slot_clk(void)
{
    return 0;
}

#endif
