#include "system/includes.h"
#include "app_config.h"
#include "btcontroller_config.h"
#include "btstack/bt_profile_config.h"
#include "bt_common.h"
#include "btstack/avctp_user.h"

#define LOG_TAG     "[BT-CFG]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#include "debug.h"
#include "app_main.h"

#if TCFG_APP_BT_EN

/*------------------------------------------------------uuid---------------------------------------------------------------*/
u8 gx_iap2_id = 0;  // IAP2相关的ID，用于标识或管理当前IAP2连接状态

#if RFCOMM_UUID_ANDROID_AUTO_ENABLE

// CarPlay服务UUID
// 用于在蓝牙SDP中标识CarPlay对应的服务
const u8 carplay_service_uuid_128[16] = {
    0xEC, 0x88, 0x43, 0x48, 0xCD, 0x41, 0x40, 0xA2,
    0x97, 0x27, 0x57, 0x5D, 0x50, 0xBF, 0x1F, 0xD3
};

// Apple设备CarPlay UUID
// 用于标识Apple设备侧的CarPlay相关服务/功能
const u8 apple_device_carplay_uuid_128[16] = {
    0x2D, 0x8D, 0x24, 0x66, 0xE1, 0x4D, 0x45, 0x1C,
    0x88, 0xBC, 0x73, 0x01, 0xAB, 0xEA, 0x29, 0x1A
};

// Apple IAP2 UUID第1部分
// IAP2是Apple设备与车机之间进行认证、控制和通信时使用的协议之一
const u8 apple_iap2_uuid_128_1[16] = {
    0x00, 0x00, 0x00, 0x00, 0xDE, 0xCA, 0xFA, 0xDE,
    0xDE, 0xCA, 0xDE, 0xAF, 0xDE, 0xCA, 0xCA, 0xFF
};

// Apple IAP2 UUID第2部分
const u8 apple_iap2_uuid_128_2[16] = {
    0x00, 0x00, 0x00, 0x00, 0xDE, 0xCA, 0xFA, 0xDE,
    0xDE, 0xCA, 0xDE, 0xAF, 0xDE, 0xCA, 0xCA, 0xFE
};

// Apple IAP2 UUID第3部分
const u8 apple_iap2_uuid_128_3[16] = {
    0xFF, 0xCA, 0xCA, 0xDE, 0xAF, 0xDE, 0xCA, 0xDE,
    0xDE, 0xFA, 0xCA, 0xDE, 0x00, 0x00, 0x00, 0x00
};

// Apple IAP2 UUID第4部分
const u8 apple_iap2_uuid_128_4[16] = {
    0xFE, 0xCA, 0xCA, 0xDE, 0xAF, 0xDE, 0xCA, 0xDE,
    0xDE, 0xFA, 0xCA, 0xDE, 0x00, 0x00, 0x00, 0x00
};

#endif

#if RFCOMM_UUID_ANDROID_AUTO_ENABLE

// Android Auto相关UUID

// 标准SPP（Serial Port Profile）UUID
// 用于标识经典蓝牙串口服务
const u8 normal_service_uuid_128[16] = {
    0x00, 0x00, 0x11, 0x01, 0x00, 0x00, 0x10, 0x00,
    0x80, 0x00, 0x00, 0x80, 0x5f, 0x9b, 0x34, 0xfb
};

// Android Auto服务UUID
// 用于标识Android Auto对应的蓝牙服务
const u8 Android_auto_uuid_128[16] = {
    0x4d, 0xe1, 0x7a, 0x00, 0x52, 0xcb, 0x11, 0xe6,
    0xbd, 0xf4, 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66
};

// Android Auto UUID的另一种字节排列形式
// 通常用于兼容不同UUID字节序/协议处理方式
const u8 Android_auto_uuid_128_2[16] = {
    0x66, 0x9a, 0x0c, 0x20, 0x00, 0x08, 0xf4, 0xbd,
    0xe6, 0x11, 0xcb, 0x52, 0x00, 0x7a, 0xe1, 0x4d
};

#endif

#if RFCOMM_UUID_CARLIFE_ENABLE

// CarLife相关UUID

// CarLife服务UUID
// 用于在SDP中标识CarLife服务
const u8 carlife_service_uuid_128[16] = {
    0xa4, 0x5b, 0xc7, 0xe5, 0xbb, 0x50, 0x49, 0x49,
    0x9d, 0xe1, 0xf7, 0x82, 0x99, 0xcf, 0x6d, 0x78
};

// 标准SPP UUID
// CarLife通过RFCOMM串口方式进行通信
const u8 carlife_SerialPort_uuid_128[16] = {
    0x00, 0x00, 0x11, 0x01, 0x00, 0x00, 0x10, 0x00,
    0x80, 0x00, 0x00, 0x80, 0x5f, 0x9b, 0x34, 0xfb
};

#endif

#if RFCOMM_UUID_HICAR_ENABLE

// HiCar FE35服务UUID
// 用于标识HiCar的FE35蓝牙服务
const u8 hicar_fe35_uuid_128[16] = {
    0x00,0x00,0xFE,0x35,
    0x00,0x00,
    0x10,0x00,
    0x80,0x00,
    0x00,0x80,
    0x5F,0x9B,
    0x34,0xFB
};

// HiCar FE36认证通道UUID
// 用于标识HiCar的FE36认证相关服务
const u8 hicar_fe36_uuid_128[16] = {
    0x00,0x00,0xFE,0x36,
    0x00,0x00,
    0x10,0x00,
    0x80,0x00,
    0x00,0x80,
    0x5F,0x9B,
    0x34,0xFB
};

#endif

#define CAR_UUID_NUM 3  // 当前需要处理的车机服务UUID数量

// 手机侧发送/查询时需要匹配的UUID
const u8 *car_uuid_buf[CAR_UUID_NUM] =
{
    apple_iap2_uuid_128_1,     // CarPlay/IAP2相关UUID
    Android_auto_uuid_128,     // Android Auto相关UUID
    carlife_service_uuid_128  // CarLife服务UUID
};

// 车机侧对应返回的UUID
const u8 *car_uuid_answer_buf[CAR_UUID_NUM] =
{
    carplay_service_uuid_128,  // CarPlay服务UUID
    Android_auto_uuid_128,     // Android Auto服务UUID
    carlife_service_uuid_128  // CarLife服务UUID
};

typedef unsigned char UINT8;  // 8位无符号整数

typedef struct {
    void *offset_item;              // 链表节点，必须放在结构体第一个位置
    u32 service_record_handle;      // SDP服务记录句柄
    u8 *service_record;             // SDP服务记录数据
} service_record_item_t;

// 注册SDP服务记录到指定的链接段
#define USER_SDP_RECORD_HANDLER_REGISTER(handler) \
    const service_record_item_t handler \
        sec(.sdp_record_item)

u8 spp_up_profile_support = 1;  // 开启SPP上层Profile支持

#define USER_SDP_SERVICE_MAX 3  // 最大SDP服务数量

typedef struct {
    u32 dResult;              // 注册结果
    u32 dUUIDLen;             // UUID长度
    char pbUUID[16];          // 128位UUID数据
    unsigned int channel_id;  // RFCOMM通道号
} BTRfcRegUUID_t;

#define RFCOMM_CH_SPP_A 10        // 第1个SPP/RFCOMM通道
#define RFCOMM_CH_SPP_B RFCOMM_CH_SPP_A+1  // 第2个SPP/RFCOMM通道
#define RFCOMM_CH_SPP_C RFCOMM_CH_SPP_A+2  // 第3个SPP/RFCOMM通道

typedef struct {
    u8 working_flag;           // 服务是否正在使用
    BTRfcRegUUID_t user_rfcomm_info;  // RFCOMM服务注册信息
} user_sdp_service_t;

user_sdp_service_t user_sdp_ser[USER_SDP_SERVICE_MAX];  // SDP服务列表

// 128位UUID示例：FE010000-1234-5678-ABCD-00805F9B34FB
// 用于生成/存放128位UUID对应的RFCOMM SDP服务记录数据
const u8 rfcomm_sdp_data_sample_for_uuid128[96] = {
    // SDP服务记录内容
    0x36, 0x00, 0x5B, 0x09, 0x00, 0x00, 0x0A, 0x00, 0x01, 0x00, 0x11, 0x09, 0x00, 0x01, 0x36, 0x00,
    0x11, 0x1C, 0xfe, 0x01, 0x00, 0x00, 0x12, 0x34, 0x56, 0x78, 0xab, 0xcd, 0x00, 0x80, 0x5F, 0x9b,
    0x34, 0xfb, 0x09, 0x00, 0x04, 0x36, 0x00, 0x0E, 0x36, 0x00, 0x03, 0x19, 0x01, 0x00, 0x36, 0x00,
    0x05, 0x19, 0x00, 0x03, 0x08, 0x0a, 0x09, 0x00, 0x09, 0x36, 0x00, 0x17, 0x36, 0x00, 0x14, 0x1C,
    0xfe, 0x01, 0x00, 0x00, 0x12, 0x34, 0x56, 0x78, 0xab, 0xcd, 0x00, 0x80, 0x5F, 0x9b, 0x34, 0xfb,
    0x09, 0x01, 0x00, 0x09, 0x01, 0x00, 0x25, 0x06, 0x4A, 0x4C, 0x5F, 0x53, 0x50, 0x50, 0x00, 0x00,
};

// 16位UUID示例：0x1101（标准SPP UUID）
// 用于生成/存放16位UUID对应的RFCOMM SDP服务记录数据
const u8 rfcomm_sdp_data_sample_for_uuid16[68] = {
    // SDP服务记录内容
    0x36, 0x00, 0x3F, 0x09, 0x00, 0x00, 0x0A, 0x00, 0x01, 0x00, 0x04, 0x09, 0x00, 0x01, 0x36, 0x00,
    0x03, 0x19, 0x11, 0x01, 0x09, 0x00, 0x04, 0x36, 0x00, 0x0E, 0x36, 0x00, 0x03, 0x19, 0x01, 0x00,
    0x36, 0x00, 0x05, 0x19, 0x00, 0x03, 0x08, 0x02, 0x09, 0x00, 0x09, 0x36, 0x00, 0x09, 0x36, 0x00,
    0x06, 0x19, 0x11, 0x01, 0x09, 0x01, 0x02, 0x09, 0x01, 0x00, 0x25, 0x06, 0x4A, 0x4C, 0x5F, 0x53,
    0x50, 0x50, 0x00, 0x00,
};

// SDP服务数据缓冲区最大长度
#define SDP_INFO_BUFFER_LEN 96

// 为每个SDP服务分配一个独立的服务记录数据缓冲区
u8 user_sdp_service_data[USER_SDP_SERVICE_MAX][SDP_INFO_BUFFER_LEN] = {0x00};

// SDP服务A索引
#define USER_SDP_SERVICE_A 0

// SDP服务B索引
#define USER_SDP_SERVICE_B 1

// SDP服务C索引
#define USER_SDP_SERVICE_C 2

// 注册SDP服务记录A
USER_SDP_RECORD_HANDLER_REGISTER(user_sdp_record_item_A) = {
    .service_record = (u8 *)&user_sdp_service_data[USER_SDP_SERVICE_A][0],  // 指向服务A的数据缓冲区
    .service_record_handle = 0x00010011,                                    // 服务A的SDP记录句柄
};

// 注册SDP服务记录B
USER_SDP_RECORD_HANDLER_REGISTER(user_sdp_record_item_B) = {
    .service_record = (u8 *)&user_sdp_service_data[USER_SDP_SERVICE_B][0],  // 指向服务B的数据缓冲区
    .service_record_handle = 0x00010012,                                    // 服务B的SDP记录句柄
};

// 注册SDP服务记录C
USER_SDP_RECORD_HANDLER_REGISTER(user_sdp_record_item_C) = {
    .service_record = (u8 *)&user_sdp_service_data[USER_SDP_SERVICE_C][0],  // 指向服务C的数据缓冲区
    .service_record_handle = 0x00010013,                                    // 服务C的SDP记录句柄
};
// SDP记录中各字段的固定位置
#define SERVICE_RECORD_HANDLE_INDEX       10  // SDP Service Record Handle位置
#define SERVICE_CLASS_UUID_INDEX          18  // Service Class UUID位置
#define SERVICE_CHANNEL_ID_UUIF16_INDEX   39  // 16位UUID的RFCOMM通道号位置
#define SERVICE_CHANNEL_ID_UUIF128_INDEX  53  // 128位UUID的RFCOMM通道号位置
#define SERVICE_PROFILE_DEC_UUIF16_INDEX 50  // 16位UUID的Profile UUID位置
#define SERVICE_PROFILE_DEC_UUIF128_INDEX 64  // 128位UUID的Profile UUID位置

// 注册RFCOMM UUID并生成对应的SDP服务记录
int BTSrv_IF_regSdpRfcommUuid(u8 **pbUuid, int sdUuidLen)
{
    int i = 0;
    int sdp_data_len = 0;
    BTRfcRegUUID_t *reg_btrfc_info = NULL;
    u8 *reg_sdp_index = NULL;
    u8 req_sdp_recoed_handle = 0;
    u8 rfcomm_ch_id = 0;

    // 注意：pbUuid中的UUID字节序需要确认是否正确

    // 查找空闲的SDP服务槽位
    for (i = 0; i < USER_SDP_SERVICE_MAX; i++) {
        if (user_sdp_ser[i].working_flag != 0xA5) {
            user_sdp_ser[i].working_flag = 0xA5;  // 标记当前服务槽位正在使用

            reg_btrfc_info = &user_sdp_ser[i].user_rfcomm_info;  // 获取RFCOMM注册信息
            reg_sdp_index  = &user_sdp_service_data[i][0];       // 获取SDP数据缓冲区

            req_sdp_recoed_handle = 0x11 + i;  // 分配SDP记录句柄
            rfcomm_ch_id = RFCOMM_CH_SPP_A + i;  // 分配RFCOMM通道号
            break;
        }
    }

    // 没有空闲的SDP服务槽位
    if (i == 3) {
        printf("not support more ,please check\n");
        return 0;
    }

    // 保存RFCOMM UUID注册信息
    reg_btrfc_info->dResult = 0;              // 初始化注册结果
    reg_btrfc_info->channel_id = rfcomm_ch_id;  // 保存RFCOMM通道号
    reg_btrfc_info->dUUIDLen = sdUuidLen;       // 保存UUID长度
    memcpy(reg_btrfc_info->pbUUID, pbUuid, sdUuidLen);  // 保存UUID数据

    // 清空当前SDP服务数据缓冲区
    memset(reg_sdp_index, 0, SDP_INFO_BUFFER_LEN);

    // 处理16位UUID
    if (sdUuidLen == 2) {
        sdp_data_len = sizeof(rfcomm_sdp_data_sample_for_uuid16);

        // 使用16位UUID的SDP模板
        memcpy(reg_sdp_index, rfcomm_sdp_data_sample_for_uuid16, sdp_data_len);

        reg_sdp_index[SERVICE_RECORD_HANDLE_INDEX] = req_sdp_recoed_handle;  // 设置SDP记录句柄
        reg_sdp_index[SERVICE_CHANNEL_ID_UUIF16_INDEX] = rfcomm_ch_id;        // 设置RFCOMM通道号

        // 设置Service Class UUID
        memcpy(&reg_sdp_index[SERVICE_CLASS_UUID_INDEX], pbUuid, sdUuidLen);

        // 设置Profile Descriptor中的UUID
        memcpy(&reg_sdp_index[SERVICE_PROFILE_DEC_UUIF16_INDEX], pbUuid, sdUuidLen);
    }

    // 处理128位UUID
    if (sdUuidLen == 16) {
        sdp_data_len = sizeof(rfcomm_sdp_data_sample_for_uuid128);

        // 使用128位UUID的SDP模板
        memcpy(reg_sdp_index, rfcomm_sdp_data_sample_for_uuid128, sdp_data_len);

        reg_sdp_index[SERVICE_RECORD_HANDLE_INDEX] = req_sdp_recoed_handle;  // 设置SDP记录句柄
        reg_sdp_index[SERVICE_CHANNEL_ID_UUIF128_INDEX] = rfcomm_ch_id;      // 设置RFCOMM通道号

        // 设置Service Class UUID
        memcpy(&reg_sdp_index[SERVICE_CLASS_UUID_INDEX], pbUuid, sdUuidLen);

        // 设置Profile Descriptor中的UUID
        memcpy(&reg_sdp_index[SERVICE_PROFILE_DEC_UUIF128_INDEX], pbUuid, sdUuidLen);
    }

    // SDP数据组装完成后打印，便于调试
    // 成功后的响应数据包需要在bt_polling之后自行组装返回
    printf("user_sdp_service_data\n");
    put_buf(reg_sdp_index, sdp_data_len);

    return 0;
}
// 注销RFCOMM UUID并清除对应的SDP服务
int BTSrv_IF_unregSdpRfcommUuid(u8 **pbUuid, int sdUuidLen)
{
    int i = 0;
    BTRfcRegUUID_t *unreg_sdp_index = NULL;

    // 遍历所有已注册的SDP服务
    for (i = 0; i < USER_SDP_SERVICE_MAX; i++) {

        // 只检查正在使用的服务
        if (user_sdp_ser[i].working_flag == 0xA5) {

            // 获取当前服务的RFCOMM注册信息
            unreg_sdp_index = &user_sdp_ser[i].user_rfcomm_info;

            // UUID长度和内容都匹配时，注销该服务
            if ((unreg_sdp_index->dUUIDLen == sdUuidLen) &&
                !memcmp(unreg_sdp_index->pbUUID, pbUuid, sdUuidLen)) {

                printf("find the UUID to release\n");

                // 清除该SDP服务的所有注册信息
                memset(&user_sdp_ser[i], 0, sizeof(user_sdp_service_t));

                return 0;
            }
        }
    }

    // 未找到需要注销的UUID
    // 成功后的响应数据包需要在bt_polling之后自行组装返回
    printf("why no find the UUID to release\n");

    return 1;
}

int user_define_sdp_info_filter(u8 **current_record)  // 查询当前SDP记录是否支持
{
    // 返回0表示支持，非0表示不支持
    int i = 0;

    // 遍历所有用户自定义SDP服务
    for (i = 0; i < USER_SDP_SERVICE_MAX; i++) {

        // 判断当前SDP记录是否属于用户自定义服务
        if (&user_sdp_service_data[i][0] == current_record) {

            // 当前服务正在使用，表示支持
            if (user_sdp_ser[i].working_flag == 0xA5) {
                printf("find user infor need\n");
                return 0;
            }

            // 当前服务未使用，表示不支持
            return 1;
        }
    }

    // 非用户自定义的SDP记录默认支持
    return 0;
}
typedef struct {
    u32 dResult;       // 连接结果
    u32 dHandle;       // RFCOMM连接句柄，用于后续通路操作
    u32 dUUIDLen;      // 连接UUID长度
    char pbUUID[16];   // 连接UUID
} BTRfcConnect_t;

BTRfcConnect_t want_to_connect_info;  // 当前需要连接的RFCOMM服务信息

// 蓝牙库回调：查询远端设备是否存在对应UUID的连接信息
u8 user_sdp_uuid_connect_result(u8 flag)
{
    // flag非0表示未找到对应的RFCOMM通道信息
    if (flag) {
        printf("not find rfcomm uuid connect ch info\n");
    } else {
        // 找到对应的RFCOMM通道信息，开始连接
        printf("find rfcomm uuid connect ch id,begin to connect\n");
    }

    return 0;
}

// 根据远端地址和UUID连接RFCOMM服务
extern u8 user_rfcomm_uuid_connect(u8 **addr, u8 **uuid, u8 uuid_len, u8 *local_cid);

// 断开指定RFCOMM通道
extern void multi_spp_disconnect_channel(u8 local_cid);

// 通过指定RFCOMM通道发送数据
extern int multi_spp_send_data(u8 local_cid, u8 *rfcomm_cid, u8 **buf, u16 len);

// RFCOMM UUID回连
int RFCOMM_IF_connect(u8 **pstBtAddr, unsigned char **pbUUID, int dUUIDLen)
{
    // RFCOMM连接前需要先查询远端设备是否支持对应UUID
    // 找到本地已注册的UUID后，再获取对应的RFCOMM通道进行连接
    int i = 0;
    BTRfcRegUUID_t *sdp_index = NULL;

    // 遍历所有已注册的SDP服务
    for (i = 0; i < USER_SDP_SERVICE_MAX; i++) {

        // 只检查正在使用的服务
        if (user_sdp_ser[i].working_flag == 0xA5) {

            // 获取当前服务的RFCOMM注册信息
            sdp_index = &user_sdp_ser[i].user_rfcomm_info;

            // UUID长度和内容匹配
            if ((sdp_index->dUUIDLen == dUUIDLen) &&
                !memcmp(sdp_index->pbUUID, pbUUID, dUUIDLen - 1)) {

                // 找到对应UUID，使用对应的RFCOMM通道进行连接
                printf("find the UUID to connect\n");

                // 保存当前待连接的UUID信息
                want_to_connect_info.dResult = 0;
                want_to_connect_info.dUUIDLen = dUUIDLen;
                memcpy(want_to_connect_info.pbUUID, pbUUID, dUUIDLen);

                // 保存对应的本地RFCOMM通道号
                want_to_connect_info.dHandle = sdp_index->channel_id;

                // 根据远端地址、UUID和本地通道发起RFCOMM连接
                user_rfcomm_uuid_connect(
                    pstBtAddr,
                    pbUUID,
                    dUUIDLen,
                    want_to_connect_info.dHandle
                );

                return 0;
            }
        }
    }

    // 未找到对应UUID
    printf("not find the UUID to connect\n");

    // 清除连接句柄并设置连接失败
    want_to_connect_info.dHandle = 0;
    want_to_connect_info.dResult = 1;

    return 1;
}

int RFCOMM_IF_disconnect(u8 **pstBtAddr, int pHandle)
{
    // 只支持一台设备连接，此处不需要使用设备地址参数
    // 使用当前IAP2对应的本地RFCOMM通道ID
    u8 local_cid = gx_iap2_id;

    // 断开指定RFCOMM通道
    multi_spp_disconnect_channel(local_cid);
}

int RFCOMM_IF_sendData(u8 **pstBtAddr, int sdHandle, char **pbData, u16 wDataLen)  // 发数据
{
    // 只支持一台设备连接，此处不需要使用设备地址参数
    // 使用当前IAP2对应的本地RFCOMM通道ID
    u8 local_cid = gx_iap2_id;

    // 通过RFCOMM通道发送数据
    multi_spp_send_data(local_cid, 0, pbData, wDataLen);
}

int RFCOMM_IF_sendData_New(u8 **pstBtAddr, int sdHandle, char **pbData, u16 wDataLen)  // 指定UUID发数据
{
    // 打印指定的本地RFCOMM通道ID
    printf("local_cid : %d", sdHandle);

    // 打印待发送的数据
    put_buf(pbData, wDataLen);

    // 使用指定的RFCOMM通道ID
    u8 local_cid = sdHandle;

    // 通过指定RFCOMM通道发送数据
    multi_spp_send_data(local_cid, 0, pbData, wDataLen);
}

// 发送自定义命令数据
void my_cmd_data_send(u8 *cmd, u8 *data, u16 data_len);

// 通过UART发送数据
void my_uart_send_data(u8 *buf, u16 buf_len);

// 当前连接的手机类型
u8 my_connect_phone_type = 0;

// 获取当前连接的手机类型
u8 gx_get_conn_remote_type()
{
    return my_connect_phone_type;
}
extern void carlife_conn_end();  // CarLife连接结束处理函数

extern u8 reconnect_flag;  // RFCOMM回连标志


// RFCOMM多通道数据处理
// local_id: 当前RFCOMM本地通道ID，用于区分不同协议
// packet_type: 数据类型（连接、断开、接收数据等）
// channel: RFCOMM通信通道
// packet: 接收到的数据
// size: 数据长度
int mutil_handle_data_deal(u8 *local_id, u8 *packet_type, u16 *channel, u8 **packet, u16 *size)
{
    // 数据从这里返回
    // 通过local_id区分不同RFCOMM连接
    // bt_polling之后的数据流程需要自行处理

    printf("local_id %d\n", *local_id);
    printf("channel %d\r\n", *channel);
    printf("my_connect_phone_type %d\r\n", my_connect_phone_type);


    switch (*packet_type) {

    case 1:
        // RFCOMM连接成功事件

        printf("\n");

        // 结束之前的CarLife连接状态
        carlife_conn_end();

        // 保存当前RFCOMM连接对应的local_id
        gx_iap2_id = *local_id;

        // IAP2 RFCOMM连接成功
        // 清除回连标志
        reconnect_flag = 0;


        /*
        // 根据手机类型判断协议类型
        if(my_connect_phone_type == 2)
        {
            // iOS手机，返回CarPlay UUID
            my_cmd_data_send(0xc0, carplay_service_uuid_128, 16);

        }else if(my_connect_phone_type == 1)
        {
            // Android手机，返回Android Auto UUID
            my_cmd_data_send(0xc0, Android_auto_uuid_128, 16);

            // CarLife UUID
            // my_cmd_data_send(0xc0, carlife_service_uuid_128, 16);
        }
        */


        // 根据RFCOMM local_id区分不同服务
        if(reconnect_mgr.type == CONNECTIONMODE_Hicar){
                /*
                * HiCar模式
                *
                * 0x0A -> FE35
                * 0x0B -> FE36
                */

                printf("HiCar mode, local_id: %d\n", *local_id);
                if(*local_id == 0x0A)
                {
                    // HiCar FE35业务通道

                    my_cmd_data_send(
                        0xc0,
                        hicar_fe35_uuid_128,
                        16
                    );

                }
                else if(*local_id == 0x0B)
                {
                    // HiCar FE36认证通道

                    my_cmd_data_send(
                        0xc0,
                        hicar_fe36_uuid_128,
                        16
                    );

                }
        }else{
            if (*local_id == 0x0A)
                    {
                        // local_id 0x0A对应CarPlay服务

                        // 返回CarPlay对应UUID
                        my_cmd_data_send(0xc0, car_uuid_answer_buf[0], 16);


                        /*
                        // HiCar需要上报RFCOMM UUID对应的local_id

                        // uuid_answer_buf[0] = 0x0a;

                        // memcpy(uuid_answer_buf+1, car_uuid_buf[0], 16);

                        // my_cmd_data_send(0x0a, uuid_answer_buf, 17);
                        */
                    }
                    else if (*local_id == 0x0B)
                    {
                        // local_id 0x0B对应Android Auto服务

                        // 返回Android Auto对应UUID
                        my_cmd_data_send(0xc0, car_uuid_answer_buf[1], 16);


                        /*
                        // HiCar需要上报RFCOMM UUID对应的local_id

                        // uuid_answer_buf[0] = 0x0b;

                        // memcpy(uuid_answer_buf+1, car_uuid_buf[1], 16);

                        // my_cmd_data_send(0x0a, uuid_answer_buf, 17);
                        */
                    }
                    else if (*local_id == 0x0C)
                    {
                        // local_id 0x0C对应CarLife服务

                        // 返回CarLife对应UUID
                        my_cmd_data_send(0xc0, car_uuid_answer_buf[2], 16);


                        /*
                        // HiCar需要上报RFCOMM UUID对应的local_id

                        // uuid_answer_buf[0] = 0x0b;

                        // memcpy(uuid_answer_buf+1, car_uuid_buf[2], 16);

                        // my_cmd_data_send(0x0a, uuid_answer_buf, 17);
                        */
                    }
        }

        


        // 保存自动回连标志
        u8 auto_reconn = 1;

        // 写入系统配置，保存自动回连状态
        syscfg_write(MY_AUTO_RECONNECT_FLAG, &auto_reconn, 1);

        break;


    case 2:
        // RFCOMM断开连接事件

        printf("nutil disconnect#########\n");


        // 清除当前RFCOMM连接ID
        gx_iap2_id = 0;


        // 通知上层RFCOMM断开
        my_cmd_data_send(0xcf, 0, 0);

        break;


    case 7:
        // RFCOMM接收到数据事件

        // printf("GET PKG >>>>>>>>>>>>#########\n");

        // 打印接收到的数据
        // put_buf(packet, size);


        // 将接收到的数据上传给上层处理
        my_cmd_data_send(0xc2, *packet, *size);

        break;
    }
}


/************************************************************
 * 设置EIR广播数据
 ************************************************************/

// 最大支持的UUID数量
#define EIR_MAX_SUPPORT_UUID 3


// 当前16位UUID数量
u8 UUID16_buf_len = 0;

// 当前32位UUID数量
u8 UUID32_buf_len = 0;

// 当前128位UUID数量
u8 UUID128_buf_len = 0;


// 存放16位UUID列表
u16 UUID16_buf[EIR_MAX_SUPPORT_UUID] = {0};

// 存放32位UUID列表
u32 UUID32_buf[EIR_MAX_SUPPORT_UUID] = {0};

// 存放128位UUID列表
// 每个UUID占16字节
u8 UUID128_buf[EIR_MAX_SUPPORT_UUID][16];


// CarPlay对应的128位UUID
static const u8 de_size_128_carplay[] = {
    0x2D, 0x8D, 0x24, 0x66,
    0xE1, 0x4D, 0x45, 0x1C,
    0x88, 0xBC, 0x73, 0x01,
    0xAB, 0xEA, 0x29, 0x1A
};


// 添加用户自定义数据到EIR广播包
// eir: EIR数据缓存地址
u8 user_add_data_2_eir(u8 **eir)
{
    u8 len = 0;

    // 发射功率，单位dBm
    u8 tx_power = 3;

    // 保存EIR长度位置
    u8 *eir_len_ptr = NULL;

    // 保存EIR起始地址，用于打印
    u8 *user_eir_begin = eir;


#if 0
    // 测试UUID数据

    // 添加两个16位UUID
    UUID16_buf_len = 2;
    UUID16_buf[0] = 0x1101;
    UUID16_buf[1] = 0x1102;


    // 添加两个128位UUID
    UUID128_buf_len = 2;

    // 添加CarPlay UUID
    memcpy(&UUID128_buf[0][0], de_size_128_carplay, 16);

    // 添加CarPlay UUID
    memcpy(&UUID128_buf[1][0], de_size_128_carplay, 16);

#endif


    /***********************
     * 添加发射功率字段
     ***********************/

    // EIR格式：
    // Length + Type + Data

    // 长度 = Type长度1 + 数据长度1
    *eir++ = 1 + 1;

    // EIR类型：TX Power Level
    *eir++ = 0x0A;

    // 发射功率值
    *eir++ = tx_power;

    len = 3;



    // 保存当前位置
    eir_len_ptr = eir;



    /***********************
     * 添加16位UUID
     ***********************/

    if (UUID16_buf_len > 0) {

        // 长度：
        // UUID数量 * 2字节 + Type字段1字节
        *eir++ = 1 + UUID16_buf_len * 2;

        // EIR类型：
        // Complete List of 16-bit Service Class UUIDs
        *eir++ = 0x02;


        // 填充16bit UUID
        for (int i = 0; i < UUID16_buf_len; i++) {

            // 蓝牙UUID使用小端格式存储
            little_endian_store_16(eir, 0, UUID16_buf[i]);

            eir += 2;
        }


        // 更新EIR总长度
        len += (1 + UUID16_buf_len * 2 + 1);
    }



    /***********************
     * 添加128位UUID
     ***********************/

    if (UUID128_buf_len > 0) {

        // 长度：
        // UUID数量 * 16字节 + Type字段1字节
        *eir++ = 1 + UUID128_buf_len * 16;


        // EIR类型：
        // Complete List of 128-bit Service Class UUIDs
        *eir++ = 0x06;


        // 填充128bit UUID
        for (int i = 0; i < UUID128_buf_len; i++) {

            memcpy(*eir, &UUID128_buf[i][0], 16);

            eir += 16;
        }


        // 更新EIR总长度
        len += (1 + UUID128_buf_len * 16 + 1);
    }


    printf("user_add_data_2_eir");

    // 打印最终生成的EIR数据
    put_buf(user_eir_begin, len);


    return len;
}


// 清除UUID缓存
int uuid_bufffer_clean()
{
    // 清空16位UUID缓存
    memset(UUID16_buf, 0, sizeof(UUID16_buf));


    // 清空32位UUID缓存
    memset(UUID32_buf, 0, sizeof(UUID32_buf));


    // 清空128位UUID缓存
    memset(UUID128_buf, 0, sizeof(UUID128_buf));


    // 清零UUID数量
    UUID16_buf_len = 0;

    UUID32_buf_len = 0;

    UUID128_buf_len = 0;
}


// 蓝牙服务UUID类型
typedef enum {

    // 16bit UUID
    BT_SERVICE_UUID_16 = 0x03,

    // 32bit UUID
    BT_SERVICE_UUID_32 = 0x05,

    // 128bit UUID
    BT_SERVICE_UUID_128 = 0x07,

} BTServiceUuid_e;
/*
 * 使用时：
 * 先关闭蓝牙可发现、可连接功能
 * 连续配置完所有UUID后
 * 再重新打开可发现、可连接
 *
 * 作用：
 * 设置本机EIR广播中的Service UUID信息
 */
int BTSrv_IF_Srv_setLocalUuid(BTServiceUuid_e eUuidType, unsigned char len, unsigned char *pUuid)
{
    // 注意：
    // UUID大小端格式需要根据实际蓝牙协议栈调试确认


    /*
     * 保存UUID到缓存
     * 后续重新开启蓝牙广播时
     * 根据缓存重新生成EIR数据包
     */


    /***********************
     * 设置16bit UUID
     ***********************/
    if (BT_SERVICE_UUID_16 == eUuidType) {

        // 从输入数据中读取16位UUID
        // 蓝牙UUID一般采用小端格式
        u16 temp_uuid = little_endian_read_16(pUuid, 0);


        // 检查UUID是否已经存在
        for (int i = 0; i < EIR_MAX_SUPPORT_UUID; i++) {

            if (UUID16_buf[i] == temp_uuid) {

                // 已经存在，不重复保存
                return 0;
            }
        }


        // 保存新的16bit UUID
        UUID16_buf[UUID16_buf_len] = temp_uuid;


        // UUID数量增加
        if (++UUID16_buf_len > EIR_MAX_SUPPORT_UUID) {

            printf("buffer UUID16 not enough, please check\n");
        }
    }



    /***********************
     * 设置128bit UUID
     ***********************/
    if (BT_SERVICE_UUID_128 == eUuidType) {


        // 检查UUID是否已经存在
        for (int i = 0; i < EIR_MAX_SUPPORT_UUID; i++) {

            if (!memcmp(&UUID128_buf[i][0], pUuid, 16)) {

                // 已经存在，不重复保存
                return 0;
            }
        }


        // 保存新的128bit UUID
        memcpy(&UUID128_buf[UUID128_buf_len][0], pUuid, 16);


        // UUID数量增加
        if (++UUID128_buf_len > EIR_MAX_SUPPORT_UUID) {

            printf("buffer UUID128 not enough, please check\n");
        }
    }



    /*
     * 32bit UUID暂时不用
     *
     * 蓝牙常用的是16bit和128bit UUID
     * 32bit UUID实际应用较少
     */

    /*
    if(BT_SERVICE_UUID_32 == eUuidType){

        // 读取32bit UUID
        u32 temp_uuid = little_endian_read_32(pUuid, 0);


        // 判断是否重复
        for(int i = 0; i < EIR_MAX_SUPPORT_UUID; i++){

            if(UUID32_buf[i] == temp_uuid){

                // 已存在，不保存
                return 0;
            }
        }


        // 保存32bit UUID
        UUID32_buf[UUID32_buf_len] = temp_uuid;


        // 更新数量
        if(++UUID32_buf_len > EIR_MAX_SUPPORT_UUID){

            printf("buffer UUID32 not enough, please check\n");
        }
    }
    */


    return 0;
}
/************************************************************
 * 获取EIR数据
 ************************************************************/


// EIR类型：16bit UUID列表（不完整）
#define EIR_TYPE_INCOMPLETE_LIST_OF_UUID_16      0x02

// EIR类型：16bit UUID列表（完整）
#define EIR_TYPE_COMPLETE_LIST_OF_UUID_16        0x03

// EIR类型：32bit UUID列表（不完整）
#define EIR_TYPE_INCOMPLETE_LIST_OF_UUID_32      0x04

// EIR类型：32bit UUID列表（完整）
#define EIR_TYPE_COMPLETE_LIST_OF_UUID_32        0x05

// EIR类型：128bit UUID列表（不完整）
#define EIR_TYPE_INCOMPLETE_LIST_OF_UUID_128     0x06

// EIR类型：128bit UUID列表（完整）
#define EIR_TYPE_COMPLETE_LIST_OF_UUID_128       0x07


// 最大EIR数据长度
#define MAX_EIR_LENGTH    64



// EIR中的UUID信息结构
typedef struct BTSrvEirUuid_ {

    UINT8 uuid_len;                 // UUID长度

    UINT8 uuid[MAX_EIR_LENGTH];     // EIR中的UUID数据
                                    // 一个EIR包中可能包含一个或多个UUID

} BTSrvUuid_t;



// 保存搜索到的远端设备EIR UUID信息
typedef struct local_save_eir_uuid {

    UINT8 addr[6];                  // 远端蓝牙设备地址

    UINT8 uuid_len;                 // UUID长度

    UINT8 uuid[MAX_EIR_LENGTH];     // EIR中的UUID数据
                                    // 可能包含多个UUID

} local_save_eir_t;



// 保存当前搜索到的EIR信息
local_save_eir_t local_save_eir;


// 开始蓝牙设备搜索
extern void bt_search_device(void);



// 获取远端设备UUID信息
// pstBtAddr: 目标设备蓝牙地址
// pstUuid: 返回UUID数据
int BTSrv_IF_Srv_getDeviceUuid(u8 **pstBtAddr, u8 **pstUuid)
{
    // EIR搜索过程是异步的
    // 无法立即返回UUID结果
    // 需要通过搜索回调函数返回结果


    // 保存目标设备地址
    memcpy(local_save_eir.addr, pstBtAddr, 6);


    // 设置查询时间
    // inquiry_length单位：
    // 1个单位 = 1.28秒
    u8 inquiry_length = 20;


    // 发起蓝牙设备搜索
    user_send_cmd_prepare(
        USER_CTRL_SEARCH_DEVICE,
        1,
        &inquiry_length
    );
}



// 搜索设备后获取UUID信息回调
// addr: 设备蓝牙地址
// data: EIR数据内容
// len: EIR数据长度
void search_device_UUID_info(u8 **addr, u8 **data, u16 len)
{
    printf("search_device_UUID_info\n");


    // 打印设备地址
    put_buf(addr, 6);


    // 打印EIR数据
    // 里面包含设备广播的UUID信息
    put_buf(data, len);
}

// 过滤EIR广播数据中的UUID信息
// addr: 远端设备蓝牙地址
// eir: 远端设备EIR数据
// len: EIR数据长度
void inquiry_filter_UUID_info(u8 **addr, u8 **eir, u16 len)
{
    // 当前解析的EIR数据指针
    u8 *uuid_p = eir;

    // 保存解析出来的UUID数据长度
    u16 remote_uuid_len = 0;

    // 剩余待搜索的EIR数据长度
    int search_uuid_len = 0;


    // EIR数据为空，直接返回
    if (len == 0) {

        printf("eir data len = 0\n");

        return;
    }


    // 保存当前EIR搜索长度
    search_uuid_len = len;


    // 根据EIR长度申请缓存
    // 用于保存找到的UUID信息
    u8 *UUID_info = malloc(search_uuid_len);

    // 清空缓存
    memset(UUID_info, 0, search_uuid_len);



    // 循环解析EIR数据
    while (search_uuid_len > 0) {


        // EIR第一个字节为Length
        // Length为0表示后面没有数据
        if (uuid_p[0] == 0) {

            break;
        }



        /*
         * EIR格式：
         *
         * Byte0:
         *     Length
         *
         * Byte1:
         *     Type
         *
         * Byte2~:
         *     Data
         *
         *
         * 判断当前字段是否为UUID类型
         *
         * 0x02~0x07:
         *     16bit UUID
         *     32bit UUID
         *     128bit UUID
         */

        if ((uuid_p[1] >= EIR_TYPE_INCOMPLETE_LIST_OF_UUID_16) &&
            (uuid_p[1] <= EIR_TYPE_COMPLETE_LIST_OF_UUID_128)) {


            // Length大于1表示存在UUID数据
            if (uuid_p[0] > 1) {


                printf("eir uuid type %d, len %d\n",
                       uuid_p[1],
                       uuid_p[0] + 1);


                // 保存当前UUID EIR字段
                // 包含：
                // Length
                // Type
                // UUID Data
                memcpy(
                    UUID_info + remote_uuid_len,
                    uuid_p,
                    uuid_p[0] + 1
                );


                // 更新保存长度
                remote_uuid_len += (uuid_p[0] + 1);
            }
        }



        // 跳过当前EIR字段
        //
        // uuid_p[0]表示后续数据长度
        // 加1跳过Length自身
        search_uuid_len -= uuid_p[0] + 1;


        // 指向下一个EIR字段
        uuid_p += uuid_p[0] + 1;
    }



    // 找到了UUID信息
    if (remote_uuid_len > 0) {


        // 调用回调通知上层
        search_device_UUID_info(
            addr,
            UUID_info,
            remote_uuid_len
        );
    }


    // 释放临时缓存
    free(UUID_info);
}



/*-----------------------------------------------------extern---------------------------------------------------------------*/

// 蓝牙初始化完成回调
void MY_BT_INIT_OK();

// 发起HFP连接
void my_connect_hfp();



/*-----------------------------------------------------MY API---------------------------------------------------------------*/


// 测试使用的手机蓝牙地址
extern u8 my_phone_addr_test[6];



/**
 * 初始化CarPlay蓝牙相关功能
 *
 * 功能：
 * 1. 注册RFCOMM SDP服务UUID
 * 2. 设置本机EIR广播UUID
 * 3. 开启蓝牙扫描和连接功能
 */
void carplay_bluetooth_init()
{

#if 1

    /*
     * 创建CarPlay SDP UUID服务
     *
     * 注册RFCOMM服务后，
     * 手机可以通过SDP查询车机支持的服务UUID
     */


    // 注册CarPlay对应RFCOMM UUID
    // BTSrv_IF_regSdpRfcommUuid((u8 *)Android_auto_uuid_128, 16);

    // 注册Apple iAP2 UUID
    // BTSrv_IF_regSdpRfcommUuid((u8 *)apple_iap2_uuid_128_1, 16);

    // 注册标准Service UUID
    // BTSrv_IF_regSdpRfcommUuid((u8 *)normal_service_uuid_128, 16);


    // 注册三个车载协议UUID
    // car_uuid_buf[0] : CarPlay UUID
    // car_uuid_buf[1] : Android Auto UUID
    // car_uuid_buf[2] : CarLife UUID

    BTSrv_IF_regSdpRfcommUuid((u8 *)car_uuid_buf[0], 16);

    BTSrv_IF_regSdpRfcommUuid((u8 *)car_uuid_buf[1], 16);

    BTSrv_IF_regSdpRfcommUuid((u8 *)car_uuid_buf[2], 16);



    /*
     * 添加EIR广播UUID
     *
     * 手机搜索蓝牙设备时，
     * 会通过EIR数据判断车机支持哪些服务
     */


    // 添加Android Auto UUID
    // BTSrv_IF_Srv_setLocalUuid(
    //     BT_SERVICE_UUID_128,
    //     16,
    //     (u8 *)Android_auto_uuid_128
    // );


    // 添加CarPlay UUID
    // BTSrv_IF_Srv_setLocalUuid(
    //     BT_SERVICE_UUID_128,
    //     16,
    //     (u8 *)carplay_service_uuid_128
    // );


    // 添加Apple iAP2 UUID
    // BTSrv_IF_Srv_setLocalUuid(
    //     BT_SERVICE_UUID_128,
    //     16,
    //     (u8 *)apple_iap2_uuid_128_3
    // );


    // 添加三个车载协议UUID到EIR广播
    BTSrv_IF_Srv_setLocalUuid(
        BT_SERVICE_UUID_128,
        16,
        (u8 *)car_uuid_buf[0]
    );


    BTSrv_IF_Srv_setLocalUuid(
        BT_SERVICE_UUID_128,
        16,
        (u8 *)car_uuid_buf[1]
    );


    BTSrv_IF_Srv_setLocalUuid(
        BT_SERVICE_UUID_128,
        16,
        (u8 *)car_uuid_buf[2]
    );



    // 关闭蓝牙扫描功能
    user_send_cmd_prepare(
        USER_CTRL_WRITE_SCAN_ENABLE,
        0,
        NULL
    );


    // 关闭蓝牙连接功能
    user_send_cmd_prepare(
        USER_CTRL_WRITE_CONN_ENABLE,
        0,
        NULL
    );


#else

    // 测试：只注册Android Auto UUID

    BTSrv_IF_regSdpRfcommUuid(
        (u8 *)Android_auto_uuid_128,
        16
    );


    // 添加Android Auto广播UUID
    BTSrv_IF_Srv_setLocalUuid(
        BT_SERVICE_UUID_128,
        16,
        (u8 *)Android_auto_uuid_128
    );


    // 开启蓝牙扫描
    user_send_cmd_prepare(
        USER_CTRL_WRITE_SCAN_ENABLE,
        0,
        NULL
    );


    // 开启蓝牙连接
    user_send_cmd_prepare(
        USER_CTRL_WRITE_CONN_ENABLE,
        0,
        NULL
    );


#endif

}

void hicar_mode_init(void)
{
    printf("enter hicar mode\n");


    //关闭蓝牙广播
    user_send_cmd_prepare(
        USER_CTRL_WRITE_SCAN_ENABLE,
        0,
        NULL
    );

    user_send_cmd_prepare(
        USER_CTRL_WRITE_CONN_ENABLE,
        0,
        NULL
    );


    //清理之前注册的服务

    BTSrv_IF_unregSdpRfcommUuid(
        (u8 **)car_uuid_buf[0],
        16
    );


    BTSrv_IF_unregSdpRfcommUuid(
        (u8 **)car_uuid_buf[1],
        16
    );


    BTSrv_IF_unregSdpRfcommUuid(
        (u8 **)car_uuid_buf[2],
        16
    );


    //清理EIR
    uuid_bufffer_clean();



    //注册HiCar FE35
    BTSrv_IF_regSdpRfcommUuid(
        (u8 *)hicar_fe35_uuid_128,
        16
    );


    //注册HiCar FE36
    BTSrv_IF_regSdpRfcommUuid(
        (u8 *)hicar_fe36_uuid_128,
        16
    );



    //重新添加EIR

    BTSrv_IF_Srv_setLocalUuid(
        BT_SERVICE_UUID_128,
        16,
        (u8 *)hicar_fe35_uuid_128
    );


    BTSrv_IF_Srv_setLocalUuid(
        BT_SERVICE_UUID_128,
        16,
        (u8 *)hicar_fe36_uuid_128
    );



    //重新打开广播

    user_send_cmd_prepare(
        USER_CTRL_WRITE_SCAN_ENABLE,
        0,
        NULL
    );

    user_send_cmd_prepare(
        USER_CTRL_WRITE_CONN_ENABLE,
        0,
        NULL
    );
}

/**
 * CarPlay发送测试数据
 *
 * 通过RFCOMM通道发送测试数据
 */
void my_carplay_send_test()
{

    // 测试数据
    u8 data[6] = {
        0xFF,
        0x55,
        0x02,
        0x00,
        0xEE,
        0x10
    };


    // 使用CarPlay RFCOMM通道发送数据
    // local_cid = 0x0A
    RFCOMM_IF_sendData(
        NULL,
        0x0A,
        data,
        6
    );


    log_info(
        "my_carplay_send_test 0 ...... \n"
    );
}



/**
 * 获取手机设备UUID
 *
 * 通过EIR搜索远端设备支持的UUID
 */
void my_get_deviceUUid()
{

    BTSrv_IF_Srv_getDeviceUuid(
        my_phone_addr_test,
        carplay_service_uuid_128
    );


    printf(
        "my_get_deviceUUid : ( 0 )"
    );
}



/**
 * 断开iAP2 RFCOMM连接
 */
void my_iap2_disconnect()
{

    // 断开CarPlay RFCOMM通道
    // local_cid = 0x0A
    RFCOMM_IF_disconnect(
        NULL,
        0x0A
    );
}


#endif