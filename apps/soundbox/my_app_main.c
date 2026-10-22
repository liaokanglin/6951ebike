/*
 * @Author: Stu 2253462718@qq.com
 * @Date: 2026-09-07 13:51:26
 * @LastEditors: Stu 2253462718@qq.com
 * @LastEditTime: 2026-09-08 16:21:47
 * @FilePath: \SDK\apps\soundbox\my_app_main.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "system/includes.h"
#include "app_config.h"
#include "btcontroller_config.h"
#include "btstack/bt_profile_config.h"
#include "bt_common.h"
#include "btstack/avctp_user.h"
#include "app_main.h"
#include "le_common.h"

reconnect_mgr_t reconnect_mgr = {0};
ble_adv_data_t custom_ble_adv_data = {0};
ble_rsp_data_t custom_ble_rsp_data = {0};

int custom_ble_adv_data_set(BLE_ADV_TYPE adv_type, u8 *data, u8 len)
{
    if(len > 32)
    {
        printf("custom_ble_adv_data_set len err\r\n");
        return -1;
    }

    if(adv_type == BLE_ADV_TYPE_ADV)
    {
        custom_ble_adv_data.adv_len = len;
        memcpy(custom_ble_adv_data.adv_data, data, len);
    }
    else if(adv_type == BLE_ADV_TYPE_RSP)
    {
        custom_ble_rsp_data.rsp_len = len;
        memcpy(custom_ble_rsp_data.rsp_data, data, len);
    }
    else
    {
        printf("custom_ble_adv_data_set adv_type err\r\n");
        return -1;
    }

    return 0;
}


extern void uart_dev_test_main();
extern void gx_hc_app_init();

// Hc_RFcomm hc_rfcomm;

void gx_music_playing_status_for_test()
{
    // 拿取手机状态
    printf("playing status .... %d", get_bt_connect_status());
}

void my_app_init()
{
    // for test
    // sys_timer_add(NULL, mem_stats, 1000);

    gx_hc_app_init();
    uart_dev_test_main();
}
