  /*
 ****************************************************************
 *							AUDIO_IIS_TX_DEMO
 * File  : audio_iis_tx_demo.c
 * By    :
 * Notes : iis¨o?3?demo : adc->iis_tx
 * Usage : ?¨2app_main()?D|ì??¨??????¨|2a¨o?????ê?¨|
 *
 ****************************************************************
 */

#include "media/includes.h"
#include "app_config.h"
#include "audio_config.h"
#include "asm/includes.h"
#include "audio_link.h"
#include "media/audio_splicing.h"
#include "aec_user.h"

#include "effect_parm.h"
#include "clock_cfg.h"
#include "media/audio_drc.h"
#include "media/effects_adj.h"
#include "audio_effect/audio_eff_default_parm.h"
#if TCFG_IIS_ENABLE

extern struct audio_adc_hdl adc_hdl;
extern ALINK_PARM alink0_param;

#define ADC_TEST_CH_NUM        	1	/*??ì3?|ì??á???¨?2¨|?¨′¨a?§|ì¨¤(max = 2)*/
#define ADC_TEST_BUF_NUM        2	/*2¨|?¨′buf¨oy*/
//#define ADC_TEST_IRQ_POINTS     256	/*2¨|?¨′?D??|ì?¨oy?ê?D¨¨¨°a?¨2IIS?D??|ì?¨oy?¨¤¨a?*/
#define ADC_TEST_IRQ_POINTS     256	/*2¨|?¨′?D??|ì?¨oy?ê?D¨¨¨°a?¨2IIS?D??|ì?¨oy?¨¤¨a?*/
#define ADC_TEST_BUFS_SIZE      (ADC_TEST_CH_NUM * ADC_TEST_BUF_NUM * ADC_TEST_IRQ_POINTS)

//#define ADC_2_IIS_TX_CBUF_LEN   1024 * 2
#define ADC_2_IIS_TX_CBUF_LEN   1024 * 6
struct adc_2_iis_tx_hdl {
    struct audio_adc_output_hdl adc_output;
    struct adc_mic_ch mic_ch;
    s16 adc_buf[ADC_TEST_BUFS_SIZE];
    s16 dual_data[640 * 6];
    cbuffer_t cbuf;
    u8 *buf;
};
static struct adc_2_iis_tx_hdl *adc_2_iis_tx = NULL;



#include "Resample_api.h"
static RS_STUCT_API *sw_src_api = NULL;
static u8 *sw_src_buf = NULL;
static s16 sw_src_output[320 * 6 + 64];

static u32 sw_src_need_buf;
int my_sw_src_init(u32 in_sr, u32 out_sr)
{
    sw_src_api = get_rs16_context();        //
    g_printf("sw_src_api:0x%x\n", sw_src_api);
    ASSERT(sw_src_api);
    sw_src_need_buf = sw_src_api->need_buf();
    g_printf("sw_src_buf:%d\n", sw_src_need_buf);
    sw_src_buf = zalloc(sw_src_need_buf);
    ASSERT(sw_src_buf);
    RS_PARA_STRUCT rs_para_obj;
    rs_para_obj.nch = 1;//¨a?§|ì¨¤¨oy

    rs_para_obj.new_insample = in_sr;
    rs_para_obj.new_outsample = out_sr; //¨|¨¨??2¨|?¨′?¨o
    printf("sw src,in = %d,out = %d\n", rs_para_obj.new_insample, rs_para_obj.new_outsample);
    sw_src_api->open(sw_src_buf, &rs_para_obj);
    return 0;
}
extern int mic_to_iis(s16 *data, int len);
int my_newsrI = 16000;
int my_iis_cbuf = 0;
extern u32 my_get_iis_cbuf();
int my_usb_mic_aec_output(s16 *data, u16 len)
{
    int outlen = len;
    s16 *outdat = data;

    // putchar('m');

    if (sw_src_api && sw_src_buf) {

        my_iis_cbuf = my_get_iis_cbuf();
        // wdt_clear();
        // printf(">>> : %d ,%d, %d\n",my_newsrI, cbuf_get_data_len(&adc_2_iis_tx->cbuf), my_iis_cbuf);
        outlen = sw_src_api->run(sw_src_buf, data, len >> 1, sw_src_output);
        ASSERT(outlen <= (sizeof(sw_src_output) >> 1));     //????
        outlen = outlen << 1;
        outdat = sw_src_output;
    }

    u16 wlen = mic_to_iis(outdat, outlen);
    return len;
}

//adc?D??
static void adc_output_handle(void *priv, s16 *data, int len)
{
    audio_aec_inbuf(data, len);
}

u32 my_get_iis_cbuf()
{
    u32 my_iis_cbuf_len = 0;
    my_iis_cbuf_len = cbuf_get_data_len(&adc_2_iis_tx->cbuf);
    return my_iis_cbuf_len;
}
int mic_to_iis(s16 *data, int len)
{
    // my_drc_run(data, len);
    pcm_single_to_dual(adc_2_iis_tx->dual_data, data, len);
    len <<= 1;
    int wlen = cbuf_write(&adc_2_iis_tx->cbuf, adc_2_iis_tx->dual_data, len);
    if (wlen != len) {
        // putchar('a');
    }

    // mic_drc_open_test();


    return len;
}

extern u8 open_mic_flag;
//iis?D??
static void iis_output_handle(u8 ch, s16 *buf, u16 len)
{
    int rlen = cbuf_read(&adc_2_iis_tx->cbuf, buf, len);

    // static u8 cnt_tmp = 0;
    // if(cnt_tmp > 100)
    // {
    //     printf("len %d \r\n", len);
    //     printf("rlen %d \r\n", rlen);
    //     cnt_tmp = 0;
    // }
    // cnt_tmp++;

    if (rlen != len) {
        // putchar('b');
    }

}

int adc_2_iis_tx_demo_open(u16 sr)
{
    if (adc_2_iis_tx) {
        return -1;
    }
    adc_2_iis_tx = zalloc(sizeof(struct adc_2_iis_tx_hdl));
    if (!adc_2_iis_tx) {
        printf("adc_2_iis_tx zalloc err\n");
        return -1;
    }
    //buf alloc
    adc_2_iis_tx->buf = zalloc(ADC_2_IIS_TX_CBUF_LEN);
    cbuf_init(&adc_2_iis_tx->cbuf, adc_2_iis_tx->buf, ADC_2_IIS_TX_CBUF_LEN);

    //adc
    // void my_drc_creat();
    // my_drc_creat();

    int ret = 0;
    ret = audio_adc_mic_open(&adc_2_iis_tx->mic_ch, AUDIO_ADC_MIC_CH, &adc_hdl);      //??¨°?amic2¨|?¨′¨a?§|ì¨¤
    ret = audio_adc_mic_set_gain(&adc_2_iis_tx->mic_ch, 3);                           //10 MIC_GAINDT??|ì?     //¨|¨¨??mic??¨°?
    audio_mic_0dB_en(1);                                                    //
    ret = audio_adc_mic_set_sample_rate(&adc_2_iis_tx->mic_ch, 16000);            //mic 3?¨o???￥ ¨|¨¨??mic2¨|?¨′?¨o
    ret = audio_adc_mic_set_buffs(&adc_2_iis_tx->mic_ch, adc_2_iis_tx->adc_buf, ADC_TEST_IRQ_POINTS * 2, ADC_TEST_BUF_NUM);   //¨|¨¨??2¨|?¨′bufo¨a3?è?¨¨

    adc_2_iis_tx->adc_output.handler = adc_output_handle;
    audio_adc_add_output_handler(&adc_hdl, &adc_2_iis_tx->adc_output);      //?á?é2¨￠ adc2¨|?¨′¨o?3???|ì??o?￥¨oy
    extern void aec_test();
    aec_test();     //¨?¨a?¨?AEC

    //iis
    alink_init(&alink0_param);      //iis 3?¨o???￥????
    alink_channel_init(ALINK1_PORTA, TCFG_IIS_OUTPUT_DATAPORT_SEL, ALINK_DIR_TX, iis_output_handle);    //iis¨o?3?¨a?§|ì¨¤3?¨o???￥

    return 0;
}

int adc_2_iis_tx_demo_start()
{
    audio_adc_mic_start(&adc_2_iis_tx->mic_ch); //????￥mic2¨|?¨′
    // printf("%d", adc_2_iis_tx->mic_ch);         //mic¨a?§|ì¨¤
    alink_start(ALINK1_PORTA);
}

int adc_2_iis_tx_demo_close(void)
{
    if (!adc_2_iis_tx) {
        return -1;  // 未初始化或已经关闭
    }

    // 1. 先移除ADC回调函数
    if (&adc_2_iis_tx->adc_output) {
        audio_adc_del_output_handler(&adc_hdl, &adc_2_iis_tx->adc_output);
    }

    // 2. 关闭ADC MIC通道
    if (&adc_2_iis_tx->mic_ch) {
        audio_adc_mic_close(&adc_2_iis_tx->mic_ch);
    }

    // 3. 关闭IIS输出通道（假设有对应的关闭函数）
    // 如果系统有alink_channel_close函数：
    alink_uninit(&alink0_param);
    // alink_channel_close(ALINK1_PORTA, TCFG_IIS_OUTPUT_DATAPORT_SEL, ALINK_DIR_TX);

    // 或者如果只有停止函数：
    // alink_stop(ALINK1_PORTA, ALINK_DIR_TX);

    // 或者如果有对应的IIS关闭函数：
    // iis_output_close();

    // 4. 如果之前有开启AEC，可能需要关闭
    audio_aec_close();

    if (sw_src_buf) {
        free(sw_src_buf);
        sw_src_buf = NULL;
        sw_src_api = NULL;
    }

    // 5. 如果开启了DRC，需要关闭

    // 6. 释放缓冲区内存
    if (adc_2_iis_tx->buf) {
        free(adc_2_iis_tx->buf);
        adc_2_iis_tx->buf = NULL;
    }

    // 7. 释放cbuf相关资源（如果有清理函数的话）
    cbuf_clear(&adc_2_iis_tx->cbuf);

    // 8. 释放结构体本身
    free(adc_2_iis_tx);
    adc_2_iis_tx = NULL;

    printf("adc_2_iis_tx closed\n");
    return 0;
}
#endif

extern int my_sw_src_init(u32 in_sr, u32 out_sr);
extern int mic_to_iis(s16 *data, int len);
extern int my_usb_mic_aec_output(s16 *data, u16 len);
static int my_aec_output_hdl(s16 *data, u16 len)
{
    my_usb_mic_aec_output(data, len);
    return len;
}

void aec_test()
{
    // audio_aec_open(16000, 0xff, my_aec_output_hdl); //AEC_MODE_REDUCE      //??¨°?aAEC??ê?¨|
    audio_aec_open(16000, -1, my_aec_output_hdl); //AEC_MODE_REDUCE      //??¨°?aAEC??ê?¨|
    my_sw_src_init(16000, 48000);
}

u8 my_mic_gain = 0;
void my_set_mic_gain(u8 gain)
{
    my_mic_gain = gain;
    audio_adc_mic_set_gain(&adc_2_iis_tx->mic_ch, gain);                           //10 MIC_GAINDT??|ì?     //¨|¨¨??mac??¨°?

}


// void adc_2_iis_tx_demo_open_test(u16 sr, u8 gain)
// {
//     if (adc_2_iis_tx) {
//         return -1;
//     }
//     adc_2_iis_tx = zalloc(sizeof(struct adc_2_iis_tx_hdl));
//     if (!adc_2_iis_tx) {
//         printf("adc_2_iis_tx zalloc err\n");
//         return -1;
//     }
//     //buf alloc
//     adc_2_iis_tx->buf = zalloc(ADC_2_IIS_TX_CBUF_LEN);
//     cbuf_init(&adc_2_iis_tx->cbuf, adc_2_iis_tx->buf, ADC_2_IIS_TX_CBUF_LEN);

//     //adc
//     // void my_drc_creat();
//     // my_drc_creat();

//     audio_adc_mic_open(&adc_2_iis_tx->mic_ch, AUDIO_ADC_MIC_CH, &adc_hdl);      //??¨°?amic2¨|?¨′¨a?§|ì¨¤
//     audio_adc_mic_set_gain(&adc_2_iis_tx->mic_ch, gain);                           //10 MIC_GAINDT??|ì?     //¨|¨¨??mac??¨°?
//     audio_mic_0dB_en(0);                                                    //
//     audio_adc_mic_set_sample_rate(&adc_2_iis_tx->mic_ch, 16000);            //mic 3?¨o???￥ ¨|¨¨??mic2¨|?¨′?¨o
//     audio_adc_mic_set_buffs(&adc_2_iis_tx->mic_ch, adc_2_iis_tx->adc_buf, ADC_TEST_IRQ_POINTS * 2, ADC_TEST_BUF_NUM);   //¨|¨¨??2¨|?¨′bufo¨a3?è?¨¨

//     adc_2_iis_tx->adc_output.handler = adc_output_handle;
//     audio_adc_add_output_handler(&adc_hdl, &adc_2_iis_tx->adc_output);      //?á?é2¨￠ adc2¨|?¨′¨o?3???|ì??o?￥¨oy
//     extern void aec_test();
//     aec_test();     //¨?¨a?¨?AEC

//     //iis
//     alink_init(&alink0_param);      //iis 3?¨o???￥????
//     alink_channel_init(ALINK1_PORTA, TCFG_IIS_OUTPUT_DATAPORT_SEL, ALINK_DIR_TX, iis_output_handle);    //iis¨o?3?¨a?§|ì¨¤3?¨o???￥

//     return 0;
// }













