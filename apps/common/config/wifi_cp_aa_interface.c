#include "system/includes.h"
#include "app_config.h"
#include "btcontroller_config.h"
#include "btstack/bt_profile_config.h"
#include "bt_common.h"
#include "btstack/avctp_user.h"

#define LOG_TAG     "[BT-CFG]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE

#include "debug.h"
#include "app_main.h"

#if TCFG_APP_BT_EN

u8 gx_iap2_id = 0;
#if RFCOMM_UUID_ANDROID_AUTO_ENABLE

/* CarPlay 服务标识，用于广播及向主控上报服务类型。 */
const u8 carplay_service_uuid_128[16] = {0xEC, 0x88, 0x43, 0x48, 0xCD, 0x41, 0x40, 0xA2, 0x97, 0x27, 0x57, 0x5D, 0x50, 0xBF, 0x1F, 0xD3};

/* 苹果设备的 CarPlay 服务标识。 */
const u8 apple_device_carplay_uuid_128[16] = {0x2D, 0x8D, 0x24, 0x66, 0xE1, 0x4D, 0x45, 0x1C, 0x88, 0xBC, 0x73, 0x01, 0xAB, 0xEA, 0x29, 0x1A};

/* iAP2 服务标识及不同后缀、字节序形式，使用时保持协议要求的顺序。 */
const u8 apple_iap2_uuid_128_1[16] = {0x00, 0x00, 0x00, 0x00, 0xde, 0xca, 0xfa, 0xde, 0xde, 0xca, 0xde, 0xaf, 0xde, 0xca, 0xca, 0xff};
const u8 apple_iap2_uuid_128_2[16] = {0x00, 0x00, 0x00, 0x00, 0xde, 0xca, 0xfa, 0xde, 0xde, 0xca, 0xde, 0xaf, 0xde, 0xca, 0xca, 0xfe};
const u8 apple_iap2_uuid_128_3[16] = {0xff, 0xca, 0xca, 0xde, 0xaf, 0xde, 0xca, 0xde, 0xde, 0xfa, 0xca, 0xde, 0x00, 0x00, 0x00, 0x00};
const u8 apple_iap2_uuid_128_4[16] = {0xfe, 0xca, 0xca, 0xde, 0xaf, 0xde, 0xca, 0xde, 0xde, 0xfa, 0xca, 0xde, 0x00, 0x00, 0x00, 0x00};
#endif  

#if RFCOMM_UUID_ANDROID_AUTO_ENABLE

/* 标准串口服务及 Android Auto 服务标识。 */
const u8 normal_service_uuid_128[16] = {0x00, 0x00, 0x11, 0x01, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5f, 0x9b, 0x34, 0xfb};
const u8 Android_auto_uuid_128[16] = {0x4d, 0xe1, 0x7a, 0x00, 0x52, 0xcb, 0x11, 0xe6, 0xbd, 0xf4, 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66};
const u8 Android_auto_uuid_128_2[16] = {0x66, 0x9a, 0x0c, 0x20, 0x00, 0x08, 0xf4, 0xbd, 0xe6, 0x11, 0xcb, 0x52, 0x00, 0x7a, 0xe1, 0x4d};
#endif  
#if RFCOMM_UUID_CARLIFE_ENABLE

/* CarLife 专用服务与标准串口服务标识。 */
const u8 carlife_service_uuid_128[16] = {0xa4, 0x5b, 0xc7, 0xe5, 0xbb, 0x50, 0x49, 0x49, 0x9d, 0xe1, 0xf7, 0x82, 0x99, 0xcf, 0x6d, 0x78};
const u8 carlife_SerialPort_uuid_128[16] = {0x00, 0x00, 0x11, 0x01, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5f, 0x9b, 0x34, 0xfb};
#endif  

#if RFCOMM_UUID_HICAR_ENABLE

/* HiCar FE35 服务标识。 */
const u8 hicar_fe35_uuid_128[16] = {
    0x00,0x00,0xFE,0x35,
    0x00,0x00,
    0x10,0x00,
    0x80,0x00,
    0x00,0x80,
    0x5F,0x9B,
    0x34,0xFB
};

/* HiCar FE36 服务标识。 */
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

/* 投屏服务标识表：iAP2、Android Auto、CarLife。 */
const u8 *car_uuid_buf[CAR_UUID_NUM] =
{
    apple_iap2_uuid_128_1,
    Android_auto_uuid_128,
    carlife_service_uuid_128
};

/* 对应的主控上报标识表，其中 iAP2 对应上报 CarPlay 标识。 */
const u8 *car_uuid_answer_buf[CAR_UUID_NUM] =
{
    carplay_service_uuid_128,
    Android_auto_uuid_128,
    carlife_service_uuid_128
};

typedef unsigned short UINT8;

typedef struct {

/* 服务记录链表节点；该字段必须保持在结构体首位。 */
    void *offset_item;

    u32        service_record_handle;
    u8         *service_record;
} service_record_item_t;

#define USER_SDP_RECORD_HANDLER_REGISTER(handler) \
	const service_record_item_t  handler \
		sec(.sdp_record_item)

u8 spp_up_profile_support = 1;

#define USER_SDP_SERVICE_MAX 3

typedef struct {
/* 注册结果、UUID 字节长度、UUID 内容及本地 RFCOMM 通道号。 */
    u32  dResult;            
    u32  dUUIDLen;     

    char pbUUID[16];            
    unsigned int channel_id;             
} BTRfcRegUUID_t;
#define RFCOMM_CH_SPP_A   10
#define RFCOMM_CH_SPP_B   RFCOMM_CH_SPP_A+1
#define RFCOMM_CH_SPP_C   RFCOMM_CH_SPP_A+2

typedef struct {
    u8 working_flag;
    BTRfcRegUUID_t user_rfcomm_info;
} user_sdp_service_t;

user_sdp_service_t user_sdp_ser[USER_SDP_SERVICE_MAX];

/* 128 位 UUID 的 SDP 记录模板，注册时填入实际 UUID、句柄和通道号。 */
const u8 rfcomm_sdp_data_sample_for_uuid128[96] = {
    0x36, 0x00, 0x5B, 0x09, 0x00, 0x00, 0x0A, 0x00, 0x01, 0x00, 0x11, 0x09, 0x00, 0x01, 0x36, 0x00,
    0x11, 0x1C, 0xfe, 0x01, 0x00, 0x00, 0x12, 0x34, 0x56, 0x78, 0xab, 0xcd, 0x00, 0x80, 0x5F, 0x9b,
    0x34, 0xfb, 0x09, 0x00, 0x04, 0x36, 0x00, 0x0E, 0x36, 0x00, 0x03, 0x19, 0x01, 0x00, 0x36, 0x00,
    0x05, 0x19, 0x00, 0x03, 0x08, 0x0a, 0x09, 0x00, 0x09, 0x36, 0x00, 0x17, 0x36, 0x00, 0x14, 0x1C,
    0xfe, 0x01, 0x00, 0x00, 0x12, 0x34, 0x56, 0x78, 0xab, 0xcd, 0x00, 0x80, 0x5F, 0x9b, 0x34, 0xfb,
    0x09, 0x01, 0x00, 0x09, 0x01, 0x00, 0x25, 0x06, 0x4A, 0x4C, 0x5F, 0x53, 0x50, 0x50, 0x00, 0x00,
};
/* 16 位 UUID 的 SDP 记录模板。 */
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

u8 rfcomm_connect_flag = 0;

/* SDP 模板中需要替换的字段偏移，分别对应句柄、服务 UUID、通道和协议描述。 */
#define SERVICE_RECORD_HANDLE_INDEX             10
#define SERVICE_CLASS_UUID_INDEX                18
#define SERVICE_CHANNEL_ID_UUIF16_INDEX         39
#define SERVICE_CHANNEL_ID_UUIF128_INDEX        53
#define SERVICE_PROFILE_DEC_UUIF16_INDEX        50
#define SERVICE_PROFILE_DEC_UUIF128_INDEX       64
/* 注册本地 RFCOMM 服务：分配空闲槽位，并按 UUID 长度生成 SDP 记录。 */
int BTSrv_IF_regSdpRfcommUuid(u8 *pbUuid, int sdUuidLen)     
{
    int i = 0;
    int sdp_data_len = 0;
    BTRfcRegUUID_t *reg_btrfc_info = NULL;
    u8 *reg_sdp_index = NULL;
    u8 req_sdp_recoed_handle = 0;
    u8 rfcomm_ch_id = 0;

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

    printf("user_sdp_service_data\n");
    put_buf(reg_sdp_index, sdp_data_len);
    return 0;
}

/* 清空全部自定义服务及其 SDP 数据，供模式重新配置时使用。 */
void BTSrv_IF_clearAllSdpRfcommUuid(void)
{

    memset(user_sdp_ser, 0, sizeof(user_sdp_ser));

    memset(user_sdp_service_data, 0, sizeof(user_sdp_service_data));
}
/* 按 UUID 和长度注销服务；返回 0 表示找到并清除，1 表示未找到。 */
int BTSrv_IF_unregSdpRfcommUuid(u8 *pbUuid, int sdUuidLen)   
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

    printf("why no find the UUID to release\n");
    return 1;
}

/* 过滤 SDP 记录：自定义槽位仅在已注册时可见，其他记录交由协议栈处理。 */
int user_define_sdp_info_filter(u8 *current_record)      
{

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

    return 0;
}

typedef struct {
    u32    dResult;            
    u32    dHandle;            
    u32    dUUIDLen;     
    char   pbUUID[16];            
} BTRfcConnect_t;
/* 保存最近一次主动连接请求的信息及匹配到的本地通道。 */
BTRfcConnect_t want_to_connect_info;
/* 远端 SDP 查询结果回调，仅记录是否找到对应的 RFCOMM 通道。 */
u8 user_sdp_uuid_connect_result(u8 flag)
{

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
/* 查找本地已注册服务，发起远端 SDP 查询及 RFCOMM 连接；返回 0 表示已发起请求。 */
int RFCOMM_IF_connect(u8 *pstBtAddr, unsigned char *pbUUID, int dUUIDLen)    
{

    int i = 0;
    BTRfcRegUUID_t *sdp_index = NULL;
    for (i = 0; i < USER_SDP_SERVICE_MAX; i++) {
        if (user_sdp_ser[i].working_flag == 0xA5) {
            sdp_index = &user_sdp_ser[i].user_rfcomm_info;
            if ((sdp_index->dUUIDLen == dUUIDLen) && !memcmp(sdp_index->pbUUID, pbUUID, dUUIDLen - 1)) {

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

/* 请求断开最近记录的 RFCOMM 通道；当前实现未使用传入的地址和句柄。 */
int RFCOMM_IF_disconnect(u8 *pstBtAddr, int pHandle)
{

    u8 local_cid = gx_iap2_id;
    multi_spp_disconnect_channel(local_cid);
}
/* 通过最近记录的 RFCOMM 通道发送数据，未使用传入的通道句柄。 */
int RFCOMM_IF_sendData(u8 *pstBtAddr, int sdHandle, char *pbData, u16 wDataLen)  
{

    u8 local_cid = gx_iap2_id;

    multi_spp_send_data(local_cid, 0, pbData, wDataLen);
}

/* 通过调用方指定的本地 RFCOMM 通道发送数据。 */
int RFCOMM_IF_sendData_New(u8 *pstBtAddr, int sdHandle, char *pbData, u16 wDataLen)  
{
    printf("local_cid : %d", sdHandle);
    put_buf(pbData, wDataLen);
    u8 local_cid = sdHandle;
    multi_spp_send_data(local_cid, 0, pbData, wDataLen);
}

void my_cmd_data_send(u8 cmd, u8* data, u16 data_len);
void my_uart_send_data(u8 * buf, u16 buf_len);
u8 my_connect_phone_type = 0;

/* 返回当前保存的远端手机类型。 */
u8 gx_get_conn_remote_type()
{
    return my_connect_phone_type;
}

extern void carlife_conn_end();
extern u8 reconnect_flag;
static u8 hc_packet[1024];
u8 uuid_answer_buf[1+16] = {0};
/* 处理 RFCOMM 事件：1 为连接、2 为断开、7 为收数，并通过串口通知主控。 */
int mutil_handle_data_deal(u8 local_id, u8 packet_type, u16 channel, u8 *packet, u16 size)
{

    printf("local_id %d\n", local_id);
    printf("channel %d\r\n", channel);
    printf("my_connect_phone_type %d\r\n", my_connect_phone_type);
    switch (packet_type) {
    case 1:

        printf("\n");
        printf("nutil connect#########\n");
        gx_iap2_id = local_id;

        reconnect_flag = 0;

        if(reconnect_mgr.type == CONNECTIONMODE_Hicar){

                printf("HiCar mode, local_id: %d\n", local_id);
                if(local_id == 0x0A)
                {

                    my_cmd_data_send(
                        0xc0,
                        hicar_fe35_uuid_128,
                        16
                    );

                    uuid_answer_buf[0] = 0x0a;
                    memcpy(uuid_answer_buf+1, hicar_fe35_uuid_128, 16);

                    my_cmd_data_send(0x0a, uuid_answer_buf, 17);
                    extern void hicar_reconnect_connected(u8 local_id);
                    hicar_reconnect_connected(local_id);

                }
                else if(local_id == 0x0B)
                {

                    my_cmd_data_send(
                        0xc0,
                        hicar_fe36_uuid_128,
                        16
                    );

                    uuid_answer_buf[0] = 0x0b;
                    memcpy(uuid_answer_buf+1, hicar_fe36_uuid_128, 16);

                    my_cmd_data_send(0x0a, uuid_answer_buf, 17);
                    extern void hicar_reconnect_connected(u8 local_id);
                    hicar_reconnect_connected(local_id);

                }
    } else {

/* 按实际注册表查找通道对应的服务，不能将槽位固定解释为某种投屏类型。 */
        int slot;
        for (slot = 0; slot < USER_SDP_SERVICE_MAX; slot++) {
            BTRfcRegUUID_t *info = &user_sdp_ser[slot].user_rfcomm_info;
            if (user_sdp_ser[slot].working_flag == 0xA5 &&
                info->channel_id == local_id && info->dUUIDLen == 16) {
                const u8 *report_uuid = info->pbUUID;

                if (!memcmp(report_uuid, apple_iap2_uuid_128_1, 16)) {
/* iAP2 建链沿用串口协议，上报为 CarPlay 服务。 */
                    report_uuid = carplay_service_uuid_128;
                }
                if (!memcmp(info->pbUUID, carlife_service_uuid_128, 16)) {

/* 只有 CarLife 服务连接成功，才停止 CarLife 回连定时器。 */
                    carlife_conn_end();
                }

                my_cmd_data_send(0xc0, (u8 *)report_uuid, 16);
                break;
            }
        }
        if (slot == USER_SDP_SERVICE_MAX) {

            printf("[RFCOMM] No registered UUID for local_id=%u\n", local_id);
        }
    }

        u8 auto_reconn = 1;
        syscfg_write(MY_AUTO_RECONNECT_FLAG, &auto_reconn, 1);
        break;
    case 2: 
        rfcomm_connect_flag = 0;
        printf("nutil disconnect#########\n");
        gx_iap2_id = 0;

        my_cmd_data_send(0xcf, 0, 0);
        break;
    case 7:
        rfcomm_connect_flag = 1;

        printf("GET PKG >>>>>>>>>>>>#########\n");

        printf("CONNECTIONMODE_Hicar=%d\n",reconnect_mgr.type);
        if(reconnect_mgr.type == CONNECTIONMODE_Hicar){
/* HiCar 透传数据前加本地通道号，供主控区分 FE35 和 FE36。 */
            hc_packet[0] = local_id;  
            memcpy(hc_packet + 1, packet, size);  

            my_cmd_data_send(MY_IAP_SEND_DATA_NEW, hc_packet, size + 1);
        }else{
            my_cmd_data_send(0xc2, packet, size);
        }

        break;
    }
}

/* 经典蓝牙 EIR 广播的自定义 UUID 缓存，每种列表最多保存三项。 */
#define EIR_MAX_SUPPORT_UUID  3
u8   UUID16_buf_len = 0;
u8   UUID32_buf_len = 0;
u8   UUID128_buf_len = 0;
u16  UUID16_buf[EIR_MAX_SUPPORT_UUID] = {0};
u32  UUID32_buf[EIR_MAX_SUPPORT_UUID] = {0};
u8   UUID128_buf[EIR_MAX_SUPPORT_UUID][16];

static const u8 de_size_128_carplay[]  = {0x2D, 0x8D, 0x24, 0x66, 0xE1, 0x4D, 0x45, 0x1C, 0x88, 0xBC, 0x73, 0x01, 0xAB, 0xEA, 0x29, 0x1A};

/* 将发射功率及缓存的 16 位、128 位 UUID 列表写入 EIR，返回写入长度。 */
u8 user_add_data_2_eir(u8 *eir)
{
    u8 len = 0;
    u8 tx_power = 3;  
    u8 *eir_len_ptr = NULL;
    u8 *user_eir_begin = eir;

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
        *eir++ = 1 + UUID16_buf_len * 2; 
        *eir++ = 0x02;
        for (int i = 0; i < UUID16_buf_len; i++) {
            little_endian_store_16(eir, 0, UUID16_buf[i]);
            eir += 2;
        }
        len += (1 + UUID16_buf_len * 2 + 1);
    }
    if (UUID128_buf_len > 0) {
        *eir++ = 1 + UUID128_buf_len * 16; 
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

/* 清空 EIR UUID 缓存和数量，避免不同模式的广播内容残留。 */
int uuid_bufffer_clean()
{

    memset(UUID16_buf, 0, sizeof(UUID16_buf));
    memset(UUID32_buf, 0, sizeof(UUID32_buf));
    memset(UUID128_buf, 0, sizeof(UUID128_buf));
    UUID16_buf_len = 0;
    UUID32_buf_len = 0;
    UUID128_buf_len = 0;
}
typedef enum {
    BT_SERVICE_UUID_16 = 0x03, 
    BT_SERVICE_UUID_32 = 0x05, 
    BT_SERVICE_UUID_128 = 0x07, 
} BTServiceUuid_e;

/* 缓存本地广播 UUID，重复项不再添加；重新生成 EIR 时使用这些内容。 */
int BTSrv_IF_Srv_setLocalUuid(BTServiceUuid_e eUuidType, unsigned char len, unsigned char *pUuid)
{

    if (BT_SERVICE_UUID_16 == eUuidType) {
        u16 temp_uuid = little_endian_read_16(pUuid, 0);
        for (int i = 0; i < EIR_MAX_SUPPORT_UUID; i++) {
            if (UUID16_buf[i] == temp_uuid) {

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

                return 0;
            }
        }
        memcpy(&UUID128_buf[UUID128_buf_len][0], pUuid, 16);
        if (++UUID128_buf_len > EIR_MAX_SUPPORT_UUID) {
            printf("buffer UUID128 not enough, please check\n");
        }
    }

}

#define EIR_TYPE_INCOMPLETE_LIST_OF_UUID_16      0x02
#define EIR_TYPE_COMPLETE_LIST_OF_UUID_16        0x03
#define EIR_TYPE_INCOMPLETE_LIST_OF_UUID_32      0x04
#define EIR_TYPE_COMPLETE_LIST_OF_UUID_32        0x05
#define EIR_TYPE_INCOMPLETE_LIST_OF_UUID_128     0x06
#define EIR_TYPE_COMPLETE_LIST_OF_UUID_128       0x07
#define MAX_EIR_LENGTH    64
typedef struct BTSrvEirUuid_ {
    UINT8 uuid_len; 
    UINT8 uuid[MAX_EIR_LENGTH];  
} BTSrvUuid_t;

typedef struct local_save_eir_uuid {
    UINT8 addr[6];
    UINT8 uuid_len; 
    UINT8 uuid[MAX_EIR_LENGTH];  
} local_save_eir_t;

local_save_eir_t local_save_eir;
extern void bt_search_device(void);
/* 保存待查询地址并启动异步蓝牙搜索；当前接口不直接返回 UUID。 */
int BTSrv_IF_Srv_getDeviceUuid(u8 *pstBtAddr, u8 *pstUuid)
{

    memcpy(local_save_eir.addr, pstBtAddr, 6);

    u8 inquiry_length = 20;    
    user_send_cmd_prepare(USER_CTRL_SEARCH_DEVICE, 1, &inquiry_length);
}

/* 输出搜索到的远端地址及 UUID 数据。 */
void search_device_UUID_info(u8 *addr, u8 *data, u16 len)
{
    printf("search_device_UUID_info\n");
    put_buf(addr, 6);
    put_buf(data, len);
}

/* 遍历远端 EIR 字段，提取 UUID 列表并交给搜索结果处理函数。 */
void inquiry_filter_UUID_info(u8 *addr, u8 *eir, u16 len)
{

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

void MY_BT_INIT_OK();
void my_connect_hfp();

extern u8 my_phone_addr_test[6];

extern switch_ble_mac_hicar;
/* 按主控下发模式配置本地 SDP 与 EIR；HiCar 使用独立初始化入口。 */
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

/* 仅开放当前模式对应的投屏服务；普通蓝牙及未处理模式不注册投屏 UUID。 */
    switch (reconnect_mgr.type) {
    case CONNECTIONMODE_Carplay:

        BTSrv_IF_regSdpRfcommUuid((u8 *)apple_iap2_uuid_128_1, 16);
        BTSrv_IF_Srv_setLocalUuid(BT_SERVICE_UUID_128, 16, (u8 *)carplay_service_uuid_128);
        BTSrv_IF_Srv_setLocalUuid(BT_SERVICE_UUID_128, 16, (u8 *)apple_iap2_uuid_128_3);
        break;
    case CONNECTIONMODE_Android_Auto:

        BTSrv_IF_regSdpRfcommUuid((u8 *)Android_auto_uuid_128, 16);
        BTSrv_IF_Srv_setLocalUuid(BT_SERVICE_UUID_128, 16, (u8 *)Android_auto_uuid_128);
        break;
    case CONNECTIONMODE_Carlife:

        BTSrv_IF_regSdpRfcommUuid((u8 *)carlife_service_uuid_128, 16);
        BTSrv_IF_Srv_setLocalUuid(BT_SERVICE_UUID_128, 16, (u8 *)carlife_service_uuid_128);
        break;
    default:

        break;
    }

    user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_ENABLE, 0, NULL);
    user_send_cmd_prepare(USER_CTRL_WRITE_CONN_ENABLE, 0, NULL);

}

/* 向当前 RFCOMM 通道发送固定测试数据。 */
void my_carplay_send_test()
{
    u8 data[6] = {0xFF, 0x55, 0x02, 0x00, 0xEE, 0x10};
    RFCOMM_IF_sendData(NULL, 0x0A, data, 6);
    log_info("my_carplay_send_test 0 ...... \n");
}

/* 触发测试手机的异步服务发现。 */
void my_get_deviceUUid()
{
    BTSrv_IF_Srv_getDeviceUuid(my_phone_addr_test, carplay_service_uuid_128);
    printf("my_get_deviceUUid : ( 0 )");
}

/* 调用适配层断开当前记录的 RFCOMM 通道。 */
void my_iap2_disconnect()
{
    RFCOMM_IF_disconnect(NULL, 0x0A);
}

/* 注销原投屏服务，注册 HiCar FE35、FE36，并更新对应的 EIR 广播。 */
void hicar_mode_init(void)
{
    printf("enter hicar mode\n");

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

    uuid_bufffer_clean();

    BTSrv_IF_regSdpRfcommUuid(
        (u8 *)hicar_fe35_uuid_128,
        16
    );

    BTSrv_IF_regSdpRfcommUuid(
        (u8 *)hicar_fe36_uuid_128,
        16
    );

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
#endif
