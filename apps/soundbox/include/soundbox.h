#ifndef SOUNDBOX_H
#define SOUNDBOX_H

#include "app_config.h"
#include "system/includes.h"

#define HCI_EVENT_INQUIRY_COMPLETE                             0x01  // HCI事件：设备查询（搜索）完成
#define HCI_EVENT_CONNECTION_COMPLETE                          0x03  // HCI事件：连接建立完成
#define HCI_EVENT_DISCONNECTION_COMPLETE                       0x05  // HCI事件：连接断开完成
#define HCI_EVENT_PIN_CODE_REQUEST                             0x16  // HCI事件：请求输入 PIN 码
#define HCI_EVENT_IO_CAPABILITY_REQUEST                        0x31  // HCI事件：请求 IO 能力（用于配对安全级别协商）
#define HCI_EVENT_USER_CONFIRMATION_REQUEST                    0x33  // HCI事件：请求用户确认配对（如数值比较）
#define HCI_EVENT_USER_PASSKEY_REQUEST                         0x34  // HCI事件：请求用户输入配对 Passkey（6位数字）
#define HCI_EVENT_USER_PRESSKEY_NOTIFICATION                   0x3B  // HCI事件：按键按下通知（在输入 Passkey 时的按键反馈）
#define HCI_EVENT_VENDOR_NO_RECONN_ADDR                        0xF8  // 厂商自定义 HCI事件：无重连地址通知
#define HCI_EVENT_VENDOR_REMOTE_TEST                           0xFE  // 厂商自定义 HCI事件：远程测试
#define BTSTACK_EVENT_HCI_CONNECTIONS_DELETE                   0x6D  // BTstack协议栈事件：删除 HCI 连接记录


#define ERROR_CODE_SUCCESS                                     0x00  // 错误码：成功 / 无错误
#define ERROR_CODE_PAGE_TIMEOUT                                0x04  // 错误码：寻呼超时（连接设备超时未响应）
#define ERROR_CODE_AUTHENTICATION_FAILURE                      0x05  // 错误码：身份验证失败（配对/认证失败）
#define ERROR_CODE_PIN_OR_KEY_MISSING                          0x06  // 错误码：缺少 PIN 码或密钥（链路密钥丢失）
#define ERROR_CODE_CONNECTION_TIMEOUT                          0x08  // 错误码：连接超时（链路监控超时断开）
#define ERROR_CODE_SYNCHRONOUS_CONNECTION_LIMIT_TO_A_DEVICE_EXCEEDED  0x0A  // 错误码：超出对单台设备的同步连接（SCO/eSCO）数量限制
#define ERROR_CODE_ACL_CONNECTION_ALREADY_EXISTS               0x0B  // 错误码：ACL 链路连接已存在
#define ERROR_CODE_CONNECTION_REJECTED_DUE_TO_LIMITED_RESOURCES        0x0D  // 错误码：因资源不足拒绝连接请求
#define ERROR_CODE_CONNECTION_REJECTED_DUE_TO_UNACCEPTABLE_BD_ADDR    0x0F  // 错误码：因不接受该蓝牙地址（BD_ADDR）而拒绝连接
#define ERROR_CODE_CONNECTION_ACCEPT_TIMEOUT_EXCEEDED          0x10  // 错误码：接受连接超时
#define ERROR_CODE_REMOTE_USER_TERMINATED_CONNECTION           0x13  // 错误码：远程设备用户主动终止了连接
#define ERROR_CODE_CONNECTION_TERMINATED_BY_LOCAL_HOST         0x16  // 错误码：本地主机主动终止了连接


#define CUSTOM_BB_AUTO_CANCEL_PAGE                             0xFD  // 自定义基带事件：应用层自动取消 Page（寻呼）
#define BB_CANCEL_PAGE                                         0xFE  // 基带事件：基带层取消 Page（寻呼）


#if ((TCFG_USER_BLE_ENABLE)&&(CONFIG_BT_MODE == BT_NORMAL))

#if SMART_BOX_EN

int smartbox_bt_state_init();
int smartbox_bt_state_set_page_scan_enable();
int smartbox_bt_state_get_connect_mac_addr();
int smartbox_bt_state_cancel_page_scan();
int smartbox_bt_state_enter_soft_poweroff();
int smartbox_bt_state_tws_init(int paired);
int smartbox_bt_state_tws_connected(int first_pair, u8 *comm_addr);
int sys_event_handler_specific(struct sys_event *event);

#define BT_STATE_INIT()                   smartbox_bt_state_init()
#define BT_STATE_SET_PAGE_SCAN_ENABLE()   smartbox_bt_state_set_page_scan_enable()
#define BT_STATE_GET_CONNECT_MAC_ADDR()   smartbox_bt_state_get_connect_mac_addr()
#define BT_STATE_CANCEL_PAGE_SCAN()       smartbox_bt_state_cancel_page_scan()
#define BT_STATE_ENTER_SOFT_POWEROFF()    smartbox_bt_state_enter_soft_poweroff()
#define BT_STATE_TWS_INIT(a)              smartbox_bt_state_tws_init(a)
#define BT_STATE_TWS_CONNECTED(a, b)      smartbox_bt_state_tws_connected(a,b)
#define BT_BREDR_HANDLE_REG()
#define SYS_EVENT_HANDLER_SPECIFIC(a)     sys_event_handler_specific(a)

#elif TRANS_DATA_EN

int trans_data_bt_state_init();
int trans_data_bt_state_set_page_scan_enable();
int trans_data_bt_state_get_connect_mac_addr();
int trans_data_bt_state_cancel_page_scan();
int trans_data_bt_state_enter_soft_poweroff();
int trans_data_bt_state_tws_init(int paired);
int trans_data_bt_state_tws_connected(int first_pair, u8 *comm_addr);
void trans_data_bt_bredr_handle_reg(void);
int trans_data_sys_event_handler_specific(struct sys_event *event);

#define BT_STATE_INIT()                   trans_data_bt_state_init()
#define BT_STATE_SET_PAGE_SCAN_ENABLE()   trans_data_bt_state_set_page_scan_enable()
#define BT_STATE_GET_CONNECT_MAC_ADDR()   trans_data_bt_state_get_connect_mac_addr()
#define BT_STATE_CANCEL_PAGE_SCAN()       trans_data_bt_state_cancel_page_scan()
#define BT_STATE_ENTER_SOFT_POWEROFF()    trans_data_bt_state_enter_soft_poweroff()
#define BT_STATE_TWS_INIT(a)              trans_data_bt_state_tws_init(a)
#define BT_STATE_TWS_CONNECTED(a, b)      trans_data_bt_state_tws_connected(a,b)
#define BT_BREDR_HANDLE_REG()			  do { } while(0)
#define SYS_EVENT_HANDLER_SPECIFIC(a)     trans_data_sys_event_handler_specific(a)

#elif ANCS_CLIENT_EN

int ancs_data_bt_state_init();
int ancs_data_bt_state_set_page_scan_enable();
int ancs_data_bt_state_get_connect_mac_addr();
int ancs_data_bt_state_cancel_page_scan();
int ancs_data_bt_state_enter_soft_poweroff();
int ancs_data_bt_state_tws_init(int paired);
int ancs_data_bt_state_tws_connected(int first_pair, u8 *comm_addr);
int ancs_data_sys_event_handler_specific(struct sys_event *event);

#define BT_STATE_INIT()                   ancs_data_bt_state_init()
#define BT_STATE_SET_PAGE_SCAN_ENABLE()   ancs_data_bt_state_set_page_scan_enable()
#define BT_STATE_GET_CONNECT_MAC_ADDR()   ancs_data_bt_state_get_connect_mac_addr()
#define BT_STATE_CANCEL_PAGE_SCAN()       ancs_data_bt_state_cancel_page_scan()
#define BT_STATE_ENTER_SOFT_POWEROFF()    ancs_data_bt_state_enter_soft_poweroff()
#define BT_STATE_TWS_INIT(a)              ancs_data_bt_state_tws_init(a)
#define BT_STATE_TWS_CONNECTED(a, b)      ancs_data_bt_state_tws_connected(a,b)
#define BT_BREDR_HANDLE_REG()
#define SYS_EVENT_HANDLER_SPECIFIC(a)     ancs_data_sys_event_handler_specific(a)

#elif BLE_CLIENT_EN

#define BT_STATE_INIT()
#define BT_STATE_SET_PAGE_SCAN_ENABLE()
#define BT_STATE_GET_CONNECT_MAC_ADDR()
#define BT_STATE_CANCEL_PAGE_SCAN()
#define BT_STATE_ENTER_SOFT_POWEROFF()
#define BT_STATE_TWS_INIT(a)
#define BT_STATE_TWS_CONNECTED(a, b)
#define BT_BREDR_HANDLE_REG()
#define SYS_EVENT_HANDLER_SPECIFIC(a)

#else

#define BT_STATE_INIT()                   do { } while(0)
#define BT_STATE_SET_PAGE_SCAN_ENABLE()   do { } while(0)
#define BT_STATE_GET_CONNECT_MAC_ADDR()   do { } while(0)
#define BT_STATE_CANCEL_PAGE_SCAN()       do { } while(0)
#define BT_STATE_ENTER_SOFT_POWEROFF()    do { } while(0)
#define BT_STATE_TWS_INIT(a)              do { } while(0)
#define BT_STATE_TWS_CONNECTED(a, b)      do { } while(0)
#define BT_BREDR_HANDLE_REG()			  do { } while(0)
#define SYS_EVENT_HANDLER_SPECIFIC(a)     do { } while(0)

#endif

#else

#define BT_STATE_INIT()                   do { } while(0)
#define BT_STATE_SET_PAGE_SCAN_ENABLE()   do { } while(0)
#define BT_STATE_GET_CONNECT_MAC_ADDR()   do { } while(0)
#define BT_STATE_CANCEL_PAGE_SCAN()       do { } while(0)
#define BT_STATE_ENTER_SOFT_POWEROFF()    do { } while(0)
#define BT_STATE_TWS_INIT(a)              do { } while(0)
#define BT_STATE_TWS_CONNECTED(a, b)      do { } while(0)
#define BT_BREDR_HANDLE_REG()			  do { } while(0)
#define SYS_EVENT_HANDLER_SPECIFIC(a)     do { } while(0)

#endif





#endif

