#include "btstack/bluetooth.h"
#include "system/includes.h"
#include "asm/uart_dev.h"
#include "system/event.h"
#include "user_cfg.h"
#include "btstack/avctp_user.h"
#include "app_power_manage.h"
#include "app_task.h"
#include "app_main.h"
#include "audio_config.h"
#include "fm_emitter/fm_emitter_manage.h"
#include "asm/fm_inside_api.h"
#include "media/eq_config.h"
#include "media/audio_eq.h"
#include "asm/mcpwm.h"

void uart_data_received(u8 *data, u16 data_len);
void gx_uart_get_data(u8 *data, u16 data_len);
void my_cmd_data_send(u8 cmd, u8 *data, u16 data_len);
int bt_modify_name(u8 *new_name);
void bt_hci_event_connection_timeout(struct bt_event * bt);
void gx_update_uart_init();
extern u8 gx_call_phonenumber[20];
extern u8 phone_callback_flag;
extern u8 BT_CONNCTION_FLAG;
extern u8 hfp_ignore_income_connect;
u8 gx_sco_conn_status = 0;

u8 gx_get_sco_conn_status()
{
    return gx_sco_conn_status;
}

extern u8 gx_get_conn_remote_type();
/*--------------------------------*/
u8 reconnect_flag = 0;
HC_BT_OPEN hc_open_bt = {0};
#if 1
/*
    [[  注意!!!  ]]
    * 如果当系统任务较少时使用本demo，需要将低功耗关闭（#define TCFG_LOWPOWER_LOWPOWER_SEL    0//SLEEP_EN ），否则任务被串口接收函数调用信号量pend时会导致cpu休眠，串口中断和DMA接收将遗漏数据或数据不正确
*/

#define UART_DEV_USAGE_TEST_SEL 2 // uart_dev.c api接口使用方法选择
//  选择1  串口中断回调函数推送事件，由事件响应函数接收串口数据
//  选择2  由task接收串口数据

#define UART_DEV_TEST_MULTI_BYTE 1 // uart_dev.c 读写多个字节api / 读写1个字节api 选择

#define UART_DEV_FLOW_CTRL 0

static u8 uart_cbuf[2048] __attribute__((aligned(4)));
static u8 uart_rxbuf[2048] __attribute__((aligned(4)));

static void my_put_u8hex(u8 dat)
{
    u8 tmp;
    tmp = dat / 16;
    if (tmp < 10)
    {
        putchar(tmp + '0');
    }
    else
    {
        putchar(tmp - 10 + 'A');
    }
    tmp = dat % 16;
    if (tmp < 10)
    {
        putchar(tmp + '0');
    }
    else
    {
        putchar(tmp - 10 + 'A');
    }
    putchar(0x20);
}

// 设备事件响应demo
static void uart_event_handler(struct sys_event *e)
{
    const uart_bus_t *uart_bus;
    u32 uart_rxcnt = 0;

    if ((u32)e->arg == DEVICE_EVENT_FROM_UART_RX_OVERFLOW)
    {
        if (e->u.dev.event == DEVICE_EVENT_CHANGE)
        {
            /* printf("uart event: DEVICE_EVENT_FROM_UART_RX_OVERFLOW\n"); */
            uart_bus = (const uart_bus_t *)e->u.dev.value;
            uart_rxcnt = uart_bus->read(uart_rxbuf, sizeof(uart_rxbuf), 0);
            if (uart_rxcnt)
            {
                printf("get_buffer1:\n");
                for (int i = 0; i < uart_rxcnt; i++)
                {
                    my_put_u8hex(uart_rxbuf[i]);
                    if (i % 16 == 15)
                    {
                        putchar('\n');
                    }
                }
                if (uart_rxcnt % 16)
                {
                    putchar('\n');
                }
#if (!UART_DEV_FLOW_CTRL)
                uart_bus->write(uart_rxbuf, uart_rxcnt);
#endif
            }
            printf("uart out\n");
        }
    }
    if ((u32)e->arg == DEVICE_EVENT_FROM_UART_RX_OUTTIME)
    {
        if (e->u.dev.event == DEVICE_EVENT_CHANGE)
        {
            /* printf("uart event:DEVICE_EVENT_FROM_UART_RX_OUTTIME\n"); */
            uart_bus = (const uart_bus_t *)e->u.dev.value;
            uart_rxcnt = uart_bus->read(uart_rxbuf, sizeof(uart_rxbuf), 0);
            if (uart_rxcnt)
            {
                printf("get_buffer2:\n");
                for (int i = 0; i < uart_rxcnt; i++)
                {
                    my_put_u8hex(uart_rxbuf[i]);
                    if (i % 16 == 15)
                    {
                        putchar('\n');
                    }
                }
                if (uart_rxcnt % 16)
                {
                    putchar('\n');
                }
#if (!UART_DEV_FLOW_CTRL)
                uart_bus->write(uart_rxbuf, uart_rxcnt);
#endif
            }
            printf("uart out\n");
        }
    }
}
SYS_EVENT_HANDLER(SYS_DEVICE_EVENT, uart_event_handler, 0);

static void uart_u_task(void *arg)
{
    const uart_bus_t *uart_bus = arg;
    int ret;
    u32 uart_rxcnt = 0;

    printf("uart_u_task start\n");
    while (1)
    {
#if !UART_DEV_TEST_MULTI_BYTE
        // uart_bus->getbyte()在尚未收到串口数据时会pend信号量，挂起task，直到UART_RX_PND或UART_RX_OT_PND中断发生，post信号量，唤醒task
        ret = uart_bus->getbyte(&uart_rxbuf[0], 0);
        if (ret)
        {
            uart_rxcnt = 1;
            printf("get_byte: %02x\n", uart_rxbuf[0]);
            uart_bus->putbyte(uart_rxbuf[0]);
        }
#else
        // uart_bus->read()在尚未收到串口数据时会pend信号量，挂起task，直到UART_RX_PND或UART_RX_OT_PND中断发生，post信号量，唤醒task
        uart_rxcnt = uart_bus->read(uart_rxbuf, sizeof(uart_rxbuf), 0);
        if (uart_rxcnt)
        {
            printf("get_buffer3:\r\n");
            // put_buf(uart_rxbuf, uart_rxcnt);
            uart_data_received(uart_rxbuf, uart_rxcnt);
#if (!UART_DEV_FLOW_CTRL)
            // uart_bus->write(uart_rxbuf, uart_rxcnt);
#endif
        }
#endif
    }
}

static void uart_isr_hook(void *arg, u32 status)
{
    const uart_bus_t *ubus = arg;
    struct sys_event e;

    // 当CONFIG_UARTx_ENABLE_TX_DMA（x = 0, 1）为1时，不要在中断里面调用ubus->write()，因为中断不能pend信号量
    if (status == UT_RX)
    {
        printf("uart_rx_isr\n");
#if (UART_DEV_USAGE_TEST_SEL == 1)
        e.type = SYS_DEVICE_EVENT;
        e.arg = (void *)DEVICE_EVENT_FROM_UART_RX_OVERFLOW;
        e.u.dev.event = DEVICE_EVENT_CHANGE;
        e.u.dev.value = (int)ubus;
        sys_event_notify(&e);
#endif
    }
    if (status == UT_RX_OT)
    {
        printf("uart_rx_ot_isr\n");
#if (UART_DEV_USAGE_TEST_SEL == 1)
        e.type = SYS_DEVICE_EVENT;
        e.arg = (void *)DEVICE_EVENT_FROM_UART_RX_OUTTIME;
        e.u.dev.event = DEVICE_EVENT_CHANGE;
        e.u.dev.value = (int)ubus;
        sys_event_notify(&e);
#endif
    }
}

static void uart_flow_ctrl_task(void *arg)
{
    const uart_bus_t *uart_bus = arg;
    while (1)
    {
        uart_bus->write("flow control test ", sizeof("flow control test "));
        os_time_dly(100);
    }
}

const uart_bus_t *uart_bus;
static OS_MUTEX uart_mutex;
void uart_queue_init(void);
void uart_dev_test_main()
{
    struct uart_platform_data_t u_arg = {0};
    u_arg.tx_pin = IO_PORTA_05;
    u_arg.rx_pin = IO_PORTA_06;
    u_arg.rx_cbuf = uart_cbuf;
    u_arg.rx_cbuf_size = 2048;
    u_arg.frame_length = 1024;
    u_arg.rx_timeout = 5;
    u_arg.isr_cbfun = uart_isr_hook;
    // u_arg.baud = 115200;
    u_arg.baud = 2000000;
    u_arg.is_9bit = 0;
#if UART_DEV_FLOW_CTRL
    u_arg.tx_pin = IO_PORTA_00;
    u_arg.rx_pin = IO_PORTA_01;
    u_arg.baud = 1000000;
    extern void flow_ctl_hw_init(void);
    flow_ctl_hw_init();
#endif
    uart_bus = uart_dev_open(&u_arg);
    if (uart_bus != NULL)
    {
        uart_queue_init();
        os_mutex_create(&uart_mutex);
        printf("uart_dev_open() success\n");
        printf("uart baud :%d\r\n", u_arg.baud);
#if (UART_DEV_USAGE_TEST_SEL == 2)
        os_task_create(uart_u_task, (void *)uart_bus, 6, 1024, 0, "uart_u_task");
#endif
#if UART_DEV_FLOW_CTRL
        os_task_create(uart_flow_ctrl_task, (void *)uart_bus, 31, 128, 0, "flow_ctrl");
#endif
    }
}

void uart_dev_test_del()
{
    os_task_del("uart_u_task");
    os_mutex_del(&uart_mutex, 0);
    uart_dev_close(uart_bus);
    uart_bus = NULL;
    printf("uart_dev_del() success\n");
    // os_time_dly(10);
}

#if UART_DEV_FLOW_CTRL
void uart_change_rts_state(void)
{
    static u8 rts_state = 1;
    extern void change_rts_state(u8 state);
    change_rts_state(rts_state);
    rts_state = !rts_state;
}
#endif

void gx_uart_send_data(u8 *cbuf, uint16_t cbuf_len)
{
    os_mutex_pend(&uart_mutex, 0);
    uart_bus->write(cbuf, cbuf_len);
    os_mutex_post(&uart_mutex);
}
////////////////////////////////////////////////////////////////////////\


int gx_ble_hid_pack_frame(u8 id, u8 *data, u16 data_len, u8 *buffer)
{
    buffer[0] = 0xAC;
    buffer[1] = 0x69;

    u16 pkg_length = 10 + data_len;

    buffer[2] = (u8)(pkg_length & 0xFF); // 低八位;
    buffer[3] = (u8)(pkg_length >> 8);   // 高八位
    buffer[4] = 0x00;
    buffer[5] = id;
    buffer[6] = (u8)(data_len & 0xFF);
    buffer[7] = (u8)(data_len >> 8); // 高八位

    for (int i = 0; i < data_len; i++)
    {
        buffer[8 + i] = data[i];
    }

    u16 checksum = 0;
    for (u16 i = 0; i < pkg_length - 2; ++i)
    { // 不包括校验和部分
        checksum += buffer[i];
        // printf("for times %d-- %x\n", i, checksum);
    }

    buffer[pkg_length - 2] = (u8)(checksum & 0xFF); // 低八位
    buffer[pkg_length - 1] = (u8)(checksum >> 8);   // 高八位

    return pkg_length;
}

void my_cmd_err_send(u8 cmd, u8 data, u16 data_len)
{
    u8 buffer_err[20] = {0};
    int pkg_len_err = gx_ble_hid_pack_frame(cmd, &data, data_len, buffer_err); // 组包

    gx_uart_send_data(buffer_err, pkg_len_err);
}
// 解析状态定义
typedef enum
{
    STATE_WAIT_HEAD1 = 0,    // 等待帧头第一个字节 0xAC
    STATE_WAIT_HEAD2,        // 等待帧头第二个字节 0x69
    STATE_WAIT_LENGTH_LOW,   // 等待长度低字节
    STATE_WAIT_LENGTH_HIGH,  // 等待长度高字节
    STATE_WAIT_RESERVED,     // 等待保留字节
    STATE_WAIT_CMD,          // 等待命令字节
    STATE_WAIT_CMD_LEN_LOW,  // 等待命令长度低字节
    STATE_WAIT_CMD_LEN_HIGH, // 等待命令长度高字节
    STATE_WAIT_DATA,         // 等待数据部分
    STATE_WAIT_CHECKSUM      // 等待校验和
} parse_state_t;

// 全局变量
u8 gx_ble_hid_buf[600];
u16 gx_ble_hid_total_len = 0;
u16 gx_ble_hid_cmd_len = 0;
u16 gx_ble_hid_current_len = 0;
parse_state_t gx_ble_hid_state = STATE_WAIT_HEAD1;
u8 gx_ble_hid_cmd = 0;

u16 gx_uart_check_data_sum(u8 *data, u16 data_len)
{
    u16 tmp_sum = 0;
    for (int i = 0; i < data_len - 2; i++)
    {
        tmp_sum += data[i];
    }

    return tmp_sum;
}


void return_uart_notify(u8 cmd, u16 cmd_len, u8 *data, u16 data_len)
{
    /* memcpy(cur_433_buffer, buffer, size); */
    struct sys_event e;
    e.type = SYS_DEVICE_EVENT;
    e.arg = DEVICE_UART_MASSAGE_NOTIFY;
    e.u.gx_uart.cmd = cmd;
    e.u.gx_uart.cmd_len = cmd_len;

    u8 *data_copy = malloc(data_len);
    if (data_copy)
    {
        memcpy(data_copy, data, data_len);
        e.u.gx_uart.rx_buf = data_copy;
        e.u.gx_uart.rx_buf_len = data_len;
        sys_event_notify(&e);
    }
}

//=======================================pwm配置=========================================//
void my_pwm_init(u32 my_fre, u32 my_pwm_ch, u32 my_time_ch, u32 port_h, u32 port_l)
{
    struct pwm_platform_data pwm_p_data;
    pwm_p_data.pwm_aligned_mode = pwm_edge_aligned; // 边沿对齐
    pwm_p_data.frequency = my_fre;                  // 1KHz

    pwm_p_data.pwm_ch_num = my_pwm_ch;     // pwm_ch1;                        //通道0
    pwm_p_data.pwm_timer_num = my_time_ch; // pwm_timer1;                  //时基选择通道0
    pwm_p_data.duty = 0;                   // 占空比50%
    pwm_p_data.h_pin = port_h;             // IO_PORTB_00;                                  //没有则填 -1。h_pin_output_ch_num无效，可不配置
    pwm_p_data.l_pin = port_l;             // 硬件引脚，l_pin_output_ch_num无效，可不配置
    pwm_p_data.h_pin_output_ch_num = 1;
    pwm_p_data.l_pin_output_ch_num = -1;

    pwm_p_data.complementary_en = 1;       // 两个引脚的波形同步

    mcpwm_init(&pwm_p_data);
}

typedef struct __hc_gpio_set
{
    u32 gpio_pin;
    u8 gpio_pin_mode;
    u8 gpio_pin_old_value;
    u8 gpio_pin_value;
    // u8 gpio_set_ret;
} hc_gpio_set;

// for test
hc_gpio_set gx_hc_gpio[20] =
    {
        0};

void gx_hc_gpio_in_read()
{
    u8 answer_buf[2] = {0};

    for (u8 i = 0; i < 20; i++)
    {
        if (gx_hc_gpio[i].gpio_pin != 0 && gx_hc_gpio[i].gpio_pin_mode == 0)
        {
            gx_hc_gpio[i].gpio_pin_value = gpio_read(gx_hc_gpio[i].gpio_pin);
            if (gx_hc_gpio[i].gpio_pin_old_value != gx_hc_gpio[i].gpio_pin_value)
            {
                printf("index : %d\r\n", i);
                printf("GPIO : %d, GPIO_OLD_VALUE : %d\r\n", gx_hc_gpio[i].gpio_pin, gx_hc_gpio[i].gpio_pin_old_value);
                printf("GPIO : %d, GPIO_VALUE : %d\r\n", gx_hc_gpio[i].gpio_pin, gx_hc_gpio[i].gpio_pin_value);

                answer_buf[0] = gx_hc_gpio[i].gpio_pin;
                answer_buf[1] = gx_hc_gpio[i].gpio_pin_value;
                my_cmd_data_send(GX_SEND_BT_GPI_STAT, answer_buf, 2);
                gx_hc_gpio[i].gpio_pin_old_value = gx_hc_gpio[i].gpio_pin_value;
            }
        }
    }
}
#if 0
void gx_hc_gpio_out_default_init()
{
    for(u8 i = 0; i < 20; i++)
    {
        if(gx_hc_gpio[i].gpio_pin != 0 && gx_hc_gpio[i].gpio_pin_mode == 1)
        {
            gpio_set_die(gx_hc_gpio[i].gpio_pin, 1);  //数字态 IPA2
            gpio_set_direction(gx_hc_gpio[i].gpio_pin, 0); //输出
            gpio_set_pull_down(gx_hc_gpio[i].gpio_pin, 0); //下拉关
            gpio_set_pull_up(gx_hc_gpio[i].gpio_pin, 0); //上拉开
            gpio_direction_output(gx_hc_gpio[i].gpio_pin, gx_hc_gpio[i].gpio_pin_value);//输出高
        }
    }
}
#endif

int gx_hc_gpio_init_check(u32 gpio_pin, u8 pin_mode)
{
    for (u8 i = 0; i < 20; i++)
    {
        if (gx_hc_gpio[i].gpio_pin == gpio_pin && gx_hc_gpio[i].gpio_pin_mode == pin_mode)
        {
            return 0;
        }
    }

    return -1;
}

int gx_set_gpio_function_init(u32 gpio_pin, u8 function)
{
    if (function > 3)
    {
        printf("不支持的function\r\n");
        return -1;
    }

    switch (function)
    {
    case 0: // gpio_in
        /* code */
        gpio_set_die(gpio_pin, 1);
        gpio_set_direction(gpio_pin, 1);
        gpio_set_pull_up(gpio_pin, 0);
        gpio_set_pull_down(gpio_pin, 0);
        break;

    case 1: // gpio_out
        /* code */
        gpio_set_direction(gpio_pin, 0);
        gpio_set_pull_up(gpio_pin, 0);
        gpio_set_pull_down(gpio_pin, 0);
        break;

    case 2: // gpio_pwm
        /* code */

        break;

    case 3: // gpio_adc
        /* code */
        gpio_set_die(gpio_pin, 0);
        gpio_set_direction(gpio_pin, 1);
        gpio_set_pull_up(gpio_pin, 0);
        gpio_set_pull_down(gpio_pin, 0);
        break;

    default:
        break;
    }

    for (u8 i = 0; i < 20; i++)
    {
        if (gx_hc_gpio[i].gpio_pin == gpio_pin)
        {
            gx_hc_gpio[i].gpio_pin_old_value = 2;
            gx_hc_gpio[i].gpio_pin_mode = function;
            printf("00 index : %d\r\n", i);
            break;
        }

        if (gx_hc_gpio[i].gpio_pin == 0)
        {
            gx_hc_gpio[i].gpio_pin_old_value = 2;
            gx_hc_gpio[i].gpio_pin_mode = function;
            gx_hc_gpio[i].gpio_pin = gpio_pin;
            printf("11 index : %d\r\n", i);
            break;
        }
    }

    return 0;
}

#if 1
extern int ble_list_smp_pair_set(u8 mode);
extern void user_hid_data_send(u8 report_id, u8 *buff, u16 len);
extern u8 hid_report_map[512];
extern u16 hid_map_len;
int gx_user_set_hid_map(u8 *report_map, u16 report_map_len)
{
    printf("user_write_hid_map \n");
    printf("memcmp(hid_report_map,set_map,map_len) : %d\n", memcmp(hid_report_map, report_map, report_map_len));
    printf("set_map:");
    put_buf(report_map, report_map_len);
    printf("hid_report_map:");
    put_buf(hid_report_map, report_map_len);
    if (memcmp(hid_report_map, report_map, report_map_len) == 0)
    {
        printf("report map is same \n");
        my_cmd_err_send(MY_CMD_OK, 0, 0);
    }
    else
    {
        printf("report map is not same \n");
        memset(hid_report_map, 0, sizeof(hid_report_map));
        memcpy(hid_report_map, report_map, report_map_len);
        // 紧跟在 memset/memcpy 下面同步更新全局长度变量
        hid_map_len = report_map_len; 
        int ret = syscfg_write(GX_USER_HID_REPORT_MAP, hid_report_map, report_map_len);
        if (ret < 0) {
            printf("Error: Flash write failed for map data\n");
        }
        syscfg_write(CFG_HQ_HID_MAP_LEN, &report_map_len, sizeof(u16));
        u8 hc_cpureset = 1;
        syscfg_write(CFG_HQ_CPU_RESET_FLAG, &hc_cpureset, sizeof(u8));
        // 4. Disconnect & Reset Procedure
        ble_list_smp_pair_set(4);
        ble_app_disconnect();
        // os_time_dly(10);
        // cpu_reset();
    }
}

static void user_read_hid_map()
{
    int ret = 0;
    syscfg_read(CFG_HQ_HID_MAP_LEN, &hid_map_len, sizeof(u16));
    ret = syscfg_read(GX_USER_HID_REPORT_MAP, hid_report_map, hid_map_len);
    printf("hid_map_len: %d ----- ret : [%d]\n", hid_map_len, ret);

    // u8 hc_cpureset = 0;
    // syscfg_read(CFG_HQ_CPU_RESET_FLAG, &hc_cpureset, sizeof(u8));
}
#endif

void gx_hc_app_init()
{
    // 获取hid 描述符
    user_read_hid_map();

    // gpio_in 扫描开
    sys_timer_add(NULL, gx_hc_gpio_in_read, 100);

    // 上电默认gpio初始化  (暂未启用)
    /* gx_hc_gpio_out_default_init(); */
}

int gx_uart_ble_hid_handle(u8 cmd, u16 cmd_len, u8 *data, u16 data_len)
{
    // printf("ble_data handle");
    u8 cmd_data[256];

    if (cmd == GX_SET_HID_MESSAGE)
    {
        if (cmd_len)
        {
            for (u16 i = 0; i < cmd_len; i++)
            {
                cmd_data[i] = data[8 + i]; // cmd赋值
            }
        }

        printf("cmd b1 send hid data ....\r\n");
        user_hid_data_send(cmd_data[0], cmd_data + 1, cmd_len - 1);
        my_cmd_err_send(MY_CMD_OK, 0, 0);
        return 1;
    }

    return 0;
}

static u8 clear_buf[512] = {0};
int gx_uart_event_handler(struct notify_uart *nfuart)
{
    printf("start parse\r\n");
    int ret = 0;
    u8 cmd_data_tmp[1024];
    u8 answer_data[128], answer_addr[6];
    u16 answer_len = 0;

    // printf("buf_cnt [%d], cmd_len [%d]\r\n", nfuart->rx_buf_len, nfuart->cmd_len);
    // put_buf(nfuart->rx_buf, nfuart->rx_buf_len);

    if (nfuart->cmd_len)
    {
        wdt_clear();
        for (u16 i = 0; i < nfuart->cmd_len; i++)
        {
            cmd_data_tmp[i] = nfuart->rx_buf[8 + i]; // cmd赋值
        }
    }

    switch (nfuart->cmd)
    {
#if 1
    case GX_SET_BT_PIN_FUNCTION:
        printf(">>> --- GX_SET_BT_PIN_FUNCTION --- <<<\n");
        ret = gx_set_gpio_function_init(cmd_data_tmp[0], cmd_data_tmp[1]);

        if(ret)
        {
            my_cmd_err_send(MY_CMD_OK, 0, 0);
        }
    break;

    case GX_SET_BT_GPO_POL:
        ret = gx_hc_gpio_init_check(cmd_data_tmp[0], 1);
        if(!ret)
        {
            gpio_write(cmd_data_tmp[0], cmd_data_tmp[1]);
        }else
        {
            printf("this gpio is not init or init err\r\n");
        }
    break;

    case GX_SET_BT_PWM_PARAM:
        if(cmd_data_tmp[1] == 0x00)
        {
            my_pwm_init(cmd_data_tmp[2]*1000 ,pwm_ch1, pwm_timer1, cmd_data_tmp[0], -1);
        }else if(cmd_data_tmp[1] = 0x01)
        {
            mcpwm_set_duty(pwm_ch1, pwm_timer1, cmd_data_tmp[2]*100); //PB9
        }
    break;

#endif

    case MY_IAP_GET_DATA:
        printf(">>> --- MY_IAP_GET_DATA --- <<<\n");

        RFCOMM_IF_sendData(NULL, 0x0A, cmd_data_tmp, nfuart->cmd_len);
        my_cmd_err_send(MY_CMD_OK, 0, 0);
        break;

    case MY_SET_BT_NAME:
        printf(">>> --- MY_SET_BT_NAME --- <<<\n");
        u8 my_name[32] = {0};
        ret = memcmp(cmd_data_tmp, get_edr_name(), strlen(get_edr_name()));
        if(ret == 0)
        {
            printf("name is same\r\n");
            my_cmd_err_send(MY_CMD_ERR, 5, 1);
        }else
        {
            memcpy(my_name, cmd_data_tmp, nfuart->cmd_len);
  
            if(get_total_connect_dev() == 0)
            {
                printf("no remote conn\r\n");
                user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_DISABLE, 0, NULL);
                user_send_cmd_prepare(USER_CTRL_WRITE_CONN_DISABLE, 0, NULL);
                user_send_cmd_prepare(USER_CTRL_PAGE_CANCEL, 0, NULL);
                user_send_cmd_prepare(USER_CTRL_CONNECTION_CANCEL, 0, NULL);
            }else
            {
                printf("some remote is conn\r\n");
            }

            printf("name : %s|\r\n", my_name);
            ret = bt_modify_name(my_name);
            if(ret == 1)
            {
                printf("set name success\r\n");
                my_cmd_err_send(MY_CMD_OK, 0, 0);
            }else
            {
                printf("set name err\r\n");
                my_cmd_err_send(MY_CMD_ERR, 5, 1);
            }

            if(hc_open_bt.bt_open == 1)
            {
                // 重新启用扫描和连接功能
                user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_ENABLE, 0, NULL);
                user_send_cmd_prepare(USER_CTRL_WRITE_CONN_ENABLE, 0, NULL);
            }
        }
        break;

    case MY_TURN_ON_BT:
        printf("---------------------------------\r\n");
        printf("---------------------------------\r\n");
        printf("OPEN BT ....\r\n");
        extern void carplay_bluetooth_init();
        carplay_bluetooth_init();

        u8 auto_reconn_flag = 0;
        syscfg_read(MY_AUTO_RECONNECT_FLAG, &auto_reconn_flag, 1);
        // clear_current_poweron_memory_search_index(0);
        
        hc_open_bt.bt_open = 1;
        syscfg_write(CFG_HC_BT_OPEN_STATSUS, &hc_open_bt, sizeof(HC_BT_OPEN));
        if(reconnect_mgr.type != CONNECTIONMODE_Hicar){
             if (auto_reconn_flag == 1)
            {
                reconnect_flag = 1;
                printf("reconn ..... \r\n");
                bt_hci_event_connection_timeout(NULL);
                // bt_wait_connect_and_phone_connect_switch(0);
            }
            else
            {
                bt_hci_event_connection_timeout(NULL);
                // bt_wait_connect_and_phone_connect_switch(0);
            }
        }
       
        
        my_cmd_err_send(MY_CMD_OK, 0, 0);
        printf("OPEN BT END ....");
        printf("---------------------------------\r\n");
        printf("---------------------------------\r\n");
        break;
    case MY_IAP_GET_DATA_NEW:
        printf(">>> --- MY_IAP_hicar_GET_DATA NEW --- <<<\n");

        RFCOMM_IF_sendData_New(NULL, cmd_data_tmp[0], cmd_data_tmp+1,nfuart->cmd_len - 1);
        my_cmd_err_send(MY_CMD_OK, 0, 0);
        break;

    case MY_TURN_OFF_BT:

        printf("close bt .......\r\n");

        user_send_cmd_prepare(USER_CTRL_DISCONNECTION_HCI, 0, NULL); // 断开当前所有蓝牙连接

        user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_DISABLE, 0, NULL); // 关闭蓝牙可发现
        user_send_cmd_prepare(USER_CTRL_WRITE_CONN_DISABLE, 0, NULL); // 关闭蓝牙可连接

        hc_open_bt.bt_open = 0;
        syscfg_write(CFG_HC_BT_OPEN_STATSUS, &hc_open_bt, sizeof(HC_BT_OPEN));
        
        my_cmd_err_send(MY_CMD_OK, 0, 0);
        break;
#if 1
    case MY_SET_BT_MODE:
        printf("set bt mode %d, \r\n", cmd_data_tmp[0]);
        switch (cmd_data_tmp[0])
        {
        case 0:
            /* code */ // 空闲.....
            app_task_switch_to(APP_IDLE_TASK);
            my_cmd_err_send(MY_CMD_OK, 0, 0);
            break;

        case 1:
            /* code */ // iis....
            app_task_switch_to(APP_LINEIN_TASK);
            my_cmd_err_send(MY_CMD_OK, 0, 0);
            break;

        case 2:
            /* code */ // bt ....
            app_task_switch_to(APP_BT_TASK);
            my_cmd_err_send(MY_CMD_OK, 0, 0);
            break;

        case 3:
            /* code */ // fmrx ....
            app_task_switch_to(APP_FM_TASK);
            my_cmd_err_send(MY_CMD_OK, 0, 0);
            break;

        default:
            my_cmd_err_send(MY_CMD_ERR, 0x01, 1);
            break;
        }

        break;
#endif

    case MY_SET_BT_RESET:
        cpu_reset();
        break;

    case MY_BT_SET_PIN_PAIR:
        // int RFCOMM_IF_disconnect(u8 *pstBtAddr, int pHandle);
        // RFCOMM_IF_disconnect(0,0);

        u8 pair_flag_tmp = 0;
        ret = syscfg_read(CFG_HC_PAIR_PIN_CODE, &pair_flag_tmp, 1);
        printf("MY_BT_SET_PIN_PAIR : will_set[%d], state_now[%d]\r\n", cmd_data_tmp[0], pair_flag_tmp);
        if(cmd_data_tmp[0] == pair_flag_tmp)
        {
            // 已经设置成功
            printf("is same\r\n");
            my_cmd_err_send(MY_CMD_OK, 0, 0);
        }else{
            pair_flag_tmp = cmd_data_tmp[0];
            syscfg_write(CFG_HC_PAIR_PIN_CODE, &pair_flag_tmp, 1);
            my_cmd_err_send(MY_CMD_OK, 0, 0);
                    
            os_time_dly(10);
            cpu_reset();
        }

        break;

    case GX_BT_SET_RFCOMUUID_CONN:
        extern u8 switch_ble_mac_hicar;
        printf("GX_BT_SET_RFCOMUUID_CONN %d\r\n", cmd_data_tmp[0]);
        printf("switch_ble_mac_hicar : %d\r\n", switch_ble_mac_hicar);
        reconnect_mgr.type = cmd_data_tmp[0];
        if(reconnect_mgr.type == CONNECTIONMODE_Hicar)
        {
            printf("hicar mode init\r\n");
            extern void  switch_to_hicar_mode(void);
            switch_to_hicar_mode();
            hicar_mode_init();
        }else{//(reconnect_mgr.type不为CONNECTIONMODE_Hicar
            extern void carplay_bluetooth_init();
            carplay_bluetooth_init();
            hfp_ignore_income_connect = 0;
        }
        if(switch_ble_mac_hicar == 1 && reconnect_mgr.type != CONNECTIONMODE_Hicar)
        {
            printf("switch_ble_mac_hicar\r\n");
            switch_to_carplay_mode();
        }
        my_cmd_err_send(MY_CMD_OK, 0, 0);
        break;

    case GX_SET_UART_UPDATE:
        my_cmd_err_send(MY_CMD_OK, 0, 0);
        uart_dev_test_del();
        gx_update_uart_init();

        printf("uart update init end\r\n");
        break;
#if 1
    case GX_SET_BLE_ADV_DATA:
        //设置ble广播数据
        printf("cmd af set ble adv data ....\r\n");
        // <adv/rsp>  <data>
        int custom_ble_adv_data_set(BLE_ADV_TYPE adv_type, u8 *data, u8 len);
        custom_ble_adv_data_set(cmd_data_tmp[0], cmd_data_tmp + 1, nfuart->cmd_len - 1);

        my_cmd_err_send(MY_CMD_OK, 0, 0);
        break;

    /*HID 部分 ------- ↓ */
    case GX_SET_HID_DESCRIPTOR:
    printf("cmd b0 set hid report map ....\r\n");
    // if (app_get_curr_task() != APP_BT_TASK)
    // {
    //     printf("Not in BT task, switching to BT...\r\n");
    //     // 主动请求切回蓝牙模式（根据你 SDK 的 API 调用，如 app_task_switch_to(APP_BT_TASK)）
    //     // app_task_switch_to(APP_BT_TASK); 

    //     // 如果支持缓存或延迟处理，可在此缓存 HID map，待切入 BT TASK 完毕后再执行 gx_user_set_hid_map
    //     // 或者给上位机回复忙/稍后再试
    //     my_cmd_err_send(MY_CMD_ERR, 0x06, 1);
    //     break;
    // }
    gx_user_set_hid_map(cmd_data_tmp, nfuart->cmd_len);
    break;

        // case GX_SET_HID_MESSAGE:
        //     printf("cmd b1 send hid data ....\r\n");
        //     // if(app_get_curr_task() != APP_BT_TASK)
        //     // {
        //     //     printf("TASK ERR\r\n");
        //     //     my_cmd_err_send(MY_CMD_ERR, 0x06, 1);
        //     //     break;
        //     // }
        //     user_hid_data_send(cmd_data_tmp[0], cmd_data_tmp+1, nfuart->cmd_len - 1);

        //     my_cmd_err_send(MY_CMD_OK, 0, 0);
        // break;

    case GX_SET_HID_DEVICE_NAME:
        printf("cmd b2 set hid ble name ....\r\n"); // 打印日志：收到设置 HID 蓝牙名称的指令
        u8 name_buf[32] = {0};                      // 定义 32 字节大小的缓冲区用于存放新名称，并初始化为 0

        /* 1. 校验命令数据长度 */
        if(nfuart->cmd_len == 0)
        {          
            printf("set name len is 0 \r\n");       // 长度为 0，打印错误日志
            my_cmd_err_send(MY_CMD_ERR, 0x05, 1);   // 发送错误响应状态（错误码 0x05）
            break;                                  // 跳出 switch 分支
        }

        /* 2. 拷贝名称数据并防范溢出 */
        // 注意：此处若 nfuart->cmd_len > 32 会导致缓冲区溢出，建议增加长度限定：
        // memcpy(name_buf, cmd_data_tmp, nfuart->cmd_len > 31 ? 31 : nfuart->cmd_len);
        memcpy(name_buf, cmd_data_tmp, nfuart->cmd_len); // 将接收到的指令数据拷贝到 name_buf 数组中

        /* 3. 校验指针（注：栈上静态数组 name_buf 不可能为 NULL，此处条件永远为假） */
        if(name_buf == NULL)
        {
            printf("set name_buf is null\r\n");
            my_cmd_err_send(MY_CMD_ERR, 0x05, 1);
            break;
        }

        /* 4. 打印当前名称与新名称的比对结果 */
        printf("memcmp(name_buf,ble_get_local_name() : %d\n", memcmp(name_buf, ble_get_local_name(), nfuart->cmd_len));

        /* 5. 检查设置的名称是否与当前本地蓝牙名称相同 */
        if (memcmp(name_buf, ble_get_local_name(), nfuart->cmd_len) == 0)
        {
            printf("name is same \r\n");            // 名称相同，无需重复设置
            // 注意：若这里直接 break，可能需要考虑是否要向上位机补发 MY_CMD_OK 响应
            break;
        }

        /* 6. 将新名称写入系统配置（非易失性存储 Flash/NVRAM） */
        syscfg_write(CFG_BLE_NAME, name_buf, nfuart->cmd_len);

        // 以下注释掉的代码为通过复位 CPU 使配置生效的方式
        // u8 hc_cpureset = 1;
        // syscfg_write(CFG_HQ_CPU_RESET_FLAG, &hc_cpureset, sizeof(u8));
        // os_time_dly(50);
        // cpu_reset();

        /* 7. 动态更新蓝牙广播名称/配置 */
        void gx_ble_name_set();                     // 声明设置蓝牙名称的函数原型
        gx_ble_name_set();                          // 调用函数使新蓝牙名称生效

        /* 8. 返回成功响应 */
        my_cmd_err_send(MY_CMD_OK, 0, 0);           // 向上位机发送指令执行成功的响应
        break;

    case GX_GET_HID_DEVICE_NAME:
        printf("cmd b3 get hid ble name ....\r\n");
        char *hc_ble_name;
        hc_ble_name = ble_get_local_name();
        my_cmd_data_send(GX_SEND_HID_DEVICE_NAME, hc_ble_name, strlen(hc_ble_name));
        my_cmd_err_send(MY_CMD_OK, 0, 0);

        break;
    case GX_SET_HID_RADIO_FREQ_START:
        printf("cmd b5 open hid ble power ....\r\n");
        ble_module_enable(1);

        hc_open_bt.ble_open = 1;
        syscfg_write(CFG_HC_BT_OPEN_STATSUS, &hc_open_bt, sizeof(HC_BT_OPEN));
        my_cmd_err_send(MY_CMD_OK, 0, 0);
        break;
    case GX_SET_HID_RADIO_FREQ_STOP:
        printf("cmd b6 close hid ble power ....\r\n");
        ble_module_enable(0);

        hc_open_bt.ble_open = 0;
        syscfg_write(CFG_HC_BT_OPEN_STATSUS, &hc_open_bt, sizeof(HC_BT_OPEN));
        my_cmd_err_send(MY_CMD_OK, 0, 0);
        break;
    case GX_GET_HID_DEVICE_STATUS:
        printf("cmd b7 get hid ble conn_status....\r\n");
        if (bt_ble_is_connected())
        {
            answer_data[0] = 0x01;
        }
        else
        {
            answer_data[0] = 0x00;
        }
        answer_len += 1;
        my_cmd_data_send(GX_SEND_HID_STATUS, answer_data, answer_len);
        break;
    case GX_GET_HID_FW_VERSION:
        printf("cmd b9 get hid fw version ....\r\n"); // 蓝牙固件版本

        break;
    case GX_SET_HID_FACTORY_SETTINGS:
        printf("cmd bb set ble factory reset....\r\n"); // 恢复出厂

        break;
    case GX_SET_HID_DELETE_DEVICE:
        printf("cmd bc set unpair ....\r\n"); // 取消配对
        ble_list_smp_pair_set(4);
        ble_app_disconnect();

        my_cmd_err_send(MY_CMD_OK, 0, 0);
        break;
    case GX_GET_HID_BATTERY_LEVEL:
        printf("cmd bd set hid report map ....\r\n");
        printf("get_vbat_percent: %d\n", get_vbat_percent());
        answer_data[0] = get_vbat_percent();
        answer_len += 1;
        my_cmd_data_send(GX_SEND_HID_BATTERY_LEVEL, answer_data, answer_len);

        my_cmd_err_send(MY_CMD_OK, 0, 0);
        break;

    case GX_SET_HID_DEVICE_DISCONNECT:
        printf("cmd bf set disconn hid ble....\r\n");
        ble_app_disconnect();

        my_cmd_err_send(MY_CMD_OK, 0, 0);
        break;
#endif
    case MY_GET_BT_NAME:
        printf(">>> --- MY_GET_BT_NAME --- <<<\n");
        char *my_bt_name = get_edr_name();
        printf("name :%s ---- \n", my_bt_name);
        my_cmd_data_send(MY_SEND_BT_NAME, my_bt_name, strlen(my_bt_name));
        my_cmd_err_send(MY_CMD_OK, 0, 0);

        break;

    case MY_GET_PHONE_NAME:
        printf(">>> --- MY_GET_PHONE_NAME --- <<<\n");
        char *my_phone_name = my_get_phone_name();

        printf("name :%s ---- \n", my_phone_name);

        my_cmd_data_send(MY_SEND_PHONE_NAME, my_phone_name, strlen(my_phone_name));
        my_cmd_err_send(MY_CMD_OK, 0, 0);

        break;

    case MY_GET_BT_MAC:
        printf(">>> --- MY_GET_BT_MAC --- <<<\n");
        memcpy(answer_addr, bt_get_mac_addr(), 6); // 拷贝地址
        printf("answer_addr :\n");
        put_buf(answer_addr, 6);
        my_cmd_data_send(MY_SEND_BT_MAC, answer_addr, 6);
        my_cmd_err_send(MY_CMD_OK, 0, 0);

        break;

    case MY_GET_PHONE_MAC:
        printf(">>> --- MY_GET_PHONE_MAC --- <<<\n");

        memcpy(answer_addr, my_get_phone_addr(), 6); // 拷贝地址
        printf("answer_addr :\n");
        put_buf(answer_addr, 6);

        my_cmd_data_send(MY_SEND_PHONE_MAC, answer_addr, 6);
        my_cmd_err_send(MY_CMD_OK, 0, 0);

        break;

    case MY_DISCONNECT_PHONE:
        printf(">>> --- MY_DISCONNECT_PHONE --- <<<\n");
        user_send_cmd_prepare(USER_CTRL_DISCONNECTION_HCI, 0, NULL); // test   //断开当前所有蓝牙连接
        

        my_cmd_err_send(MY_CMD_OK, 0, 0);

        break;
    case MY_RECONNECT_PHONE:
        printf(">>> --- MY_RECONNECT_PHONE --- <<<\n");
        u8 recon_mac[6] = {0};
        reconnect_flag = 1;
        syscfg_read(MY_RECONNECT_MAC, recon_mac, 6);
        printf("phone_addr : \n");
        put_buf(recon_mac, 6);
        user_send_cmd_prepare(USER_CTRL_START_CONNEC_VIA_ADDR, 6, recon_mac); // test

        my_cmd_err_send(MY_CMD_OK, 0, 0);
        break;

    case MY_DELETE_PHONE:
        printf(">>> --- MY_DELETE_PHONE --- <<<\n");
        user_send_cmd_prepare(USER_CTRL_DISCONNECTION_HCI, 0, NULL);
        user_send_cmd_prepare(USER_CTRL_DEL_ALL_REMOTE_INFO, 0, NULL);

        my_cmd_err_send(MY_CMD_OK, 0, 0);
        break;

    case MY_GET_BT_STATE:
        printf(">>> --- MY_GET_BT_STATE --- <<<\n");
        if (BT_CONNCTION_FLAG == 1)
        {
            my_cmd_err_send(MY_BT_STATE, 2, 1);
        }
        else
        {
            my_cmd_err_send(MY_BT_STATE, 3, 1);
        }
        break;

    case MY_ASK_VERSION:
        printf(">>> --- MY_ASK_VERSION --- <<<\n");
        // u8 my_version[30] = "CarPlay695-V1-20260416";
        // my_cmd_data_send(MY_ANSWER_VERSION, my_version, strlen(my_version));
        extern u8 version_data[32];
        my_cmd_data_send(MY_ANSWER_VERSION, version_data, strlen(version_data));

        my_cmd_err_send(MY_CMD_OK, 0, 0);
        break;

    case MY_SET_VOL:
        printf("MY_SET_VOL\r\n");
        if (cmd_data_tmp[0] < 0)
        {
            my_cmd_err_send(MY_CMD_ERR, 5, 1);
        }
        else
        {
            printf("vol_set %d", cmd_data_tmp[0]);
            app_audio_set_volume(APP_AUDIO_STATE_MUSIC, cmd_data_tmp[0], 1);
            user_send_cmd_prepare(USER_CTRL_HFP_CALL_SET_VOLUME, 1, &cmd_data_tmp[0]);

            my_cmd_err_send(MY_CMD_OK, 0, 0);
        }

        break;

    case MY_GET_VOL:
        printf("MY_GET_VOL\r\n");
        printf("2222222222222:%d\n", app_audio_get_volume(APP_AUDIO_STATE_MUSIC));
        answer_data[0] = app_audio_get_volume(APP_AUDIO_STATE_MUSIC);
        my_cmd_data_send(MY_SEND_VOL, answer_data, 1);
        break;

    case MY_CARPLAY_RECONN:

        printf("--MY_CARPLAY_RECONN--\r\n");
        bt_hci_event_connection_timeout(NULL);
        my_cmd_err_send(MY_CMD_OK, 0, 0);
        break;

    case GX_GET_BT_PBAP: // 获取电话本

        printf("--GX_GET_BT_PBAP--\r\n");
        if (app_get_curr_task() != APP_BT_TASK)
        {
            printf("TASK ERR\r\n");
            my_cmd_err_send(MY_CMD_ERR, 0x06, 1);
            break;
        }
        user_send_cmd_prepare(USER_CTRL_PBAP_READ_ALL, 0, NULL);
        phone_callback_flag = GX_GET_BT_PBAP;
        break;

    case GX_GET_BT_PBAP_PART: // 通话记录

        printf("--GX_GET_BT_PBAP_PART--\r\n");
        if (app_get_curr_task() != APP_BT_TASK)
        {
            printf("TASK ERR\r\n");
            my_cmd_err_send(MY_CMD_ERR, 0x06, 1);
            break;
        }
        user_send_cmd_prepare(USER_CTRL_PBAP_READ_PART, 0, NULL);
        phone_callback_flag = GX_GET_BT_PBAP_PART;
        break;

    case GX_GET_BT_CALL_STATUS: // 当前通话状态

        printf("--GX_GET_BT_CALL_STATUS--\r\n");
        if (app_get_curr_task() != APP_BT_TASK)
        {
            printf("TASK ERR\r\n");
            my_cmd_err_send(MY_CMD_ERR, 0x06, 1);
            break;
        }
        answer_data[0] = 0;
        switch (get_call_status())
        {
        case BT_CALL_INCOMING:
        case BT_CALL_OUTGOING:
        case BT_CALL_ACTIVE:
            /* code */
            answer_data[1] = get_call_status();
            break;

        case BT_CALL_HANGUP:
            /* code */
            answer_data[1] = 1;
            break;

        default:
            break;
        }
        answer_len += 2;
        my_cmd_data_send(GX_SEND_BT_CALL_STATUS, answer_data, answer_len);

        my_cmd_err_send(MY_CMD_OK, 0, 0);
        break;

    case GX_SET_BT_CALL_BACK:

        printf("--GX_SET_BT_CALL_BACK--\r\n");
        if (app_get_curr_task() != APP_BT_TASK)
        {
            printf("TASK ERR\r\n");
            my_cmd_err_send(MY_CMD_ERR, 0x06, 1);
            break;
        }

        printf("phone_len : %d", nfuart->cmd_len);
        put_buf(cmd_data_tmp, nfuart->cmd_len);
        user_send_cmd_prepare(USER_CTRL_DIAL_NUMBER, nfuart->cmd_len, cmd_data_tmp);

        my_cmd_err_send(MY_CMD_OK, 0, 0);
        break;

    case GX_SET_BT_DTMF_SEND:

        printf("--GX_SET_BT_DTMF_SEND--\r\n");
        if (app_get_curr_task() != APP_BT_TASK)
        {
            printf("TASK ERR\r\n");
            my_cmd_err_send(MY_CMD_ERR, 0x06, 1);
            break;
        }

        printf("phone_len : %d", nfuart->cmd_len);
        put_buf(cmd_data_tmp, nfuart->cmd_len);
        user_send_cmd_prepare(USER_CTRL_HFP_DTMF_TONES, nfuart->cmd_len, cmd_data_tmp);

        my_cmd_err_send(MY_CMD_OK, 0, 0);
        break;

    case GX_SET_BT_CALL_STATUS:

        printf("--GX_SET_BT_CALL_STATUS--\r\n");
        if (app_get_curr_task() != APP_BT_TASK)
        {
            printf("TASK ERR\r\n");
            my_cmd_err_send(MY_CMD_ERR, 0x06, 1);
            break;
        }

        if (cmd_data_tmp[0] == 1)
        {
            user_send_cmd_prepare(USER_CTRL_HFP_CALL_ANSWER, 0, NULL);
        }
        else if (cmd_data_tmp[0] == 2)
        {
            user_send_cmd_prepare(USER_CTRL_HFP_CALL_HANGUP, 0, NULL);
        }

        my_cmd_err_send(MY_CMD_OK, 0, 0);
        break;

    case GX_GET_BT_CALLING_PHONE_NUMBER:

        printf("--GX_GET_BT_CALLING_PHONE_NUMBER--\r\n");
        if (app_get_curr_task() != APP_BT_TASK)
        {
            printf("TASK ERR\r\n");
            my_cmd_err_send(MY_CMD_ERR, 0x06, 1);
            break;
        }

        my_cmd_data_send(GX_SEND_BT_CALLING_PHONE_NUMBER, gx_call_phonenumber + 1, gx_call_phonenumber[0]);
        my_cmd_err_send(MY_CMD_OK, 0, 0);
        break;

    case GX_SET_BT_MUSIC_TRACK_DETAILS:

        printf("--GX_SET_BT_MUSIC_TRACK_DETAILS--\r\n");
        if (app_get_curr_task() != APP_BT_TASK)
        {
            printf("TASK ERR\r\n");
            my_cmd_err_send(MY_CMD_ERR, 0x06, 1);
            break;
        }
        // 打开蓝牙歌词信息获取
        extern u8 uart_get_bt_music_info_flag;
        uart_get_bt_music_info_flag = cmd_data_tmp[0];

        my_cmd_err_send(MY_CMD_OK, 0, 0);
        break;

    case GX_GET_PHONE_TIME:
        printf("--GX_GET_PHONE_TIME--\r\n");
        if (app_get_curr_task() != APP_BT_TASK)
        {
            printf("TASK ERR\r\n");
            my_cmd_err_send(MY_CMD_ERR, 0x06, 1);
            break;
        }

        extern u8 time_get_active_reporting;
        time_get_active_reporting = 0;
        if (gx_get_conn_remote_type() == 1)
        {
            user_send_cmd_prepare(USER_CTRL_MAP_READ_TIME, 0, NULL);
        }
        else
        {
            user_send_cmd_prepare(USER_CTRL_HFP_GET_PHONE_DATE_TIME, 0, NULL);
        }

        // my_cmd_err_send(MY_CMD_OK, 0, 0);
        break;

    case GX_GET_A2DP_PLAYING_STATUS:
        printf("--GX_GET_A2DP_PLAYING_STATUS--\r\n");
        if (app_get_curr_task() != APP_BT_TASK)
        {
            printf("TASK ERR\r\n");
            my_cmd_err_send(MY_CMD_ERR, 0x06, 1);
            break;
        }
        answer_data[0] = 0;
        answer_data[1] = get_bt_connect_status();
        my_cmd_data_send(GX_SEND_A2DP_PLAYING_STATUS, answer_data, 2);
        // if(answer_data[1] == BT_STATUS_PLAYING_MUSIC)
        // {
        //     // music playing
        // }else
        // {
        //     // music display
        // }
        break;

    case GX_GET_BT_MUSIC_PLAYING_TIME:
        printf("--GX_GET_BT_MUSIC_PLAYING_TIME--\r\n");
        if (app_get_curr_task() != APP_BT_TASK)
        {
            printf("TASK ERR\r\n");
            my_cmd_err_send(MY_CMD_ERR, 0x06, 1);
            break;
        }
        user_send_cmd_prepare(USER_CTRL_AVCTP_OPID_GET_PLAY_TIME, 0, NULL);
        break;

    case GX_SET_BT_CALLING_SCO_STATUS:
        printf("--GX_SET_BT_CALLING_SCO_STATUS--\r\n");
        if (app_get_curr_task() != APP_BT_TASK)
        {
            printf("TASK ERR\r\n");
            printf("app_get_curr_task() : %d\r\n", app_get_curr_task());
            my_cmd_err_send(MY_CMD_ERR, 0x06, 1);
            break;
        }
        user_send_cmd_prepare(USER_CTRL_SCO_LINK, 0, NULL);
        my_cmd_err_send(MY_CMD_OK, 0, 0);
        break;

    case GX_SET_BT_HFP_ENABLE:
        printf("--GX_SET_BT_HFP_ENABLE--\r\n");
        if (cmd_data_tmp[0] == 0) {
            printf("Disabling HFP...\r\n");
            user_send_cmd_prepare(USER_CTRL_HFP_DISCONNECT, 0, NULL);
            hfp_ignore_income_connect = 1;
        } else {
            if (!(get_curr_channel_state() & HFP_CH)) {
                printf("Enabling HFP...\r\n");
                user_send_cmd_prepare(USER_CTRL_HFP_CMD_BEGIN, 0, NULL);
                hfp_ignore_income_connect = 0;
            }
        }
        my_cmd_err_send(MY_CMD_OK, 0, 0);
        break;

    case GX_GET_BT_CALLING_SCO_STATUS:
        printf("--GX_GET_BT_CALLING_SCO_STATUS--\r\n");
        if (app_get_curr_task() != APP_BT_TASK)
        {
            printf("TASK ERR\r\n");
            my_cmd_err_send(MY_CMD_ERR, 0x06, 1);
            break;
        }
        answer_data[0] = 1;
        answer_data[1] = gx_get_sco_conn_status();
        my_cmd_data_send(GX_SEND_BT_CALLING_SCO_STATUS, answer_data, 2);

        break;
    case GX_SET_BT_CONN_PINCODE:
        printf("--GX_SET_BT_CONN_PINCODE--\r\n");
        if (nfuart->cmd_len != 4)
        {
            printf("SET PIN CODE ERR len is not 4\r\n");
            my_cmd_err_send(MY_CMD_ERR, 0x05, 1);
            break;
        }
        void gx_set_pin_code(char *pin_code);
        gx_set_pin_code(cmd_data_tmp);

        
        // extern u8 phone_addr_temp1[6];
        // if(BT_CONNCTION_FLAG ==1){
        //     hicar_reconnect_start(phone_addr_temp1);
        //     my_cmd_err_send(MY_CMD_OK, 0, 0);
        // }else{
        //     printf("BT_CONNCTION_FLAG is not 1\r\n");
        //     my_cmd_err_send(MY_CMD_ERR, 0x05, 1);
        // }
        
        break;

    case GX_SET_HICAR_RECONNECT:
        printf("--GX_SET_HICAR_RECONNECT--\r\n");
        extern u8 phone_addr_temp1[6];
        if(BT_CONNCTION_FLAG ==1){
            hicar_reconnect_start(phone_addr_temp1);
            my_cmd_err_send(MY_CMD_OK, 0, 0);
        }else{
            printf("BT_CONNCTION_FLAG is not 1\r\n");
            my_cmd_err_send(MY_CMD_ERR, 0x05, 1);
        }
        
        break;

    case GX_GET_BT_CONN_PINCODE:
        printf("--GX_GET_BT_CONN_PINCODE--\r\n");
        const char *bt_get_pin_code();
        memcpy(answer_data, bt_get_pin_code(), 4);
        put_buf(answer_data, 4);

        my_cmd_data_send(GX_SEND_BT_CONN_PINCODE, answer_data, 4);

        break;

    case GX_SET_BT_MUSIC_STATUS:
        printf("--GX_SET_BT_MUSIC_STATUS--\r\n");
        if (app_get_curr_task() != APP_BT_TASK)
        {
            printf("TASK ERR\r\n");
            my_cmd_err_send(MY_CMD_ERR, 0x06, 1);
            break;
        }
        if (cmd_data_tmp[0] == 1)
        {
            // 上一曲
            bt_key_music_prev();
        }
        else if (cmd_data_tmp[0] == 2)
        {
            // 下一曲
            bt_key_music_next();
        }
        else if (cmd_data_tmp[0] == 3)
        {
            // 播放
            // check a2dp is active.....
            bt_key_music_pp();
        }
        else if (cmd_data_tmp[0] == 4)
        {
            // 暂停
            // check a2dp is active.....
            bt_key_music_pp();
        }
        else if (cmd_data_tmp[0] == 5)
        {
            // 音量增
            void bt_key_vol_up();
            bt_key_vol_up();
        }
        else if (cmd_data_tmp[0] == 6)
        {
            // 音量减
            void bt_key_vol_down();
            bt_key_vol_down();
        }
        else
        {
            my_cmd_err_send(MY_CMD_ERR, 0x05, 1);
        }

        my_cmd_err_send(MY_CMD_OK, 0, 0);
        break;
#if TCFG_APP_FM_EMITTER_EN
    case GX_SET_FMTX_ON_OFF:

        printf("--GX_SET_FMTX_ON_OFF--\r\n");
        printf("FM tx fre ... %d", fm_emitter_manage_get_fre());
        if(!cmd_data_tmp[0])
        {
            fm_emitter_manage_stop();
        }else
        {
            fm_emitter_manage_start();
        }
        my_cmd_err_send(MY_CMD_OK, 0, 0);
        break;

    case GX_SET_FMTX_POWER:

        printf("--GX_SET_FMTX_POWER--\r\n");

        fm_emitter_manage_set_power(cmd_data_tmp[0], fm_emitter_manage_get_fre());

        my_cmd_err_send(MY_CMD_OK, 0, 0);
        break;

    case GX_SET_BT_FMTX_FREQUENCY:

        printf("--GX_SET_BT_FMTX_FREQUENCY--\r\n");
        u16 fmtx_fre = ( (u16)cmd_data_tmp[0] ) | ( (u16)cmd_data_tmp[1] << 8 );

        fm_emitter_manage_set_fre(fmtx_fre);

        my_cmd_err_send(MY_CMD_OK, 0, 0);
        break;
#endif
    case GX_SET_FMRX_ON_OFF: // 播放 暂停
        if (app_get_curr_task() != APP_FM_TASK)
        {
            printf("TASK ERR\r\n");
            my_cmd_err_send(MY_CMD_ERR, 0x06, 1);
            break;
        }

        void gx_fm_volume_pp(u8 enable); //
        printf("--GX_SET_FMRX_ON_OFF--\r\n");
        if (app_get_curr_task() != APP_FM_TASK)
        {
            printf("TASK ERR\r\n");
            my_cmd_err_send(MY_CMD_ERR, 0x01, 1);
            break;
        }

        // gx_fm_volume_pp(cmd_data_tmp[0]);

        my_cmd_err_send(MY_CMD_OK, 0, 0);
        break;

    case GX_SET_FMRX_AUTO_SEARCH: //

        printf("--GX_SET_FMRX_AUTO_SEARCH--\r\n");
        if (app_get_curr_task() != APP_FM_TASK)
        {
            printf("TASK ERR\r\n");
            my_cmd_err_send(MY_CMD_ERR, 0x06, 1);
            break;
        }
        my_cmd_err_send(MY_CMD_OK, 0, 0);
        break;

    case GX_SET_FMRX_SEARCH_RANGE:

        printf("--GX_SET_FMRX_SEARCH_RANGE--\r\n");
        if (app_get_curr_task() != APP_FM_TASK)
        {
            printf("TASK ERR\r\n");
            my_cmd_err_send(MY_CMD_ERR, 0x06, 1);
            break;
        }

        // fm_scan_all();
        my_cmd_err_send(MY_CMD_OK, 0, 0);
        break;

    case GX_SET_BT_FMRX_DAC_OUTPUT_LR:
        printf("--GX_SET_BT_FMRX_DAC_OUTPUT_LR--\r\n");

        printf("app_get_curr_task:%d\n", app_get_curr_task());
        if (app_get_curr_task() != APP_FM_TASK)
        {
            my_cmd_err_send(MY_CMD_ERR, 0x06, 1);
            break;
        }
        if (cmd_data_tmp[0] == 0)
        {
            fm_inside_set_stereo(0); // fm 单声道
        }
        else if (cmd_data_tmp[0] == 1)
        {
            fm_inside_set_stereo(1); // fm 双声道
        }

        my_cmd_err_send(MY_CMD_OK, 0, 0);
        break;

    case GX_SET_BT_FMRX_Search_Power:
        printf("--GX_SET_BT_FMRX_Search_Power--\r\n");
        if (app_get_curr_task() != APP_FM_TASK)
        {
            printf("TASK ERR\r\n");
            my_cmd_err_send(MY_CMD_ERR, 0x06, 1);
            break;
        }
        my_cmd_err_send(MY_CMD_OK, 0, 0);
        break;

    case GX_SET_BT_FMRX_DAC_EQ_MODE:
        printf("--GX_SET_BT_DAC_EQ_MODE--\r\n");
        // if (app_get_curr_task() != APP_FM_TASK && app_get_curr_task() != APP_BT_TASK)
        // {
        //     printf("TASK ERR\r\n");
        //     my_cmd_err_send(MY_CMD_ERR, 0x06, 1);
        //     break;
        // }
        int eq_mode_set(EQ_MODE mode);
        if(cmd_data_tmp[0] == 6)
        {
            extern struct eq_seg_info eq_tab_custom[];
            for(int i = 0; i < 10; i++)
            {
                s8 eq_gain = (s8)cmd_data_tmp[i+1];

                if (eq_gain < -12 || eq_gain > 12) {
                    // 可以打印日志或直接限制
                    eq_gain = (eq_gain < -12) ? -12 : 12;
                }
                printf("set gain : i:[%d] gain:[%d]\r\n", i, eq_gain);
                eq_tab_custom[i].gain = eq_gain;
                // eq_tab_custom[i].gain = (s8)cmd_data_tmp[i+1];
            }
        }

        eq_mode_set(cmd_data_tmp[0]);

        my_cmd_err_send(MY_CMD_OK, 0, 0);
        break;

    default:
        // 不支持的命令
        printf("CMD unrecognized or CMD Untreated\r\n");
        my_cmd_err_send(MY_CMD_ERR, 0x02, 1);
        break;
    }

    if (nfuart->rx_buf)
    {
        free(nfuart->rx_buf);
        nfuart->rx_buf = NULL; // 避免悬空指针
    }

    mem_stats();
}

#if 1

/*****************************
 * 简化的队列帧解析器
 *****************************/

#define MAX_FRAME_SIZE 1536//1024
#define QUEUE_SIZE 2048

static u8 g_rx_queue[QUEUE_SIZE];
static u16 g_queue_head = 0;
static u16 g_queue_tail = 0;
static u16 g_queue_count = 0;

static u8 g_frame_buffer[MAX_FRAME_SIZE];
static u16 g_frame_len = 0;
static u16 g_frame_expected = 0;
static bool g_in_frame = false;

// 初始化
void uart_queue_init(void)
{
    g_queue_head = 0;
    g_queue_tail = 0;
    g_queue_count = 0;
    g_frame_len = 0;
    g_frame_expected = 0;
    g_in_frame = false;
    printf("UART queue initialized\n");
}

// 队列操作
static void queue_push(u8 data)
{
    if (g_queue_count >= QUEUE_SIZE)
    {
        // 队列满，丢弃最早的数据
        g_queue_head = (g_queue_head + 1) % QUEUE_SIZE;
        g_queue_count--;
    }

    g_rx_queue[g_queue_tail] = data;
    g_queue_tail = (g_queue_tail + 1) % QUEUE_SIZE;
    g_queue_count++;
}

static bool queue_pop(u8 *data)
{
    if (g_queue_count == 0)
    {
        return false;
    }

    *data = g_rx_queue[g_queue_head];
    g_queue_head = (g_queue_head + 1) % QUEUE_SIZE;
    g_queue_count--;

    return true;
}

static bool queue_peek(u16 offset, u8 *data)
{
    if (offset >= g_queue_count)
    {
        return false;
    }

    u16 index = (g_queue_head + offset) % QUEUE_SIZE;
    *data = g_rx_queue[index];
    return true;
}

static u16 queue_size(void)
{
    return g_queue_count;
}

static void queue_remove(u16 count)
{
    if (count > g_queue_count)
    {
        count = g_queue_count;
    }

    g_queue_head = (g_queue_head + count) % QUEUE_SIZE;
    g_queue_count -= count;
}

// 计算校验和
static u16 calculate_checksum(u8 *data, u16 length)
{
    u16 sum = 0;
    for (u16 i = 0; i < length - 2; i++)
    {
        sum += data[i];
    }
    return sum;
}

// 验证帧
static bool validate_frame(u8 *frame, u16 length)
{
    if (length < 8)
        return false;
    if (frame[0] != 0xAC || frame[1] != 0x69)
        return false;

    u16 declared_length = frame[2] | (frame[3] << 8);
    if (declared_length != length)
        return false;

    u16 calculated_sum = calculate_checksum(frame, length);
    u16 received_sum = frame[length - 2] | (frame[length - 1] << 8);

    return calculated_sum == received_sum;
}

// 处理帧
static void process_frame(u8 *frame, u16 length)
{
    printf("Processing frame, length: %d\n", length);
    put_buf(frame, length);

    u8 cmd = frame[5], ret = 0;
    u16 cmd_len = frame[6] | (frame[7] << 8);
    printf("Command: 0x%02X, Cmd length: %d\n", cmd, cmd_len);

    if (cmd_len != length - 10)
    {
        printf("cmd len or framelen err");
        return;
    }

    ret = gx_uart_ble_hid_handle(cmd, cmd_len, frame, length);
    if (!ret)
    {
        return_uart_notify(cmd, cmd_len, frame, length);
    }
}

// 从队列中查找和处理帧
static void process_queue(void)
{
    // 简单算法：每次只处理队列头部的数据

    while (1)
    {
        u16 qsize = queue_size();

        // 如果没有足够的数据，退出
        if (qsize < 8)
        {
            break;
        }

        // 查看队列头部的数据
        u8 head_data[8];
        bool have_head = true;
        for (int i = 0; i < 8; i++)
        {
            if (!queue_peek(i, &head_data[i]))
            {
                have_head = false;
                break;
            }
        }

        if (!have_head)
        {
            break;
        }

        // 检查是否是有效的帧头
        if (head_data[0] == 0xAC && head_data[1] == 0x69)
        {
            // 读取长度字段
            u16 frame_len = head_data[2] | (head_data[3] << 8);

            // 验证长度
            if (frame_len < 8 || frame_len > MAX_FRAME_SIZE)
            {
                // 长度无效，丢弃第一个字节，继续
                printf("Invalid frame length %d, discarding 1 byte\n", frame_len);
                queue_remove(1);
                continue;
            }

            // 检查是否有完整帧
            if (qsize >= frame_len)
            {
                // 提取完整帧
                static u8 frame[MAX_FRAME_SIZE];
                for (u16 i = 0; i < frame_len; i++)
                {
                    queue_peek(i, &frame[i]);
                }

                // 验证帧
                if (validate_frame(frame, frame_len))
                {
                    // 处理帧
                    process_frame(frame, frame_len);

                    // 从队列中移除帧数据
                    queue_remove(frame_len);

                    // 继续处理队列中剩余的数据
                    continue;
                }
                else
                {
                    // 帧验证失败，丢弃第一个字节
                    printf("Frame validation failed, discarding 1 byte\n");
                    queue_remove(1);
                    continue;
                }
            }
            else
            {
                // 帧不完整，等待更多数据
                break;
            }
        }
        else
        {
            // 不是帧头，丢弃第一个字节
            queue_remove(1);
        }
    }
}

// 接收数据入口
void uart_data_received(u8 *data, u16 data_len)
{
    // 将数据推入队列
    for (u16 i = 0; i < data_len; i++)
    {
        queue_push(data[i]);
    }

    // 处理队列中的数据
    process_queue();
}

// 兼容原有的gx_uart_get_data接口
void gx_uart_get_data(u8 *data, u16 data_len)
{
    uart_data_received(data, data_len);
}

// 重置解析器
void uart_parser_reset(void)
{
    uart_queue_init();
    printf("UART parser reset\n");
}

// 获取队列状态（用于调试）
void uart_queue_status(void)
{
    printf("Queue: size=%d\n", queue_size());
}

#endif

void my_cmd_data_send(u8 cmd, u8 *data, u16 data_len)
{
    u8 buffer_return[1024] = {0};
    
    int pkg_len_return = gx_ble_hid_pack_frame(cmd, data, data_len, buffer_return); // 组包

    put_buf(buffer_return, pkg_len_return);

    gx_uart_send_data(buffer_return, pkg_len_return);
}


void gx_update_uart_init()
{
    #include "uart_update.h"
    // uart_update_cfg  update_cfg = {
    //     .rx = IO_PORTA_06,
    //     .tx = IO_PORTA_05,
    //     .output_channel = CH1_UT1_TX,
    //     .input_channel = INPUT_CH0
    // };
    // uart_update_init(&update_cfg);

    // uart_dev_close(uart_bus);
    //os_task_del("uart_u_task");
    // os_time_dly(10);
    uart_update_cfg  update_cfg = {
        .rx = IO_PORTA_06,
        .tx = IO_PORTA_05,
        .output_channel = CH1_UT1_TX,
        .input_channel = INPUT_CH0
    };
    uart_update_init(&update_cfg);
}
#endif
