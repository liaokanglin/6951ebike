/*
 * @Author: Stu 2253462718@qq.com
 * @Date: 2026-09-07 14:11:45
 * @LastEditors: Stu 2253462718@qq.com
 * @LastEditTime: 2026-09-08 10:33:16
 * @FilePath: \SDK\apps\soundbox\car_app\carlife.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "system/includes.h"
#include "app_config.h"
#include "asm/pwm_led.h"
#include "tone_player.h"
#include "ui_manage.h"
#include "app_main.h"
#include "app_task.h"
#include "asm/charge.h"
#include "app_power_manage.h"
#include "app_charge.h"
#include "user_cfg.h"
#include "power_on.h"
#include "bt.h"
#include "soundcard/peripheral.h"
#include "audio.h"
#include "vm.h"


extern reconnect_mgr_t reconnect_mgr;

static u16 carlife_conn_timer = 0;
static u8 carlife_addr[6] = {0};
void conn_carlife_handle()
{
    printf(">>>>>>>>>>>>>>>>>> user_send_connect_diy_profile\n");
    printf(">>>>>>>>>>>>>>>>>> carlife\n");
    const u8 carlife_uuid[16] = {0xa4, 0x5b, 0xc7, 0xe5, 0xbb, 0x50, 0x49, 0x49, 0x9d, 0xe1, 0xf7, 0x82, 0x99, 0xcf, 0x6d, 0x78};
    RFCOMM_IF_connect(carlife_addr, carlife_uuid, 16);
}

void carlife_conn_start(u8 *addr)
{
    printf("reconnect_mgr.type: %d\n", reconnect_mgr.type);
    if(reconnect_mgr.type != CONNECTIONMODE_Carlife)
    {
        printf("conn type is not carlife\n");
        return ;
    }

    if(carlife_conn_timer)
        return;

    memcpy(carlife_addr, addr, 6);
    carlife_conn_timer = sys_timer_add(NULL, conn_carlife_handle, 1500);
}

void carlife_conn_end()
{
    if(carlife_conn_timer)
    {
        sys_timer_del(carlife_conn_timer);
        carlife_conn_timer = 0;
        memset(carlife_addr, 0, 6);
    }
}