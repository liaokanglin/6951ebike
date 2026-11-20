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
u8 gx_iap2_id = 0;
#if RFCOMM_UUID_ANDROID_AUTO_ENABLE
// CarPlay Service UUID
const u8 carplay_service_uuid_128[16] = {0xEC, 0x88, 0x43, 0x48, 0xCD, 0x41, 0x40, 0xA2, 0x97, 0x27, 0x57, 0x5D, 0x50, 0xBF, 0x1F, 0xD3};
// Apple Device CarPlay UUID
const u8 apple_device_carplay_uuid_128[16] = {0x2D, 0x8D, 0x24, 0x66, 0xE1, 0x4D, 0x45, 0x1C, 0x88, 0xBC, 0x73, 0x01, 0xAB, 0xEA, 0x29, 0x1A};
// Apple IAP2 UUID
const u8 apple_iap2_uuid_128_1[16] = {0x00, 0x00, 0x00, 0x00, 0xde, 0xca, 0xfa, 0xde, 0xde, 0xca, 0xde, 0xaf, 0xde, 0xca, 0xca, 0xff};
const u8 apple_iap2_uuid_128_2[16] = {0x00, 0x00, 0x00, 0x00, 0xde, 0xca, 0xfa, 0xde, 0xde, 0xca, 0xde, 0xaf, 0xde, 0xca, 0xca, 0xfe};
const u8 apple_iap2_uuid_128_3[16] = {0xff, 0xca, 0xca, 0xde, 0xaf, 0xde, 0xca, 0xde, 0xde, 0xfa, 0xca, 0xde, 0x00, 0x00, 0x00, 0x00};
const u8 apple_iap2_uuid_128_4[16] = {0xfe, 0xca, 0xca, 0xde, 0xaf, 0xde, 0xca, 0xde, 0xde, 0xfa, 0xca, 0xde, 0x00, 0x00, 0x00, 0x00};
#endif  

#if RFCOMM_UUID_ANDROID_AUTO_ENABLE
//AA uuid
const u8 normal_service_uuid_128[16] = {0x00, 0x00, 0x11, 0x01, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5f, 0x9b, 0x34, 0xfb};
const u8 Android_auto_uuid_128[16] = {0x4d, 0xe1, 0x7a, 0x00, 0x52, 0xcb, 0x11, 0xe6, 0xbd, 0xf4, 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66};
const u8 Android_auto_uuid_128_2[16] = {0x66, 0x9a, 0x0c, 0x20, 0x00, 0x08, 0xf4, 0xbd, 0xe6, 0x11, 0xcb, 0x52, 0x00, 0x7a, 0xe1, 0x4d};
#endif  
#if RFCOMM_UUID_CARLIFE_ENABLE
//carlife uuid
const u8 carlife_service_uuid_128[16] = {0xa4, 0x5b, 0xc7, 0xe5, 0xbb, 0x50, 0x49, 0x49, 0x9d, 0xe1, 0xf7, 0x82, 0x99, 0xcf, 0x6d, 0x78};
const u8 carlife_SerialPort_uuid_128[16] = {0x00, 0x00, 0x11, 0x01, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5f, 0x9b, 0x34, 0xfb};
#endif  

#if RFCOMM_UUID_HICAR_ENABLE
//hicar uuid
const u8 hicar_fe35_uuid_128[16] = {
    0x00,0x00,0xFE,0x35,
    0x00,0x00,
    0x10,0x00,
    0x80,0x00,
    0x00,0x80,
    0x5F,0x9B,
    0x34,0xFB
};

// HiCar FE36 认证通道
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

#define CAR_UUID_NUM  3

const u8 *car_uuid_buf[CAR_UUID_NUM] =
{
    apple_iap2_uuid_128_1,
    Android_auto_uuid_128,
    carlife_service_uuid_128
};

const u8 *car_uuid_answer_buf[CAR_UUID_NUM] =
{
    carplay_service_uuid_128,
    Android_auto_uuid_128,
    carlife_service_uuid_128
};


typedef unsigned short UINT8;

typedef struct {
    // linked list - assert: first field
    void *offset_item;
    // data is contained in same memory
    u32        service_record_handle;
    u8         *service_record;
} service_record_item_t;

#define USER_SDP_RECORD_HANDLER_REGISTER(handler) \
	const service_record_item_t  handler \
		sec(.sdp_record_item)


u8 spp_up_profile_support = 1;

#define USER_SDP_SERVICE_MAX 3

typedef struct {
    u32  dResult;/*!<result*/          //注册结果
    u32  dUUIDLen;/*!<UUID len*/   //uuid 长度
    //?????????????????????????????????????????????????
    char pbUUID[16];/*!<UUID*/          //????uuid
    unsigned int channel_id;            //???uuid ???? rfcomm channel id???
} BTRfcRegUUID_t;
#define RFCOMM_CH_SPP_A   10
#define RFCOMM_CH_SPP_B   RFCOMM_CH_SPP_A+1
#define RFCOMM_CH_SPP_C   RFCOMM_CH_SPP_A+2

typedef struct {
    u8 working_flag;
    BTRfcRegUUID_t user_rfcomm_info;
} user_sdp_service_t;

user_sdp_service_t user_sdp_ser[USER_SDP_SERVICE_MAX];

//FE010000-1234-5678-ABCD-00805F9B34FB
const u8 rfcomm_sdp_data_sample_for_uuid128[96] = {
    0x36, 0x00, 0x5B, 0x09, 0x00, 0x00, 0x0A, 0x00, 0x01, 0x00, 0x11, 0x09, 0x00, 0x01, 0x36, 0x00,
    0x11, 0x1C, 0xfe, 0x01, 0x00, 0x00, 0x12, 0x34, 0x56, 0x78, 0xab, 0xcd, 0x00, 0x80, 0x5F, 0x9b,
    0x34, 0xfb, 0x09, 0x00, 0x04, 0x36, 0x00, 0x0E, 0x36, 0x00, 0x03, 0x19, 0x01, 0x00, 0x36, 0x00,
    0x05, 0x19, 0x00, 0x03, 0x08, 0x0a, 0x09, 0x00, 0x09, 0x36, 0x00, 0x17, 0x36, 0x00, 0x14, 0x1C,
    0xfe, 0x01, 0x00, 0x00, 0x12, 0x34, 0x56, 0x78, 0xab, 0xcd, 0x00, 0x80, 0x5F, 0x9b, 0x34, 0xfb,
    0x09, 0x01, 0x00, 0x09, 0x01, 0x00, 0x25, 0x06, 0x4A, 0x4C, 0x5F, 0x53, 0x50, 0x50, 0x00, 0x00,
};
const u8 rfcomm_sdp_data_sample_for_uuid16[68] = {
    0x36, 0x00, 0x3F, 0x09, 0x00, 0x00, 0x0A, 0x00, 0x01, 0x00, 0x04, 0x09, 0x00, 0x01, 0x36, 0x00,
    0x03, 0x19, 0x11, 0x01, 0x09, 0x00, 0x04, 0x36, 0x00, 0x0E, 0x36, 0x00, 0x03, 0x19, 0x01, 0x00,
    0x36, 0x00, 0x05, 0x19, 0x00, 0x03, 0x08, 0x02, 0x09, 0x00, 0x09, 0x36, 0x00, 0x09, 0x36, 0x00,
    0x06, 0x19, 0x11, 0x01, 0x09, 0x01, 0x02, 0x09, 0x01, 0x00, 0x25, 0x06, 0x4A, 0x4C, 0x5F, 0x53,
    0x50, 0x50, 0x00, 0x00,
};
#define SDP_INFO_BUFFER_LEN                     96
u8 user_sdp_service_data[USER_SDP_SERVICE_MAX][SDP_INFO_BUFFER_LEN] = {0x00};
#define USER_SDP_SERVICE_A   0
#define USER_SDP_SERVICE_B   1
#define USER_SDP_SERVICE_C   2
USER_SDP_RECORD_HANDLER_REGISTER(user_sdp_record_item_A) = {
    .service_record = (u8 *) &user_sdp_service_data[USER_SDP_SERVICE_A][0],
    .service_record_handle = 0x00010011,
};
USER_SDP_RECORD_HANDLER_REGISTER(user_sdp_record_item_B) = {
    .service_record = (u8 *) &user_sdp_service_data[USER_SDP_SERVICE_B][0],
    .service_record_handle = 0x00010012,
};
USER_SDP_RECORD_HANDLER_REGISTER(user_sdp_record_item_C) = {
    .service_record = (u8 *) &user_sdp_service_data[USER_SDP_SERVICE_C][0],
    .service_record_handle = 0x00010013,
};
//rfcomm在连接中标志位
u8 rfcomm_connect_flag = 0;
////有些差异信息要修改的位置
#define SERVICE_RECORD_HANDLE_INDEX             10
#define SERVICE_CLASS_UUID_INDEX                18
#define SERVICE_CHANNEL_ID_UUIF16_INDEX         39
#define SERVICE_CHANNEL_ID_UUIF128_INDEX        53
#define SERVICE_PROFILE_DEC_UUIF16_INDEX        50
#define SERVICE_PROFILE_DEC_UUIF128_INDEX       64
int BTSrv_IF_regSdpRfcommUuid(u8 *pbUuid, int sdUuidLen)    //创建uuid
{
    int i = 0;
    int sdp_data_len = 0;
    BTRfcRegUUID_t *reg_btrfc_info = NULL;
    u8 *reg_sdp_index = NULL;
    u8 req_sdp_recoed_handle = 0;
    u8 rfcomm_ch_id = 0;
    // pbUuid的字节序自己留意正不正确
    for (i = 0; i < USER_SDP_SERVICE_MAX; i++) {
        if (user_sdp_ser[i].working_flag != 0xA5) {
            user_sdp_ser[i].working_flag = 0xA5;
            reg_btrfc_info = &user_sdp_ser[i].user_rfcomm_info;
            reg_sdp_index  = &user_sdp_service_data[i][0];
            req_sdp_recoed_handle = 0x11 + i;
            rfcomm_ch_id = RFCOMM_CH_SPP_A + i;
            break;
        }
    }
    if (i == 3) {
        printf("not support more ,please check\n");
        return 0;
    }
    reg_btrfc_info->dResult = 0;
    reg_btrfc_info->channel_id = rfcomm_ch_id;
    reg_btrfc_info->dUUIDLen = sdUuidLen;
    memcpy(reg_btrfc_info->pbUUID, pbUuid, sdUuidLen);
    memset(reg_sdp_index, 0, SDP_INFO_BUFFER_LEN);
    if (sdUuidLen == 2) {
        sdp_data_len = sizeof(rfcomm_sdp_data_sample_for_uuid16);
        memcpy(reg_sdp_index, rfcomm_sdp_data_sample_for_uuid16, sdp_data_len);
        reg_sdp_index[SERVICE_RECORD_HANDLE_INDEX]     = req_sdp_recoed_handle;
        reg_sdp_index[SERVICE_CHANNEL_ID_UUIF16_INDEX] = rfcomm_ch_id;
        memcpy(&reg_sdp_index[SERVICE_CLASS_UUID_INDEX], pbUuid, sdUuidLen);
        memcpy(&reg_sdp_index[SERVICE_PROFILE_DEC_UUIF16_INDEX], pbUuid, sdUuidLen);
    }
    if (sdUuidLen == 16) {
        sdp_data_len = sizeof(rfcomm_sdp_data_sample_for_uuid128);
        memcpy(reg_sdp_index, rfcomm_sdp_data_sample_for_uuid128, sdp_data_len);
        reg_sdp_index[SERVICE_RECORD_HANDLE_INDEX]      = req_sdp_recoed_handle;
        reg_sdp_index[SERVICE_CHANNEL_ID_UUIF128_INDEX] = rfcomm_ch_id;
        memcpy(&reg_sdp_index[SERVICE_CLASS_UUID_INDEX], pbUuid, sdUuidLen);
        memcpy(&reg_sdp_index[SERVICE_PROFILE_DEC_UUIF128_INDEX], pbUuid, sdUuidLen);
    }
    //成功之后自己组装响应的数据包在bt_polling之后返回
    printf("user_sdp_service_data\n");
    put_buf(reg_sdp_index, sdp_data_len);
    return 0;
}
/*
 * 此模块在 RAM 中维护 RFCOMM 服务记录。
 * 每种连接模式独占整套服务记录，因此在进行模式切换时，
 * 绝不能残留上一个模式的记录。
 */

/**
 * @brief 清除所有已注册的 SDP RFCOMM UUID 及服务数据记录
 * @note  用于在模式切换或重置时清空 RAM 中的 SDP 服务表，防止新旧模式记录混淆。
 */
void BTSrv_IF_clearAllSdpRfcommUuid(void)
{
    /* 清空 SDP 服务记录句柄/结构体数组 */
    memset(user_sdp_ser, 0, sizeof(user_sdp_ser));
    
    /* 清空 SDP 具体的服务属性数据缓存 */
    memset(user_sdp_service_data, 0, sizeof(user_sdp_service_data));
}
int BTSrv_IF_unregSdpRfcommUuid(u8 *pbUuid, int sdUuidLen)  //清除uuid
{
    int i = 0;
    BTRfcRegUUID_t *unreg_sdp_index = NULL;
    for (i = 0; i < USER_SDP_SERVICE_MAX; i++) {
        if (user_sdp_ser[i].working_flag == 0xA5) {
            unreg_sdp_index = &user_sdp_ser[i].user_rfcomm_info;
            if ((unreg_sdp_index->dUUIDLen == sdUuidLen) && !memcmp(unreg_sdp_index->pbUUID, pbUuid, sdUuidLen)) {
                printf("find the UUID to release\n");
                memset(&user_sdp_ser[i], 0, sizeof(user_sdp_service_t));
                return 0;
            }
        }
    }
    //成功之后自己组装相应的数据包在bt_polling?之后返回
    printf("why no find the UUID to release\n");
    return 1;
}

int user_define_sdp_info_filter(u8 *current_record)     //查询是否支持当前uuid
{
    //返回0表示支持，非0表示不支持
    int i = 0;
    for (i = 0; i < USER_SDP_SERVICE_MAX; i++) {
        if (&user_sdp_service_data[i][0] == current_record) {
            if (user_sdp_ser[i].working_flag == 0xA5) {
                printf("find user infor need\n");
                return 0;
            }
            return 1;
        }
    }
    //非用户自定义都返回0
    return 0;
}

typedef struct {
    u32    dResult;/*!<result*/          //结果
    u32    dHandle; /*!<handle*/         //用于rfcomm通路的后续操作
    u32    dUUIDLen;/*!<UUID len*/   //?通路的uuid len
    char   pbUUID[16];/*!<UUID*/          //?通路的uuid
} BTRfcConnect_t;
BTRfcConnect_t want_to_connect_info;
u8 user_sdp_uuid_connect_result(u8 flag)
{
//* 蓝牙库回到函数有没有找到远端设备对应的uuid的连接信息 **/
    if (flag) {
        printf("not find rfcomm uuid connect ch info\n");
    } else {
        printf("find rfcomm uuid connect ch id,begin to connect\n");
    }
    return 0;
}
extern u8 user_rfcomm_uuid_connect(u8 *addr, u8 *uuid, u8 uuid_len, u8 local_cid);
extern void multi_spp_disconnect_channel(u8 local_cid);
extern int multi_spp_send_data(u8 local_cid, u8 rfcomm_cid,  u8 *buf, u16 len);
int RFCOMM_IF_connect(u8 *pstBtAddr, unsigned char *pbUUID, int dUUIDLen)   //回连
{
    //rfcomm的连接需要先搜索地址支不支持该uuid， 然后再连接，保存一些信息，连接过程会获取使用
    // if(rfcomm_connect_flag == 1)
    // {
    //     printf("rfcomm_connect_flag == 1, not support connect\n");
    //     return 1;
    // }
    int i = 0;
    BTRfcRegUUID_t *sdp_index = NULL;
    for (i = 0; i < USER_SDP_SERVICE_MAX; i++) {
        if (user_sdp_ser[i].working_flag == 0xA5) {
            sdp_index = &user_sdp_ser[i].user_rfcomm_info;
            if ((sdp_index->dUUIDLen == dUUIDLen) && !memcmp(sdp_index->pbUUID, pbUUID, dUUIDLen - 1)) {
                //对应的UUID的位置，要拿local_cid也叫 dHandle取连接
                printf("find the UUID to connect\n");
                want_to_connect_info.dResult = 0;
                want_to_connect_info.dUUIDLen = dUUIDLen;
                memcpy(want_to_connect_info.pbUUID, pbUUID, dUUIDLen);
                want_to_connect_info.dHandle = sdp_index->channel_id;
                user_rfcomm_uuid_connect(pstBtAddr, pbUUID, dUUIDLen, want_to_connect_info.dHandle);
                return 0;
            }
        }
    }
    printf("not find the UUID to connect\n");
    want_to_connect_info.dHandle = 0;
    want_to_connect_info.dResult = 1;
    return 1;
}


int RFCOMM_IF_disconnect(u8 *pstBtAddr, int pHandle)
{
    //只做支持一台设备连接的情况。此处不需要使用地址参数
    u8 local_cid = gx_iap2_id;
    multi_spp_disconnect_channel(local_cid);
}
int RFCOMM_IF_sendData(u8 *pstBtAddr, int sdHandle, char *pbData, u16 wDataLen) //发数
{
    //只做支持一台设备连接的情况。此处不需要使用地址参数
    // u8 local_cid = sdHandle;
    u8 local_cid = gx_iap2_id;
    
    multi_spp_send_data(local_cid, 0, pbData, wDataLen);
}


int RFCOMM_IF_sendData_New(u8 *pstBtAddr, int sdHandle, char *pbData, u16 wDataLen) //指定uuid 发数
{
    printf("local_cid : %d", sdHandle);
    put_buf(pbData, wDataLen);
    u8 local_cid = sdHandle;
    multi_spp_send_data(local_cid, 0, pbData, wDataLen);
}

void my_cmd_data_send(u8 cmd, u8* data, u16 data_len);
void my_uart_send_data(u8 * buf, u16 buf_len);
u8 my_connect_phone_type = 0;

u8 gx_get_conn_remote_type()
{
    return my_connect_phone_type;
}

extern void carlife_conn_end();
extern u8 reconnect_flag;
static u8 hc_packet[1024];
u8 uuid_answer_buf[1+16] = {0};
int mutil_handle_data_deal(u8 local_id, u8 packet_type, u16 channel, u8 *packet, u16 size)
{
    //数据从这里返回， 通过local_id区分使用，需要的bt_polling之后返回的数据自行做流程处理
    printf("local_id %d\n", local_id);
    printf("channel %d\r\n", channel);
    printf("my_connect_phone_type %d\r\n", my_connect_phone_type);
    switch (packet_type) {
    case 1:
        // RFCOMM连接成功事件
        printf("\n");
        printf("nutil connect#########\n");
        gx_iap2_id = local_id;
        // iap2 rfcomm conncet success
        reconnect_flag = 0;
        // if(my_connect_phone_type == 2)
        // {
        //     //ios
        //     my_cmd_data_send(0xc0, carplay_service_uuid_128, 16);
        // }else if(my_connect_phone_type == 1)
        // {
        //     my_cmd_data_send(0xc0, Android_auto_uuid_128, 16);
        //     // my_cmd_data_send(0xc0, carlife_service_uuid_128, 16);
            
        // }
        
        // 根据RFCOMM local_id区分不同服务
        if(reconnect_mgr.type == CONNECTIONMODE_Hicar){
                /*
                * HiCar模式
                *
                * 0x0A -> FE35
                * 0x0B -> FE36
                */

                printf("HiCar mode, local_id: %d\n", local_id);
                if(local_id == 0x0A)
                {
                    // HiCar FE35业务通道

                    my_cmd_data_send(
                        0xc0,
                        hicar_fe35_uuid_128,
                        16
                    );
                    // 组装串口响应帧：首字节为通道号 0x0A，后面紧跟 16 字节 UUID
                    uuid_answer_buf[0] = 0x0a;
                    memcpy(uuid_answer_buf+1, hicar_fe35_uuid_128, 16);
                    // 通过串口向主控发送命令 0x0A，通知通道 0x0A 建立完毕
                    my_cmd_data_send(0x0a, uuid_answer_buf, 17);
                    extern void hicar_reconnect_connected(u8 local_id);
                    hicar_reconnect_connected(local_id);

                }
                else if(local_id == 0x0B)
                {
                    // HiCar FE36认证通道

                    my_cmd_data_send(
                        0xc0,
                        hicar_fe36_uuid_128,
                        16
                    );
                    // 组装串口响应帧：首字节为通道号 0x0B，后面紧跟 16 字节 UUID
                    uuid_answer_buf[0] = 0x0b;
                    memcpy(uuid_answer_buf+1, hicar_fe36_uuid_128, 16);
                    // 通过串口向主控发送命令 0x0A，通知通道 0x0B 建立完毕
                    my_cmd_data_send(0x0a, uuid_answer_buf, 17);
                    extern void hicar_reconnect_connected(u8 local_id);
                    hicar_reconnect_connected(local_id);

                }
    } else {
        /* 单服务注册后通道从首个槽位分配，不能再按固定 local_id 判断投屏类型。
         * 根据连接的 local_id 查找实际注册记录，再上报对应 UUID。
         */
        int slot;
        for (slot = 0; slot < USER_SDP_SERVICE_MAX; slot++) {
            BTRfcRegUUID_t *info = &user_sdp_ser[slot].user_rfcomm_info;
            if (user_sdp_ser[slot].working_flag == 0xA5 &&
                info->channel_id == local_id && info->dUUIDLen == 16) {
                const u8 *report_uuid = info->pbUUID;
                /* CarPlay 使用 iAP2 建链，但沿用协议向主控上报 CarPlay 服务 UUID。 */
                if (!memcmp(report_uuid, apple_iap2_uuid_128_1, 16)) {
                    report_uuid = carplay_service_uuid_128;
                }
                if (!memcmp(info->pbUUID, carlife_service_uuid_128, 16)) {
                    /* 仅 CarLife 通道成功才停止其重试，避免其他服务误停回连。 */
                    carlife_conn_end();
                }
                /* 通过 0xC0 通知主控实际连接的服务类型。 */
                my_cmd_data_send(0xc0, (u8 *)report_uuid, 16);
                break;
            }
        }
        if (slot == USER_SDP_SERVICE_MAX) {
            /* 未匹配到注册记录时仅打印异常，不猜测或伪造上报 UUID。 */
            printf("[RFCOMM] No registered UUID for local_id=%u\n", local_id);
        }
    }

        u8 auto_reconn = 1;
        syscfg_write(MY_AUTO_RECONNECT_FLAG, &auto_reconn, 1);
        break;
    case 2://断开连接事件
        rfcomm_connect_flag = 0;
        printf("nutil disconnect#########\n");
        gx_iap2_id = 0;
        // if (reconnect_mgr.type == CONNECTIONMODE_Hicar) {
        //     extern void hicar_reconnect_disconnected(u8 local_id);
        //     hicar_reconnect_disconnected(local_id);
        // }
        // iap2 rfcomm disconnect
        my_cmd_data_send(0xcf, 0, 0);
        break;
    case 7:
        rfcomm_connect_flag = 1;
        // RFCOMM接收到数据事件
        printf("GET PKG >>>>>>>>>>>>#########\n");
        // put_buf(packet, size);
        printf("CONNECTIONMODE_Hicar=%d\n",reconnect_mgr.type);
        if(reconnect_mgr.type == CONNECTIONMODE_Hicar){
            hc_packet[0] = local_id; // 首字节填入通道 ID，方便主控区分是哪条信令流
            memcpy(hc_packet + 1, packet, size); // 将蓝牙接收到的原始数据包拷贝至缓冲区
            // 通过串口向 C3000 主控发送 Command 0x0C 命令，透传 HiCar 握手认证报文
            my_cmd_data_send(MY_IAP_SEND_DATA_NEW, hc_packet, size + 1);
        }else{
            my_cmd_data_send(0xc2, packet, size);
        }
        
        break;
    }
}



/**************************************************/
/*******set eir data************/
#define EIR_MAX_SUPPORT_UUID  3
u8   UUID16_buf_len = 0;
u8   UUID32_buf_len = 0;
u8   UUID128_buf_len = 0;
u16  UUID16_buf[EIR_MAX_SUPPORT_UUID] = {0};
u32  UUID32_buf[EIR_MAX_SUPPORT_UUID] = {0};
u8   UUID128_buf[EIR_MAX_SUPPORT_UUID][16];

static const u8 de_size_128_carplay[]  = {0x2D, 0x8D, 0x24, 0x66, 0xE1, 0x4D, 0x45, 0x1C, 0x88, 0xBC, 0x73, 0x01, 0xAB, 0xEA, 0x29, 0x1A};

// 添加用户自定义数据到EIR广播包
// eir: EIR数据缓存地址
u8 user_add_data_2_eir(u8 *eir)
{
    u8 len = 0;
    u8 tx_power = 3; //dbm
    u8 *eir_len_ptr = NULL;
    u8 *user_eir_begin = eir;
    /////for test;
#if 0
    UUID16_buf_len = 2;
    UUID16_buf[0] = 0x1101;
    UUID16_buf[1] = 0x1102;
    UUID128_buf_len = 2;
    memcpy(&UUID128_buf[0][0], de_size_128_carplay, 16);
    memcpy(&UUID128_buf[1][0], de_size_128_carplay, 16);
#endif
    *eir++ = 1 + 1;
    *eir++ = 0x0A;
    *eir++ = tx_power;
    len = 3;


    eir_len_ptr = eir;
    if (UUID16_buf_len > 0) {
        *eir++ = 1 + UUID16_buf_len * 2;//每个UUID 2个byte + type 1 byte
        *eir++ = 0x02;
        for (int i = 0; i < UUID16_buf_len; i++) {
            little_endian_store_16(eir, 0, UUID16_buf[i]);
            eir += 2;
        }
        len += (1 + UUID16_buf_len * 2 + 1);
    }
    if (UUID128_buf_len > 0) {
        *eir++ = 1 + UUID128_buf_len * 16;//每个UUID 16个byte + type 1 byte
        *eir++ = 0x06;
        for (int i = 0; i < UUID128_buf_len; i++) {
            memcpy(eir, &UUID128_buf[i][0], 16);
            eir += 16;
        }
        len += (1 + UUID128_buf_len * 16 + 1);
    }
    printf("user_add_data_2_eir");
    put_buf(user_eir_begin, len);
    return (len);
}

int uuid_bufffer_clean()
{
    //??????????λ?ó???????
    memset(UUID16_buf, 0, sizeof(UUID16_buf));
    memset(UUID32_buf, 0, sizeof(UUID32_buf));
    memset(UUID128_buf, 0, sizeof(UUID128_buf));
    UUID16_buf_len = 0;
    UUID32_buf_len = 0;
    UUID128_buf_len = 0;
}
typedef enum {
    BT_SERVICE_UUID_16 = 0x03,//16bit uuid
    BT_SERVICE_UUID_32 = 0x05,//32bit uuid
    BT_SERVICE_UUID_128 = 0x07,//128bit uuid
} BTServiceUuid_e;
/*使用时，先关闭可发现可连接，连续配置完再打开可发现可连接*/
int BTSrv_IF_Srv_setLocalUuid(BTServiceUuid_e eUuidType, unsigned char len, unsigned char *pUuid)
{
    //大小端实际调试的时候自己调整一下
    //先保存下来，重新打开可发现的时候重新生成的eir数据包
    if (BT_SERVICE_UUID_16 == eUuidType) {
        u16 temp_uuid = little_endian_read_16(pUuid, 0);
        for (int i = 0; i < EIR_MAX_SUPPORT_UUID; i++) {
            if (UUID16_buf[i] == temp_uuid) {
                //the same not save
                return 0;
            }
        }
        UUID16_buf[UUID16_buf_len] = temp_uuid;
        if (++UUID16_buf_len > EIR_MAX_SUPPORT_UUID) {
            printf("buffer UUID16 not enough, please check\n");
        }
    }
    if (BT_SERVICE_UUID_128 == eUuidType) {
        for (int i = 0; i < EIR_MAX_SUPPORT_UUID; i++) {
            if (!memcmp(&UUID128_buf[i][0], pUuid, 16)) {
                //the same not save
                return 0;
            }
        }
        memcpy(&UUID128_buf[UUID128_buf_len][0], pUuid, 16);
        if (++UUID128_buf_len > EIR_MAX_SUPPORT_UUID) {
            printf("buffer UUID128 not enough, please check\n");
        }
    }
    /*应该用不上
    if(BT_SERVICE_UUID_32 == eUuidType){
        u32 temp_uuid = little_endian_read_32(pUuid, 0);
        for(int i=0;i < EIR_MAX_SUPPORT_UUID;i++){
            if(UUID32_buf[i]==temp_uuid){
                //the same not save
                return 0;
            }
        }
        UUID32_buf[UUID32_buf_len] = temp_uuid;
        if(++UUID32_buf_len > EIR_MAX_SUPPORT_UUID){
           printf("buffer UUID32 not enough, please check\n");
        }
    }*/
}

/***********************************************/
/***get eir data***/
#define EIR_TYPE_INCOMPLETE_LIST_OF_UUID_16      0x02
#define EIR_TYPE_COMPLETE_LIST_OF_UUID_16        0x03
#define EIR_TYPE_INCOMPLETE_LIST_OF_UUID_32      0x04
#define EIR_TYPE_COMPLETE_LIST_OF_UUID_32        0x05
#define EIR_TYPE_INCOMPLETE_LIST_OF_UUID_128     0x06
#define EIR_TYPE_COMPLETE_LIST_OF_UUID_128       0x07
#define MAX_EIR_LENGTH    64
typedef struct BTSrvEirUuid_ {
    UINT8 uuid_len;//uuid长度
    UINT8 uuid[MAX_EIR_LENGTH]; //eir中uuid内容， 可能有一个uuid或者多个uuid
} BTSrvUuid_t;
//获取远端设备EIR中的uuid信息
typedef struct local_save_eir_uuid {
    UINT8 addr[6];
    UINT8 uuid_len;//uuid长度
    UINT8 uuid[MAX_EIR_LENGTH]; //eir中uuid内容， 可能有一个uuid或者多个uuid
} local_save_eir_t;

local_save_eir_t local_save_eir;
extern void bt_search_device(void);
int BTSrv_IF_Srv_getDeviceUuid(u8 *pstBtAddr, u8 *pstUuid)
{
    //异步获取过程， 没办法直接返回， 进一步讨论如何返回
    memcpy(local_save_eir.addr, pstBtAddr, 6);
    //下面的接口开始搜索EIR包
    u8 inquiry_length = 20;   // inquiry_length * 1.28s
    user_send_cmd_prepare(USER_CTRL_SEARCH_DEVICE, 1, &inquiry_length);
}

void search_device_UUID_info(u8 *addr, u8 *data, u16 len)
{
    printf("search_device_UUID_info\n");
    put_buf(addr, 6);
    put_buf(data, len);
}

void inquiry_filter_UUID_info(u8 *addr, u8 *eir, u16 len)
{
    //printf("inquiry_filter_UUID_info\n");
    //put_buf(addr,6);
    //put_buf(eir,len);
    u8 *uuid_p = eir;

    u16 remote_uuid_len = 0;
    int search_uuid_len = 0;

    if (len == 0) {
        printf("eir data len = 0\n");
        return;
    }
    search_uuid_len = len;
    u8 *UUID_info = malloc(search_uuid_len);
    memset(UUID_info, 0, search_uuid_len);
    while (search_uuid_len > 0) {
        if (uuid_p[0] == 0) {
            break;
        }
        if ((uuid_p[1] >= EIR_TYPE_INCOMPLETE_LIST_OF_UUID_16) && (uuid_p[1] <= EIR_TYPE_COMPLETE_LIST_OF_UUID_128)) {
            if (uuid_p[0] > 1) {
                printf("eir uuid type %d, len %d\n", uuid_p[1], uuid_p[0] + 1);
                memcpy(UUID_info + remote_uuid_len, uuid_p, uuid_p[0] + 1);
                remote_uuid_len += (uuid_p[0] + 1);
            }
        }
        search_uuid_len -= uuid_p[0] + 1;
        uuid_p += uuid_p[0] + 1;
    }
    if (remote_uuid_len > 0) {
        search_device_UUID_info(addr, UUID_info, remote_uuid_len);
    }
    free(UUID_info);
}






/*-----------------------------------------------------extern---------------------------------------------------------------*/
void MY_BT_INIT_OK();
void my_connect_hfp();



/*-----------------------------------------------------MY API---------------------------------------------------------------*/
extern u8 my_phone_addr_test[6];
/**
 * ????? CarPlay ????????????? EIR ????
 */
extern switch_ble_mac_hicar;
void carplay_bluetooth_init()
{
    if(reconnect_mgr.type == CONNECTIONMODE_Hicar){
        return;
    }
    if(switch_ble_mac_hicar == 1){
        switch_to_carplay_mode();
    }
    BTSrv_IF_clearAllSdpRfcommUuid();
    uuid_bufffer_clean();

    /* 按 SOC 下发的模式同步设置 SDP 服务和 EIR 广播，避免手机连接到其他投屏服务。 */
    switch (reconnect_mgr.type) {
    case CONNECTIONMODE_Carplay:
        /* 注册 iAP2 通道，广播 CarPlay 服务和对应的 iAP2 UUID。 */
        BTSrv_IF_regSdpRfcommUuid((u8 *)apple_iap2_uuid_128_1, 16);
        BTSrv_IF_Srv_setLocalUuid(BT_SERVICE_UUID_128, 16, (u8 *)carplay_service_uuid_128);
        BTSrv_IF_Srv_setLocalUuid(BT_SERVICE_UUID_128, 16, (u8 *)apple_iap2_uuid_128_3);
        break;
    case CONNECTIONMODE_Android_Auto:
        /* AA 模式仅注册、广播 AA UUID。 */
        BTSrv_IF_regSdpRfcommUuid((u8 *)Android_auto_uuid_128, 16);
        BTSrv_IF_Srv_setLocalUuid(BT_SERVICE_UUID_128, 16, (u8 *)Android_auto_uuid_128);
        break;
    case CONNECTIONMODE_Carlife:
        /* CarLife 模式仅注册、广播 CarLife UUID，避免双协议手机优先连接 AA。 */
        BTSrv_IF_regSdpRfcommUuid((u8 *)carlife_service_uuid_128, 16);
        BTSrv_IF_Srv_setLocalUuid(BT_SERVICE_UUID_128, 16, (u8 *)carlife_service_uuid_128);
        break;
    default:
        /* 普通蓝牙及未处理模式不注册投屏 UUID；HFP、A2DP 等服务由各自配置管理。 */
        BTSrv_IF_regSdpRfcommUuid((u8 *)Android_auto_uuid_128, 16);
        BTSrv_IF_regSdpRfcommUuid((u8 *)apple_iap2_uuid_128_1, 16);
        BTSrv_IF_regSdpRfcommUuid((u8 *)normal_service_uuid_128, 16);
        BTSrv_IF_Srv_setLocalUuid(BT_SERVICE_UUID_128, 16, (u8 *)Android_auto_uuid_128);
        BTSrv_IF_Srv_setLocalUuid(BT_SERVICE_UUID_128, 16, (u8 *)carplay_service_uuid_128);
        BTSrv_IF_Srv_setLocalUuid(BT_SERVICE_UUID_128, 16, (u8 *)apple_iap2_uuid_128_3);
        break;
    }

    user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_ENABLE, 0, NULL);
    user_send_cmd_prepare(USER_CTRL_WRITE_CONN_ENABLE, 0, NULL);


}

/**
 * send test
 */
void my_carplay_send_test()
{
    u8 data[6] = {0xFF, 0x55, 0x02, 0x00, 0xEE, 0x10};
    RFCOMM_IF_sendData(NULL, 0x0A, data, 6);
    log_info("my_carplay_send_test 0 ...... \n");
}

void my_get_deviceUUid()
{
    BTSrv_IF_Srv_getDeviceUuid(my_phone_addr_test, carplay_service_uuid_128);
    printf("my_get_deviceUUid : ( 0 )");
}

void my_iap2_disconnect()
{
    RFCOMM_IF_disconnect(NULL, 0x0A);
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
    // extern BT_CONNCTION_FLAG;
    // extern u8 phone_addr_temp1[6];
    // if(BT_CONNCTION_FLAG ==1){
    //     hicar_reconnect_start(phone_addr_temp1);
    // }
}
#endif
