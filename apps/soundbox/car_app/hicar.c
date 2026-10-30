#include "system/includes.h"
#include "app_config.h"
#include "bt_common.h"
#include "btstack/avctp_user.h"
#include "app_main.h"

/* =========================== HiCar 专用 UUID 定义 =========================== */
// 华为 HiCar 数据传输通道 UUID (0xFE35)
#ifndef HICAR_FE35_UUID_128
#define HICAR_FE35_UUID_128 {0x00, 0x00, 0xFE, 0x35, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB}
#endif

// 华为 HiCar 认证通道 UUID (0xFE36)
#ifndef HICAR_FE36_UUID_128
#define HICAR_FE36_UUID_128 {0x00, 0x00, 0xFE, 0x36, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB}
#endif

// 蓝牙 128-bit 服务类型标识符
#ifndef BT_SERVICE_UUID_128
#define BT_SERVICE_UUID_128 0x07
#endif
extern u8 hfp_ignore_income_connect;
/* =========================== 外部函数声明 =========================== */
// 注册 SDP RFCOMM 通道 UUID 到蓝牙协议栈
extern int BTSrv_IF_regSdpRfcommUuid(u8 *pbUuid, int sdUuidLen);
// 设置本地蓝牙服务列表中的自定义 UUID
extern int BTSrv_IF_Srv_setLocalUuid(int eUuidType, unsigned char len, unsigned char *pUuid);
// 通过指定 UUID 向目标设备发起 RFCOMM 连接
extern int RFCOMM_IF_connect(u8 *pstBtAddr, unsigned char *pbUUID, int dUUIDLen);
// 协议栈命令准备与发送接口
extern u32 user_send_cmd_prepare(USER_CMD_TYPE cmd, u16 param_len, u8 *param);

/* =========================== 全局常量定义 =========================== */
static const u8 hicar_fe35_uuid_128[16] = HICAR_FE35_UUID_128; // 存储 128 位 FE35 数据 UUID
static const u8 hicar_fe36_uuid_128[16] = HICAR_FE36_UUID_128; // 存储 128 位 FE36 认证 UUID

#define HICAR_RECONNECT_INTERVAL_MS  2700

static u8 hicar_remote_addr[6];
static int hicar_reconnect_timer;
static u8 hicar_connected_mask;
static u8 hicar_next_channel;
static u8 hicar_reconnect_active;

/* Verification: request FE36 once, 3 seconds after requesting FE35. */
static u16 hicar_fe36_delay_timer;
static u8 hicar_fe36_delay_addr[6];
static void hicar_connect_fe36_delayed(void *priv);

/**
 * @brief 注册 HiCar 蓝牙本地服务及 SDP 广播 UUID
 * @details 将 HiCar 协议所需的 0xFE35 (数据) 与 0xFE36 (认证) 自定义 UUID 注册至本地 SDP 数据库，
 *          使得手机侧在搜索车机蓝牙 SDP 服务时能够成功识别 HiCar 服务。
 */
void hicar_register_local_services(void)
{
    // 1. 将 FE35 和 FE36 注册为可被 RFCOMM 调用的 SDP 服务通道
    BTSrv_IF_regSdpRfcommUuid((u8 *)hicar_fe35_uuid_128, 16);
    BTSrv_IF_regSdpRfcommUuid((u8 *)hicar_fe36_uuid_128, 16);

    // 2. 将 128-bit 自定义 UUID 写入本地广播/服务属性列表中
    BTSrv_IF_Srv_setLocalUuid(BT_SERVICE_UUID_128, 16, (u8 *)hicar_fe35_uuid_128);
    BTSrv_IF_Srv_setLocalUuid(BT_SERVICE_UUID_128, 16, (u8 *)hicar_fe36_uuid_128);
}

/**
 * @brief HiCar 蓝牙功能初始化接口
 * @details 负责向协议栈注册 HiCar 服务，并开启车机蓝牙的可被发现（Inquiry Scan）
 *          与可被连接（Page Scan）状态。
 */
void hicar_bt_init(void)
{
    // 注册 HiCar 相关的 SDP 服务 UUID
    hicar_register_local_services();

    // 开启蓝牙可被搜索/发现状态（Inquiry Scan Enable）
    user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_ENABLE, 0, NULL);
    // 开启蓝牙可被发起连接状态（Page Scan Enable）
    user_send_cmd_prepare(USER_CTRL_WRITE_CONN_ENABLE, 0, NULL);
}

/**
 * @brief 通过 HiCar 专用 UUID 向指定设备发起 RFCOMM 连接
 * @param addr 目标设备的 6 字节 MAC 地址
 * @param is_auth_channel 是否通过认证通道发起连接：
 *                        1 - 使用 0xFE36 通道 (认证通道)
 *                        0 - 使用 0xFE35 通道 (数据通道)
 */
void hicar_connect_by_uuid(u8 *addr, u8 is_auth_channel)
{
    // 指针合法性校验
    if (!addr) {
        // [调试日志] 记录异常入参
        printf("[HiCar][ERROR] hicar_connect_by_uuid: addr is NULL!\n");
        return;
    }

    // [调试日志] 打印目标设备 MAC 地址和连接类型
    printf("[HiCar][DEBUG] hicar_connect_by_uuid -> Target MAC: %02X:%02X:%02X:%02X:%02X:%02X, Channel: %s (%s)\n",
           addr[0], addr[1], addr[2], addr[3], addr[4], addr[5],
           is_auth_channel ? "Auth Channel" : "Data Channel",
           is_auth_channel ? "0xFE36" : "0xFE35");//FE36-->1

    // 根据参数选择连接认证通道 (FE36) 还是数据通道 (FE35)
    if (is_auth_channel) {
        if (hicar_fe36_delay_timer) {
            sys_timeout_del(hicar_fe36_delay_timer);
            hicar_fe36_delay_timer = 0;
        }
        RFCOMM_IF_connect(addr, (u8 *)hicar_fe36_uuid_128, 16);
        // my_cmd_err_send(0x0D, 0, 0);
    } else {
        RFCOMM_IF_connect(addr, (u8 *)hicar_fe35_uuid_128, 16);
        // if (hicar_fe36_delay_timer) {
        //     sys_timeout_del(hicar_fe36_delay_timer);
        //     hicar_fe36_delay_timer = 0;
        // }
        // memcpy(hicar_fe36_delay_addr, addr, sizeof(hicar_fe36_delay_addr));
        // hicar_fe36_delay_timer = sys_timer_add(NULL, hicar_connect_fe36_delayed, 2500);
        // if (hicar_fe36_delay_timer) {
        //     printf("[HiCar][TEST] FE36 connection scheduled in 100 ms.\n");
        // } else {
        //     printf("[HiCar][ERROR] Failed to create FE36 delay timer!\n");
        // }
    }

    // [调试日志] 函数调用结束确认
    printf("[HiCar][DEBUG] RFCOMM_IF_connect invoked successfully.\n");
}

static void hicar_connect_fe36_delayed(void *priv)
{
    //如果f35连接成功，才去连接fe36
    if (hicar_connected_mask & BIT(0)) {
        hicar_fe36_delay_timer = 0;
        printf("[HiCar][TEST] 3000 ms elapsed, requesting FE36 connection.\n");
        hicar_connect_by_uuid(hicar_fe36_delay_addr, 1);
    } else {
        printf("[HiCar][TEST] FE35 not connected yet, skipping FE36 connection attempt.\n");
    }
    
}

static void hicar_reconnect_try(void *priv)
{
    if (reconnect_mgr.type != CONNECTIONMODE_Hicar) {
        return;
    }

    if (!(hicar_connected_mask & BIT(hicar_next_channel))) {
        hicar_connect_by_uuid(hicar_remote_addr, hicar_next_channel);
    }
}
/*
 * SDP 发现是异步过程。
 * 在 A2DP 链路准备就绪后再进行重连重试，这样重连就不再依赖于单次对时序敏感的连接尝试。
 * FE35 通道会首先建立连接；只有在数据通道可用之后，才会尝试重连 FE36 通道。
 */

/**
 * @brief 启动 HiCar 重连流程
 * @param addr 目标远程设备的 MAC 地址指针
 */
void hicar_reconnect_start(u8 *addr)
{
    /* 参数校验：若地址为空，或当前连接模式不是 HiCar 模式，则直接返回 */
    if (!addr || reconnect_mgr.type != CONNECTIONMODE_Hicar) {
        return;
    }
    printf("进入hicar回连流程\n");
    //打印mac地址
    put_buf(addr,6);
    /* 保存目标远程设备的 MAC 地址 */
    memcpy(hicar_remote_addr, addr, sizeof(hicar_remote_addr));

    /* 重置连接掩码、通道序号，并标记重连状态为激活 */
    hicar_connected_mask = 0;
    hicar_next_channel = 0;
    hicar_reconnect_active = 1;

    /* 若重连定时器未创建，则添加定时任务，定时触发 hicar_reconnect_try */
    if (!hicar_reconnect_timer) {
        hicar_reconnect_timer = sys_timer_add(NULL, hicar_reconnect_try,
                                               HICAR_RECONNECT_INTERVAL_MS);
    }

    /* 立即执行一次重连尝试，无需等待第一个定时周期 */
    hicar_reconnect_try(NULL);
}

/**
 * @brief 停止 HiCar 重连流程
 */
void hicar_reconnect_stop(void)
{
    if (hicar_fe36_delay_timer) {
        sys_timeout_del(hicar_fe36_delay_timer);
        hicar_fe36_delay_timer = 0;
    }
    /* 若定时器存在，注销并清空定时器句柄 */
    if (hicar_reconnect_timer) {
        sys_timer_del(hicar_reconnect_timer);
        hicar_reconnect_timer = 0;
    }

    /* 清除所有重连状态及远程地址信息 */
    hicar_connected_mask = 0;
    hicar_next_channel = 0;
    hicar_reconnect_active = 0;
    memset(hicar_remote_addr, 0, sizeof(hicar_remote_addr));
}

/**
 * @brief 处理 HiCar 通道连接成功的事件回调
 * @param local_id 本地通道 ID (0x0A 对应第一通道，0x0B 对应第二通道)
 */
void hicar_reconnect_connected(u8 local_id)
{
    if (local_id == 0x0A) {
        /* 通道 0x0A (FE35) 连接成功：标记 Bit 0 置 1，并将下一个尝试的通道指向 1 (FE36) */
        hicar_connected_mask |= BIT(0);
        hicar_next_channel = 1;
    } else if (local_id == 0x0B) {
        /* 通道 0x0B (FE36) 连接成功：标记 Bit 1 置 1 */
        hicar_connected_mask |= BIT(1);
    }

    /* 当两个通道均已连接成功 (BIT(0) | BIT(1))，删除重连定时器以停止重试 */
    if (hicar_connected_mask == (BIT(0) | BIT(1)) && hicar_reconnect_timer) {
        sys_timer_del(hicar_reconnect_timer);
        hicar_reconnect_timer = 0;
    }
}

/**
 * @brief 处理 HiCar 通道断开连接的事件回调
 * @param local_id 本地通道 ID (0x0A 对应第一通道，0x0B 对应第二通道)
 */
void hicar_reconnect_disconnected(u8 local_id)
{
    /* 若非 HiCar 模式或当前未处于重连激活状态，则忽略断开事件 */
    if (reconnect_mgr.type != CONNECTIONMODE_Hicar || !hicar_reconnect_active) {
        return;
    }

    if (local_id == 0x0A) {
        /* 通道 0x0A 断开：清除 Bit 0 标记，重置下一个尝试通道为 0 (优先重连 FE35) */
        hicar_connected_mask &= ~BIT(0);
        hicar_next_channel = 0;
    } else if (local_id == 0x0B) {
        /* 通道 0x0B 断开：清除 Bit 1 标记，设置下一个尝试通道为 1 (重连 FE36) */
        hicar_connected_mask &= ~BIT(1);
        hicar_next_channel = 1;
    }

    /* 若重连定时器未运行，重新启动定时器继续尝试重连 */
    if (!hicar_reconnect_timer) {
        hicar_reconnect_timer = sys_timer_add(NULL, hicar_reconnect_try,
                                               HICAR_RECONNECT_INTERVAL_MS);
    }
}

/**
 * @brief HiCar RFCOMM 通道连接成功事件回调函数
 * @param addr 建立连接的对端设备 MAC 地址
 * @param local_id 本地 RFCOMM 通道/链路的句柄 ID
 */
void hicar_on_connect_success(u8 *addr, u8 local_id)
{
    if (!addr) {
        return;
    }

    // 打印连接成功的设备 MAC 地址与本地链路 ID
    printf("[HiCar] connect success, addr=%02x:%02x:%02x:%02x:%02x:%02x, local_id=%d\r\n",
           addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], local_id);
}

/**
 * @brief HiCar RFCOMM 通道断开连接事件回调函数
 * @param addr 断开连接的对端设备 MAC 地址
 * @param local_id 本地 RFCOMM 通道/链路的句柄 ID
 */
void hicar_on_disconnect(u8 *addr, u8 local_id)
{
    if (!addr) {
        return;
    }

    // 打印断开连接的设备 MAC 地址与本地链路 ID
    printf("[HiCar] disconnect, addr=%02x:%02x:%02x:%02x:%02x:%02x, local_id=%d\r\n",
           addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], local_id);
}
