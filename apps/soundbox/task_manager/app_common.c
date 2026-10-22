#include "common/app_common.h"
#include "app_task.h"
#include "app_main.h"
#include "key_event_deal.h"
#include "music/music.h"
#include "pc/pc.h"
#include "record/record.h"
#include "linein/linein.h"
#include "fm/fm.h"
#include "btstack/avctp_user.h"
#include "app_power_manage.h"
#include "app_chargestore.h"
#include "usb/otg.h"
#include "usb/host/usb_host.h"
#include <stdlib.h>
#include "bt/bt_tws.h"
#include "audio_config.h"
#include "common/power_off.h"
#include "common/user_msg.h"
#include "audio_config.h"
#include "audio_enc.h"
#include "ui/ui_api.h"
#include "fm_emitter/fm_emitter_manage.h"
#include "common/fm_emitter_led7_ui.h"
#if TCFG_CHARGE_ENABLE
#include "app_charge.h"
#endif
#include "dev_multiplex_api.h"
#include "chgbox_ctrl.h"
#include "device/chargebox.h"
#include "app_online_cfg.h"
#include "soundcard/soundcard.h"
#include "smartbox_bt_manage.h"
#include "bt.h"
#include "common/dev_status.h"
#include "tone_player.h"
#include "ui_manage.h"
#include "soundbox.h"
#include "bt_emitter.h"

#define LOG_TAG_CONST       APP_ACTION
#define LOG_TAG             "[APP_ACTION]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"


int JL_rcsp_event_handler(struct rcsp_event *rcsp);
int bt_background_event_handler(struct sys_event *event);
extern u32 timer_get_ms(void);
extern int alarm_sys_event_handler(struct sys_event *event);
extern void bt_tws_sync_volume();

static u32 input_number = 0;
static u16 input_number_timer = 0;
static void input_number_timeout(void *p)
{
    input_number_timer = 0;
    printf("input_number = %d\n", input_number);
    if (app_get_curr_task() == APP_MUSIC_TASK) {
        app_task_put_key_msg(KEY_MUSIC_PLAYE_BY_DEV_FILENUM, (int)input_number);
    }
    input_number = 0;
}
static void input_number_deal(u32 num)
{
    input_number = input_number * 10 + num;
    if (input_number > 9999) {
        input_number = num;
    }
    printf("num = %d, input_number = %d, input_number_timer = %d\n", num, input_number, input_number_timer);
    if (input_number_timer == 0) {
        input_number_timer = sys_timeout_add(NULL, input_number_timeout, 1000);
    } else {
        sys_timer_modify(input_number_timer, 1000);
    }
    UI_SHOW_MENU(MENU_FILENUM, 4 * 1000, input_number, NULL);
}

extern u8 reconnect_flag;
extern void my_uart_send_data(u8 * buf, u16 buf_len);
extern void my_cmd_data_send(u8 cmd, u8* data, u16 data_len);
extern void my_cmd_err_send(u8 cmd, u8 data, u16 data_len);
extern void bytebuf_inverse(u8 *buf, u8 len);
extern int my_make_pkg(u8 id, u8 *data, u16 data_len, u8 *buffer);
u8 open_mic_flag = 0;

int app_common_key_msg_deal(struct sys_event *event)
{
    int ret = false;
    struct key_event *key = &event->u.key;
    int key_event = event->u.key.event;
    int key_value = event->u.key.value;

    if (key_event == KEY_NULL) {
        return false;
    }

    if (key_is_ui_takeover()) {
        ui_key_msg_post(key_event);
        return false;
    }

#if (TCFG_UI_ENABLE && TCFG_APP_FM_EMITTER_EN)
    if (!ui_fm_emitter_common_key_msg(key_event)) {
        return false;
    }
#endif

    log_info("common_key_event:%d\n", key_event);

    if ((key_event != KEY_POWEROFF) && (key_event != KEY_POWEROFF_HOLD)) {
        extern u8 goto_poweroff_first_flag;
        goto_poweroff_first_flag = 0;
    }

#if (SMART_BOX_EN)
    extern bool smartbox_key_event_filter_before(int key_event);
    if (smartbox_key_event_filter_before(key_event)) {
        return true;
    }
#endif

    switch (key_event) {
#if TCFG_APP_BT_EN

#if TCFG_USER_TWS_ENABLE
    case KEY_TWS_CONN:
        log_info("    KEY_TWS_CONN \n");
        bt_open_tws_conn(0);
        break;
    case KEY_TWS_DISCONN:
        log_info("    KEY_TWS_DISCONN \n");
        bt_disconnect_tws_conn();
        break;
    case KEY_TWS_REMOVE_PAIR:
        log_info("    KEY_TWS_REMOVE_PAIR \n");
        bt_tws_remove_tws_pair();
        break;
    case KEY_TWS_SEARCH_PAIR:
        log_info("    KEY_TWS_SEARCH_PAIR \n");
        bt_tws_start_search_and_pair();
        break;
    case KEY_TWS_SEARCH_REMOVE_PAIR:
        log_info("    KEY_TWS_SEARCH_REMOVE_PAIR \n");
        bt_tws_search_or_remove_pair();
        break;
#endif

    case KEY_BT_DIRECT_INIT:
        bt_direct_init();
        break;
    case KEY_BT_DIRECT_CLOSE:
        bt_direct_close();
        break;
#endif

    case  KEY_POWEROFF:
    case  KEY_POWEROFF_HOLD:
        power_off_deal(event, key_event - KEY_POWEROFF);
        break;

    case KEY_IR_NUM_0:
    case KEY_IR_NUM_1:
    case KEY_IR_NUM_2:
    case KEY_IR_NUM_3:
    case KEY_IR_NUM_4:
    case KEY_IR_NUM_5:
    case KEY_IR_NUM_6:
    case KEY_IR_NUM_7:
    case KEY_IR_NUM_8:
    case KEY_IR_NUM_9:
        input_number_deal(key_event - KEY_IR_NUM_0);
        break;

    case KEY_CHANGE_MODE:
#if (TCFG_DEC2TWS_ENABLE)
        if (!key->init) {
            break;
        }
#endif
#if TWFG_APP_POWERON_IGNORE_DEV
        if ((timer_get_ms() - app_var.start_time) > TWFG_APP_POWERON_IGNORE_DEV)
#endif//TWFG_APP_POWERON_IGNORE_DEV

        {
            printf("KEY_CHANGE_MODE\n");
            app_task_switch_next();
        }
        break;

    case KEY_VOL_UP:
        log_info("COMMON KEY_VOL_UP\n");
        if (!tone_get_status()) {
            app_audio_volume_up(1);
            printf("common vol+: %d", app_audio_get_volume(APP_AUDIO_CURRENT_STATE));
        }
        if (app_audio_get_volume(APP_AUDIO_CURRENT_STATE) == app_audio_get_max_volume()) {
            if (tone_get_status() == 0) {
#if TCFG_MAX_VOL_PROMPT
                tone_play_by_path(tone_table[IDEX_TONE_MAX_VOL], 0);
#endif
            }
        }

#if (TCFG_DEC2TWS_ENABLE)
        bt_tws_sync_volume();
#endif
        UI_SHOW_MENU(MENU_MAIN_VOL, 1000, app_audio_get_volume(APP_AUDIO_CURRENT_STATE), NULL);
        break;

    case KEY_VOL_DOWN:
        log_info("COMMON KEY_VOL_DOWN\n");
        app_audio_volume_down(1);
        printf("common vol-: %d", app_audio_get_volume(APP_AUDIO_CURRENT_STATE));
#if (TCFG_DEC2TWS_ENABLE)
        bt_tws_sync_volume();
#endif
        UI_SHOW_MENU(MENU_MAIN_VOL, 1000, app_audio_get_volume(APP_AUDIO_CURRENT_STATE), NULL);
        break;

    case  KEY_EQ_MODE:
#if(TCFG_EQ_ENABLE == 1)
        eq_mode_sw();
#endif
        break;
#if (AUDIO_OUTPUT_WAY == AUDIO_OUTPUT_WAY_BT)


    case KEY_BT_EMITTER_RECEIVER_SW:
        printf(" KEY_BT_EMITTER_RECEIVER_SW\n");
        bt_emitter_receiver_sw();
        break;

    case KEY_BT_EMITTER_PAUSE:
        r_printf(" KEY_BT_EMITTER_PAUSE\n");
        bt_emitter_pp(0);
        break;

    case KEY_BT_EMITTER_PLAY:
        r_printf(" KEY_BT_EMITTER_PLAY\n");
        bt_emitter_pp(1);
        break;

    case KEY_BT_EMITTER_SW:
        r_printf("KEY_BT_EMITTER_SW\n");
        {
            extern u8 bt_emitter_stu_sw(void);

            if (bt_emitter_stu_sw()) {
                printf("bt emitter start \n");
            } else {
                printf("bt emitter stop \n");
            }
        }
        break;
#endif


#if (TCFG_CHARGE_BOX_ENABLE)
    case  KEY_BOX_POWER_CLICK:
    case  KEY_BOX_POWER_LONG:
    case  KEY_BOX_POWER_HOLD:
    case  KEY_BOX_POWER_UP:
    case  KEY_BOX_POWER_DOUBLE:
    case  KEY_BOX_POWER_THREE:
    case  KEY_BOX_POWER_FOUR:
    case  KEY_BOX_POWER_FIVE:
        charge_box_key_event_handler(key_event);
        break;
#endif
#if (TCFG_MIC_EFFECT_ENABLE)
    case KEY_REVERB_SWITCH:
        switch_mic_effect_mode();
        break;
    case KEY_REVERB_OPEN:
#if TCFG_USER_TWS_ENABLE
        if (!key->init) {
            break;
        }
#endif
        if (mic_effect_get_status()) {
            mic_effect_stop();
        } else {
            mic_effect_start();
        }
        ret = true;
        break;
#endif
    case KEY_ENC_START:
#if (RECORDER_MIX_EN)
        if (recorder_mix_get_status()) {
            printf("recorder_encode_stop\n");
            recorder_mix_stop();
        } else {
            if (dev_manager_get_phy_logo(dev_manager_find_active(0))) {
                printf("recorder_encode_start\n");
                recorder_mix_start();
            }
        }
#endif/*RECORDER_MIX_EN*/
        break;
    default:
        ui_key_msg_post(key_event);
        ui_simple_key_msg_post(key_event, key_value);
#if (SOUNDCARD_ENABLE)
        soundcard_event_deal(event);
#endif
        break;

    }
#if (SMART_BOX_EN)
    extern int smartbox_common_key_event_deal(int key_event, int ret);
    ret = smartbox_common_key_event_deal(key_event, ret);
#endif
    return ret;
}

int app_power_user_event_handler(struct device_event *dev)
{
#if(TCFG_SYS_LVD_EN == 1)
    switch (dev->event) {
    case POWER_EVENT_POWER_WARNING:
        ui_update_status(STATUS_LOWPOWER);
        tone_play_by_path(tone_table[IDEX_TONE_LOW_POWER], 1);
        /* return 0; */
    }
#if (SOUNDCARD_ENABLE)
    soundcard_power_event(dev);
#endif
#endif
    return app_power_event_handler(dev);
}

static void app_common_device_event_handler(struct sys_event *event)
{
    int ret = 0;
    const char *logo = NULL;
    const char *usb_msg = NULL;
    u8 app  = 0xff ;
    u8 alarm_flag = 0;
    switch ((u32)event->arg) {
#if (TCFG_SOUNDBOX_TOOL_ENABLE || TCFG_ONLINE_ENABLE)
    case DEVICE_EVENT_FROM_SOUNDBOX_TOOL:
        extern int app_soundbox_tool_event_handler(struct soundbox_tool_event * soundbox_tool_dev);
        app_soundbox_tool_event_handler(&event->u.soundbox_tool);
        break;
#endif
#if TCFG_CHARGE_ENABLE
    case DEVICE_EVENT_FROM_CHARGE:
        app_charge_event_handler(&event->u.dev);
        break;
#endif//TCFG_CHARGE_ENABLE

#if (TCFG_ONLINE_ENABLE || TCFG_SOUNDBOX_TOOL_ENABLE)
    case DEVICE_EVENT_FROM_CI_UART:
        ci_data_rx_handler(CI_UART);
        break;

#if TCFG_USER_TWS_ENABLE
    case DEVICE_EVENT_FROM_CI_TWS:
        ci_data_rx_handler(CI_TWS);
        break;
#endif//TCFG_USER_TWS_ENABLE
#endif//TCFG_ONLINE_ENABLE

    case DEVICE_EVENT_FROM_POWER:
        app_power_user_event_handler(&event->u.dev);
        break;

#if TCFG_CHARGESTORE_ENABLE || TCFG_TEST_BOX_ENABLE
    case DEVICE_EVENT_CHARGE_STORE:
        app_chargestore_event_handler(&event->u.chargestore);
        break;
#endif//TCFG_CHARGESTORE_ENABLE || TCFG_TEST_BOX_ENABLE

#if(TCFG_CHARGE_BOX_ENABLE)
    case DEVICE_EVENT_FROM_CHARGEBOX:
        charge_box_ctrl_event_handler(&event->u.chargebox);
        break;
#endif


    case DEVICE_EVENT_FROM_OTG:
        ///先分析OTG设备类型
        usb_msg = (const char *)event->u.dev.value;
        if (usb_msg[0] == 's') {
            ///是从机
#if TCFG_PC_ENABLE
            ret = pc_device_event_handler(event);
            if (ret == true) {
                app = APP_PC_TASK;
            }
#endif
            break;
        } else if (usb_msg[0] == 'h') {
            ///是主机, 统一于SD卡等响应主机处理，这里不break
        } else {
            log_e("unknow otg devcie !!!\n");
            break;
        }
    case DRIVER_EVENT_FROM_SD0:
    case DRIVER_EVENT_FROM_SD1:
    case DRIVER_EVENT_FROM_SD2:
#if TCFG_APP_MUSIC_EN
        ret = dev_status_event_filter(event);///解码设备上下线， 设备挂载等处理
        if (ret == true) {
            if (event->u.dev.event == DEVICE_EVENT_IN) {
                ///设备上线， 非解码模式切换到解码模式播放
                if (app_get_curr_task() != APP_MUSIC_TASK) {
                    app = APP_MUSIC_TASK;
                }
            }
        }
#endif
        break;

#if TCFG_APP_LINEIN_EN
    case DEVICE_EVENT_FROM_LINEIN:
        ret = linein_device_event_handler(event);
        if (ret == true) {
            app = APP_LINEIN_TASK;
        }
        break;
#endif//TCFG_APP_LINEIN_EN

#if TCFG_APP_RTC_EN
    case DEVICE_EVENT_FROM_ALM:
        ret = alarm_sys_event_handler(event);
        if (ret == true) {
            alarm_flag = 1;
            app = APP_RTC_TASK;
        }
        break;
#endif//TCFG_APP_RTC_EN

    case DEVICE_UART_MASSAGE_NOTIFY:
        printf("uart event get\r\n");
        int gx_uart_event_handler(struct notify_uart *nfuart);
        gx_uart_event_handler(&event->u.gx_uart);
    break;

    default:
        /* printf("unknow SYS_DEVICE_EVENT!!, %x\n", (u32)event->arg); */
        break;
    }

#if (SMART_BOX_EN)
    extern void smartbox_update_dev_state(u32 event);
    smartbox_update_dev_state((u32)event->arg);
#endif

    if (app != 0xff) {
        if ((true != app_check_curr_task(APP_PC_TASK)) || alarm_flag) {

            //PC 不响应因为设备上线引发的模式切换
#if TWFG_APP_POWERON_IGNORE_DEV
            if ((timer_get_ms() - app_var.start_time) > TWFG_APP_POWERON_IGNORE_DEV)
#endif//TWFG_APP_POWERON_IGNORE_DEV
            {
#if (TCFG_CHARGE_ENABLE && (!TCFG_CHARGE_POWERON_ENABLE))
                extern u8 get_charge_online_flag(void);
                if (get_charge_online_flag()) {

                } else
#endif
                {

                    printf(">>>>>>>>>>>>>%s %d \n", __FUNCTION__, __LINE__);
                    app_task_switch_to(app);
                }
            }
        }
    }
}


///公共事件处理， 各自模式没有处理的事件， 会统一在这里处理
void app_default_event_deal(struct sys_event *event)
{
    int ret;
    SYS_EVENT_HANDLER_SPECIFIC(event);
    switch (event->type) {
    case SYS_DEVICE_EVENT:
        /*默认公共设备事件处理*/
        /* printf(">>>>>>>>>>>>>%s %d \n", __FUNCTION__, __LINE__); */
        app_common_device_event_handler(event);
        break;
#if TCFG_APP_BT_EN
    case SYS_BT_EVENT:
        if (true != app_check_curr_task(APP_BT_TASK)) {
            /*默认公共BT事件处理*/
            bt_background_event_handler(event);
        }
        break;
#endif
    case SYS_KEY_EVENT:
        app_common_key_msg_deal(event);
        break;
    default:
        printf("unknow event\n");
        break;
    }
}


#if 0
extern int key_event_remap(struct sys_event *e);
extern const u16 bt_key_ad_table[KEY_AD_NUM_MAX][KEY_EVENT_MAX];
u8 app_common_key_var_2_event(u32 key_var)
{
    u8 key_event = 0;
    u8 key_value = 0;
    struct sys_event e = {0};
#if TCFG_ADKEY_ENABLE
    for (; key_value < KEY_AD_NUM_MAX; key_value++) {
        for (key_event = 0; key_event < KEY_EVENT_MAX; key_event++) {
            if (bt_key_ad_table[key_value][key_event] == key_var) {
                e.type = SYS_KEY_EVENT;
                e.u.key.type = KEY_DRIVER_TYPE_AD;
                e.u.key.event = key_event;
                e.u.key.value = key_value;
                /* e.u.key.tmr = timer_get_ms(); */
                e.arg  = (void *)DEVICE_EVENT_FROM_KEY;
                /* printf("key2event:%d %d %d\n", key_var, key_value, key_event); */
                if (key_event_remap(&e)) {
                    sys_event_notify(&e);
                    return true;
                }
            }
        }
    }
#endif
    return false;
}

#else

u8 app_common_key_var_2_event(u32 key_var)
{
    struct sys_event e = {0};
    e.type = SYS_KEY_EVENT;
    e.u.key.type = KEY_DRIVER_TYPE_SOFTKEY;
    e.u.key.event = key_var;
    e.arg  = (void *)DEVICE_EVENT_FROM_KEY;
    sys_event_notify(&e);
    return true;
}
#endif

// 紧凑的日期时间结构
#pragma pack(1)//不平台对齐编译
typedef struct {
    uint8_t active_reporting;
    uint16_t year;   // 4位年份，如2026
    uint8_t month;   // 1-12
    uint8_t day;     // 1-31
    uint8_t hour;    // 0-23
    uint8_t minute;  // 0-59
    uint8_t second;  // 0-59
} DateTime;
#pragma pack()//不平台对齐编译

// 将字符串解析为DateTime结构
int parse_datetime_to_struct(const char *str, DateTime *dt) {
    // 格式："DD/MM/YY, HH:MM:SS"
    // 确保字符串长度足够
    int i = 0;
    
        // 解析年（2位）
    if (str[i] < '0' || str[i] > '9') return -1;
    if (str[i+1] < '0' || str[i+1] > '9') return -1;
    int year2digit = (str[i] - '0') * 10 + (str[i+1] - '0');
    i += 2;
    
    // 转换为4位年份（假设是2000年以后）
    if (year2digit >= 70) {
        dt->year = 1900 + year2digit;  // 1970-1999
    } else {
        dt->year = 2000 + year2digit;  // 2000-2069
    }

    
    // 检查分隔符
    if (str[i] != '/') return -1;
    i++;
    
    // 解析月
    if (str[i] < '0' || str[i] > '9') return -1;
    if (str[i+1] < '0' || str[i+1] > '9') return -1;
    dt->month = (str[i] - '0') * 10 + (str[i+1] - '0');
    i += 2;
    
    // 检查分隔符
    if (str[i] != '/') return -1;
    i++;
    
    
    // 解析日
    if (str[i] < '0' || str[i] > '9') return -1;
    if (str[i+1] < '0' || str[i+1] > '9') return -1;
    dt->day = (str[i] - '0') * 10 + (str[i+1] - '0');
    i += 2;
    
    // 检查分隔符
    if (str[i] != ',') return -1;
    i++;
    if (str[i] != ' ') return -1;
    i++;
    
    // 解析小时
    if (str[i] < '0' || str[i] > '9') return -1;
    if (str[i+1] < '0' || str[i+1] > '9') return -1;
    dt->hour = (str[i] - '0') * 10 + (str[i+1] - '0');
    i += 2;
    
    // 检查分隔符
    if (str[i] != ':') return -1;
    i++;
    
    // 解析分钟
    if (str[i] < '0' || str[i] > '9') return -1;
    if (str[i+1] < '0' || str[i+1] > '9') return -1;
    dt->minute = (str[i] - '0') * 10 + (str[i+1] - '0');
    i += 2;
    
    // 检查分隔符
    if (str[i] != ':') return -1;
    i++;
    
    // 解析秒
    if (str[i] < '0' || str[i] > '9') return -1;
    if (str[i+1] < '0' || str[i+1] > '9') return -1;
    dt->second = (str[i] - '0') * 10 + (str[i+1] - '0');
    
    return 0;  // 成功
}

// 将DateTime转换为整数（格式：YYYYMMDDHHMMSS）
uint64_t datetime_to_integer(const DateTime *dt) {
    return (uint64_t)dt->year * 10000000000ULL +
           (uint64_t)dt->month * 100000000ULL +
           (uint64_t)dt->day * 1000000ULL +
           (uint64_t)dt->hour * 10000ULL +
           (uint64_t)dt->minute * 100ULL +
           (uint64_t)dt->second;
}

u8 time_get_active_reporting = 0;
void my_cmd_data_send(u8 cmd, u8 *data, u16 data_len);
void phone_date_and_time_feedback(u8 *data, u16 len)
{
    printf("1time： [%s] ",data);

    // ios time ......
    DateTime dt;
    dt.active_reporting = time_get_active_reporting;
    
    if (parse_datetime_to_struct(data, &dt) == 0) {
        printf("解析成功:\n");
        printf("年: %d\n", dt.year);
        printf("月: %d\n", dt.month);
        printf("日: %d\n", dt.day);
        printf("时: %d\n", dt.hour);
        printf("分: %d\n", dt.minute);
        printf("秒: %d\n", dt.second);
        
        uint64_t int_value = datetime_to_integer(&dt);
        printf("\n转换为整数: %llu\n", int_value);
        printf("\n转换为整数: %02x\n", dt.active_reporting);
        printf("格式: %04d%02d%02d%02d%02d%02d\n", 
               dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second);

        my_cmd_data_send(GX_SEND_PHONE_TIME, &dt, sizeof(DateTime));
        if(dt.active_reporting == 0)
            my_cmd_err_send(MY_CMD_OK, 0, 0);
    } else {
        printf("解析失败\n");
    }

}



int parse_datetime_to_struct_android(const char *str, DateTime *dt)
{
    // 格式："20260120T140856+0800"
    int i = 0;
    
    // 验证字符串长度
    if (strlen(str) < 15) return -1;
    
    // 解析年（4位）
    if (str[i] < '0' || str[i] > '9') return -1;
    if (str[i+1] < '0' || str[i+1] > '9') return -1;
    if (str[i+2] < '0' || str[i+2] > '9') return -1;
    if (str[i+3] < '0' || str[i+3] > '9') return -1;
    dt->year = (str[i] - '0') * 1000 + 
               (str[i+1] - '0') * 100 + 
               (str[i+2] - '0') * 10 + 
               (str[i+3] - '0');
    i += 4;
    
    // 解析月
    if (str[i] < '0' || str[i] > '9') return -1;
    if (str[i+1] < '0' || str[i+1] > '9') return -1;
    dt->month = (str[i] - '0') * 10 + (str[i+1] - '0');
    i += 2;
    
    // 解析日
    if (str[i] < '0' || str[i] > '9') return -1;
    if (str[i+1] < '0' || str[i+1] > '9') return -1;
    dt->day = (str[i] - '0') * 10 + (str[i+1] - '0');
    i += 2;
    
    // 检查分隔符'T'
    if (str[i] != 'T') return -1;
    i++;
    
    // 解析小时
    if (str[i] < '0' || str[i] > '9') return -1;
    if (str[i+1] < '0' || str[i+1] > '9') return -1;
    dt->hour = (str[i] - '0') * 10 + (str[i+1] - '0');
    i += 2;
    
    // 解析分钟
    if (str[i] < '0' || str[i] > '9') return -1;
    if (str[i+1] < '0' || str[i+1] > '9') return -1;
    dt->minute = (str[i] - '0') * 10 + (str[i+1] - '0');
    i += 2;
    
    // 解析秒
    if (str[i] < '0' || str[i] > '9') return -1;
    if (str[i+1] < '0' || str[i+1] > '9') return -1;
    dt->second = (str[i] - '0') * 10 + (str[i+1] - '0');
    i += 2;
    
    // 注意：时区部分被忽略（+0800或-0500等）
    
    return 0;  // 成功
}

void map_get_time_data(char *time,int status)
{
    if(status == 0){
        printf("2time： %s ",time);
        
    DateTime dt;
    dt.active_reporting = time_get_active_reporting;
    if (parse_datetime_to_struct_android(time, &dt) == 0) {
        printf("解析成功:\n");
        printf("年: %d\n", dt.year);
        printf("月: %d\n", dt.month);
        printf("日: %d\n", dt.day);
        printf("时: %d\n", dt.hour);
        printf("分: %d\n", dt.minute);
        printf("秒: %d\n", dt.second);
        
        uint64_t int_value = datetime_to_integer(&dt);
        printf("\n转换为整数: %llu\n", int_value);
        printf("\n转换为整数: %02x\n", dt.active_reporting);
        printf("格式: %04d%02d%02d%02d%02d%02d\n", 
               dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second);
        my_cmd_data_send(GX_SEND_PHONE_TIME, &dt, sizeof(DateTime));
        if(dt.active_reporting == 0)
            my_cmd_err_send(MY_CMD_OK, 0, 0);
    } else {
        printf("解析失败\n");
    }

    }else {
        user_send_cmd_prepare(USER_CTRL_HFP_GET_PHONE_DATE_TIME,0,NULL);
    }
}