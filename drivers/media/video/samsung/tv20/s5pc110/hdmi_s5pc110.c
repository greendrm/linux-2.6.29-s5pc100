/* linux/drivers/media/video/samsung/tv20/s5pc100/hdmi_s5pc100.c
 *
 * hdmi raw ftn  file for Samsung TVOut driver
 *
 * Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>

#include <asm/io.h>

#include "tv_out_s5pc110.h"

#include "regs/regs-hdmi.h"

#include "plat/regs-clock.h"

#include "hdmi_param.h"

#ifdef COFIG_TVOUT_RAW_DBG
#define S5P_HDMI_DEBUG 1
#endif

#ifdef S5P_HDMI_DEBUG
#define HDMIPRINTK(fmt, args...) \
	printk("\t\t[HDMI] %s: " fmt, __FUNCTION__ , ## args)
#else
#define HDMIPRINTK(fmt, args...)
#endif

static struct resource	*hdmi_mem;
void __iomem		*hdmi_base;

extern bool __s5p_start_hdcp(void);
extern void __s5p_stop_hdcp(void);
extern void __s5p_init_hdcp(bool hpd_status, struct i2c_client *ddc_port);

s32 hdmi_set_tg(s5p_hdmi_v_fmt mode)
{
	u16 temp_reg;
	u8 tc_cmd;
	
	temp_reg = hdmi_tg_param[mode].h_fsz;     
	writeb((u8)(temp_reg&0xff) , hdmi_base + S5P_TG_H_FSZ_L );
	writeb((u8)(temp_reg >> 8) , hdmi_base + S5P_TG_H_FSZ_H );

	/* set Horizontal Active Start Position */
	temp_reg = hdmi_tg_param[mode].hact_st ;     
	writeb((u8)(temp_reg&0xff) , hdmi_base + S5P_TG_HACT_ST_L );
	writeb((u8)(temp_reg >> 8) , hdmi_base + S5P_TG_HACT_ST_H );

	/* set Horizontal Active Size */    
	temp_reg = hdmi_tg_param[mode].hact_sz ;     
	writeb((u8)(temp_reg&0xff) , hdmi_base + S5P_TG_HACT_SZ_L );
	writeb((u8)(temp_reg >> 8) , hdmi_base + S5P_TG_HACT_SZ_H );

	/* set Vertical Full Size */    
	temp_reg = hdmi_tg_param[mode].v_fsz ;     
	writeb((u8)(temp_reg&0xff) , hdmi_base + S5P_TG_V_FSZ_L );
	writeb((u8)(temp_reg >> 8) , hdmi_base + S5P_TG_V_FSZ_H );

	/* set VSYNC Position */    
	temp_reg = hdmi_tg_param[mode].vsync ;     
	writeb((u8)(temp_reg&0xff) , hdmi_base + S5P_TG_VSYNC_L );
	writeb((u8)(temp_reg >> 8) , hdmi_base + S5P_TG_VSYNC_H );

	/* set Bottom Field VSYNC Position */    
	temp_reg = hdmi_tg_param[mode].vsync2;     
	writeb((u8)(temp_reg&0xff) , hdmi_base + S5P_TG_VSYNC2_L );
	writeb((u8)(temp_reg >> 8) , hdmi_base + S5P_TG_VSYNC2_H );

	/* set Vertical Active Start Position */    
	temp_reg = hdmi_tg_param[mode].vact_st ;     
	writeb((u8)(temp_reg&0xff) , hdmi_base + S5P_TG_VACT_ST_L );
	writeb((u8)(temp_reg >> 8) , hdmi_base + S5P_TG_VACT_ST_H );

	/* set Vertical Active Size */    
	temp_reg = hdmi_tg_param[mode].vact_sz ;     
	writeb((u8)(temp_reg&0xff) , hdmi_base + S5P_TG_VACT_SZ_L );
	writeb((u8)(temp_reg >> 8) , hdmi_base + S5P_TG_VACT_SZ_H );

	/* set Field Change Position */    
	temp_reg = hdmi_tg_param[mode].field_chg ;     
	writeb((u8)(temp_reg&0xff) , hdmi_base + S5P_TG_FIELD_CHG_L );
	writeb((u8)(temp_reg >> 8) , hdmi_base + S5P_TG_FIELD_CHG_H );

	/* set Bottom Field Vertical Active Start Position */    
	temp_reg = hdmi_tg_param[mode].vact_st2;     
	writeb((u8)(temp_reg&0xff) , hdmi_base + S5P_TG_VACT_ST2_L );
	writeb((u8)(temp_reg >> 8) , hdmi_base + S5P_TG_VACT_ST2_H );

	/* set VSYNC Position for HDMI */    
	temp_reg = hdmi_tg_param[mode].vsync_top_hdmi;     
	writeb((u8)(temp_reg&0xff) , hdmi_base + S5P_TG_VSYNC_TOP_HDMI_L );
	writeb((u8)(temp_reg >> 8) , hdmi_base + S5P_TG_VSYNC_TOP_HDMI_H );

	/* set Bottom Field VSYNC Position */    
	temp_reg = hdmi_tg_param[mode].vsync_bot_hdmi;     
	writeb((u8)(temp_reg&0xff) , hdmi_base + S5P_TG_VSYNC_BOT_HDMI_L );
	writeb((u8)(temp_reg >> 8) , hdmi_base + S5P_TG_VSYNC_BOT_HDMI_H );

	/* set Top Field Change Position for HDMI */     
	temp_reg = hdmi_tg_param[mode].field_top_hdmi ;     
	writeb((u8)(temp_reg&0xff) , hdmi_base + S5P_TG_FIELD_TOP_HDMI_L );
	writeb((u8)(temp_reg >> 8) , hdmi_base + S5P_TG_FIELD_TOP_HDMI_H );

	/* set Bottom Field Change Position for HDMI */    
	temp_reg = hdmi_tg_param[mode].field_bot_hdmi ;     
	writeb((u8)(temp_reg&0xff) , hdmi_base + S5P_TG_FIELD_BOT_HDMI_L );
	writeb((u8)(temp_reg >> 8) , hdmi_base + S5P_TG_FIELD_BOT_HDMI_H );

	tc_cmd = readb(hdmi_base + S5P_TG_CMD);   

	if(video_params[mode].interlaced == 1)
		/* Field Mode enable(interlace mode) */
		tc_cmd |= (1<<1);			
	else
		/* Field Mode disable */
		tc_cmd &= ~(1<<1);			

	/* TG global enable */
//	tc_cmd |= (1<<0);
//	writeb(tc_cmd, hdmi_base + S5P_TG_CMD);
		
	return 0;
}

s32 hdmi_set_video_mode(s5p_hdmi_v_fmt mode, s5p_hdmi_color_depth cd, s5p_tv_hdmi_pxl_aspect pxl_ratio)
{
	u8  temp_reg8;
	u16 temp_reg16;
	u32 temp_reg32, temp_sync2, temp_sync3;

	/* check if HDMI code support that mode */
	if (mode > (sizeof(video_params)/sizeof(struct hdmi_v_params)) || (s32)mode < 0 )
	{
		HDMIPRINTK("This video mode is not Supported\n");
		return -EINVAL;
	}
	
	hdmi_set_tg(mode);

	/* set HBlank */
	temp_reg16 = video_params[mode].h_blank;
	writeb((u8)(temp_reg16&0xff), hdmi_base + S5P_H_BLANK_0 );
	writeb((u8)(temp_reg16 >> 8), hdmi_base + S5P_H_BLANK_1 );

	/* set VBlank */
	temp_reg32 = video_params[mode].v_blank;
	writeb((u8)(temp_reg32&0xff), hdmi_base + S5P_V_BLANK_0 );
	writeb((u8)(temp_reg32 >> 8), hdmi_base + S5P_V_BLANK_1 );
	writeb((u8)(temp_reg32 >> 16), hdmi_base + S5P_V_BLANK_2 );

	/* set HVLine */
	temp_reg32 = video_params[mode].hvline;
	writeb((u8)(temp_reg32&0xff), hdmi_base + S5P_H_V_LINE_0 );
	writeb((u8)(temp_reg32 >> 8), hdmi_base + S5P_H_V_LINE_1 );
	writeb((u8)(temp_reg32 >> 16),hdmi_base + S5P_H_V_LINE_2 );
	
	/* set VSYNC polarity */
	writeb(video_params[mode].polarity, hdmi_base + S5P_SYNC_MODE);

	/* set HSyncGen */
	temp_reg32 = video_params[mode].h_sync_gen;
	writeb((u8)(temp_reg32&0xff), hdmi_base + S5P_H_SYNC_GEN_0);
	writeb((u8)(temp_reg32 >> 8), hdmi_base + S5P_H_SYNC_GEN_1);
	writeb((u8)(temp_reg32 >> 16), hdmi_base + S5P_H_SYNC_GEN_2);

	/* set VSyncGen1 */
	temp_reg32 = video_params[mode].v_sync_gen;
	writeb((u8)(temp_reg32&0xff), hdmi_base + S5P_V_SYNC_GEN_1_0);
	writeb((u8)(temp_reg32 >> 8), hdmi_base + S5P_V_SYNC_GEN_1_1);
	writeb((u8)(temp_reg32 >> 16), hdmi_base + S5P_V_SYNC_GEN_1_2);

	if (video_params[mode].interlaced)
	{
		/* set up v_blank_f, v_sync_gen2, v_sync_gen3 */
		temp_reg32 = video_params[mode].v_blank_f;
		temp_sync2 = video_params[mode].v_sync_gen2;
		temp_sync3 = video_params[mode].v_sync_gen3;

		writeb((u8)(temp_reg32 & 0xff), hdmi_base + S5P_V_BLANK_F_0);
		writeb((u8)(temp_reg32 >> 8), hdmi_base + S5P_V_BLANK_F_1);
		writeb((u8)(temp_reg32 >> 16), hdmi_base + S5P_V_BLANK_F_2);

		writeb((u8)(temp_sync2 & 0xff), hdmi_base + S5P_V_SYNC_GEN_2_0);
		writeb((u8)(temp_sync2 >> 8), hdmi_base + S5P_V_SYNC_GEN_2_1);
		writeb((u8)(temp_sync2 >> 16), hdmi_base + S5P_V_SYNC_GEN_2_2);

		writeb((u8)(temp_sync3 & 0xff), hdmi_base + S5P_V_SYNC_GEN_3_0);
		writeb((u8)(temp_sync3 >> 8), hdmi_base + S5P_V_SYNC_GEN_3_1);
		writeb((u8)(temp_sync3 >> 16), hdmi_base + S5P_V_SYNC_GEN_3_2);
	}
	else
	{
		/* progressive mode */
		writeb(0x00, hdmi_base + S5P_V_BLANK_F_0);
		writeb(0x00, hdmi_base + S5P_V_BLANK_F_1);
		writeb(0x00, hdmi_base + S5P_V_BLANK_F_2);

		writeb(0x01, hdmi_base + S5P_V_SYNC_GEN_2_0);
		writeb(0x10, hdmi_base + S5P_V_SYNC_GEN_2_1);
		writeb(0x00, hdmi_base + S5P_V_SYNC_GEN_2_2);

		writeb(0x01, hdmi_base + S5P_V_SYNC_GEN_3_0);
		writeb(0x10, hdmi_base + S5P_V_SYNC_GEN_3_1);
		writeb(0x00, hdmi_base + S5P_V_SYNC_GEN_3_2);
	}
	
	/* set interlaced mode */
	writeb(video_params[mode].interlaced, hdmi_base + S5P_INT_PRO_MODE);

	/* pixel repetition */
	temp_reg8 = readb(hdmi_base + S5P_HDMI_CON_1);

	/* clear */
	temp_reg8 &= ~ HDMI_CON_PXL_REP_RATIO_MASK;

	if (video_params[mode].repetition)
	{
		/* set pixel repetition */
		temp_reg8 |= HDMI_DOUBLE_PIXEL_REPETITION;
		/* AVI Packet */
		writeb(AVI_PIXEL_REPETITION_DOUBLE, hdmi_base + S5P_AVI_BYTE5);
	}
	else /* clear pixel repetition */
	{
		/* AVI Packet */
		writeb(0x00, hdmi_base + S5P_AVI_BYTE5);
	}
	
	/* set pixel repetition */
	writeb(temp_reg8, hdmi_base + S5P_HDMI_CON_1 );

	/* set AVI packet VIC */
	if (pxl_ratio == HDMI_PIXEL_RATIO_16_9)
		writeb(video_params[mode].avi_vic_16_9, hdmi_base + S5P_AVI_BYTE4);
	else
		writeb(video_params[mode].avi_vic, hdmi_base + S5P_AVI_BYTE4);

	/* clear */
	temp_reg8 = readb(hdmi_base + S5P_AVI_BYTE2) &
			~(AVI_PICTURE_ASPECT_4_3 | AVI_PICTURE_ASPECT_16_9);
	
	if (pxl_ratio == HDMI_PIXEL_RATIO_16_9)
	{
		temp_reg8 |= AVI_PICTURE_ASPECT_16_9;
	}
	else
	{
		temp_reg8 |= AVI_PICTURE_ASPECT_4_3;
	}

	/* set AVI packet MM */
	writeb(temp_reg8, hdmi_base + S5P_AVI_BYTE2);

/*
	// set color depth
	if (HDMI_Sets5p_hdmi_color_depth(cd) != OK)
	{
		return ERROR;
	}
*/
	// config Phy
#if 0	
	if (PHYConfig(video_params[mode].pixel_clock, cd) == ERROR)
	{
		UART_Printf("PHYConfig() failed.\n");
		return ERROR;
	}	
	else
	{
		u32 i;

		// CMU Block : HDMI_SEL(SCLK_HDMI) - 0 : SCLK_PIXEL, 1 : SCLK_HDMIPHY
		temp_reg32 = Inp32(S3C_VA_SYS+0x0204);
		temp_reg32 |= (1<<0);
		Outp32(S3C_VA_SYS+0x0204, temp_reg32);

		// CMU Block : MIXER_SEL(SCLK_MIXER) - 0 : SCLK_DAC,  1 : SCLK_HDMI
		temp_reg32 = Inp32(S3C_VA_SYS+0x0204);
		temp_reg32 |= (1<<4);
		Outp32(S3C_VA_SYS+0x0204, temp_reg32);
	}
#endif

	return 0;
}


void __s5p_hdmi_set_hpd_onoff(bool on_off)
{
	HDMIPRINTK("hpd is %s\n\r", on_off ? "on":"off");

	if (on_off) {
		writel(SW_HPD_PLUGGED, hdmi_base + S5P_HPD);
	} else {
		writel(SW_HPD_UNPLUGGED, hdmi_base + S5P_HPD);
	}

	HDMIPRINTK("0x%08x\n\r", readl(hdmi_base + S5P_HPD));
}


void __s5p_hdmi_audio_set_config(s5p_tv_audio_codec_type audio_codec)
{

	u32 data_type = (audio_codec == PCM) ? CONFIG_LINEAR_PCM_TYPE :
			(audio_codec == AC3) ? CONFIG_NON_LINEAR_PCM_TYPE : 0xff;

	HDMIPRINTK("audio codec type = %s\n\r", 
		(audio_codec&PCM) ? "PCM":
		(audio_codec&AC3) ? "AC3":
		(audio_codec&MP3) ? "MP3":
		(audio_codec&WMA) ? "WMA":"Unknown");

	writel(CONFIG_FILTER_2_SAMPLE | data_type
	       | CONFIG_PCPD_MANUAL_SET | CONFIG_WORD_LENGTH_MANUAL_SET
	       | CONFIG_U_V_C_P_REPORT | CONFIG_BURST_SIZE_2
	       | CONFIG_DATA_ALIGN_32BIT
	       , hdmi_base + S5P_SPDIFIN_CONFIG_1);
	writel(0, hdmi_base + S5P_SPDIFIN_CONFIG_2);
}

void __s5p_hdmi_audio_set_acr(u32 sample_rate)
{
	u32 value_n = (sample_rate == 32000) ? 4096 :
		     (sample_rate == 44100) ? 6272 :
		     (sample_rate == 88200) ? 12544 :
		     (sample_rate == 176400) ? 25088 :
		     (sample_rate == 48000) ? 6144 :
		     (sample_rate == 96000) ? 12288 :
		     (sample_rate == 192000) ? 24576 : 0;

	u32 cts = (sample_rate == 32000) ? 27000 :
		      (sample_rate == 44100) ? 30000 :
		      (sample_rate == 88200) ? 30000 :
		      (sample_rate == 176400) ? 30000 :
		      (sample_rate == 48000) ? 27000 :
		      (sample_rate == 96000) ? 27000 :
		      (sample_rate == 192000) ? 27000 : 0;

	HDMIPRINTK("sample rate = %d\n\r", sample_rate);
	HDMIPRINTK("cts = %d\n\r", cts);

	writel(value_n & 0xff, hdmi_base + S5P_ACR_N0);
	writel((value_n >> 8) & 0xff, hdmi_base + S5P_ACR_N1);
	writel((value_n >> 16) & 0xff, hdmi_base + S5P_ACR_N2);

	writel(cts & 0xff, hdmi_base + S5P_ACR_MCTS0);
	writel((cts >> 8) & 0xff, hdmi_base + S5P_ACR_MCTS1);
	writel((cts >> 16) & 0xff, hdmi_base + S5P_ACR_MCTS2);

	writel(cts & 0xff, hdmi_base + S5P_ACR_CTS0);
	writel((cts >> 8) & 0xff, hdmi_base + S5P_ACR_CTS1);
	writel((cts >> 16) & 0xff, hdmi_base + S5P_ACR_CTS2);

	writel(4, hdmi_base + S5P_ACR_CON); //TO-DO : what it is?
}

void __s5p_hdmi_audio_set_asp(void)
{
	writel(0x0, hdmi_base + S5P_ASP_CON);
	/* All Subpackets contain audio samples */
	writel(0x0, hdmi_base + S5P_ASP_SP_FLAT); 

	writel(1 << 3 | 0, hdmi_base + S5P_ASP_CHCFG0);
	writel(1 << 3 | 0, hdmi_base + S5P_ASP_CHCFG1);
	writel(1 << 3 | 0, hdmi_base + S5P_ASP_CHCFG2);
	writel(1 << 3 | 0, hdmi_base + S5P_ASP_CHCFG3);
}

void  __s5p_hdmi_audio_clock_enable(void)
{
	writel(0x1, hdmi_base + S5P_SPDIFIN_CLK_CTRL);
	/* HDMI operation mode */
	writel(0x3, hdmi_base + S5P_SPDIFIN_OP_CTRL); 
}

void __s5p_hdmi_audio_set_repetition_time(s5p_tv_audio_codec_type audio_codec, 
					u32 bits, u32 frame_size_code)
{
	/* Only 4'b1011  24bit */
	u32 wl = 5 << 1 | 1; 
	u32 rpt_cnt = (audio_codec == AC3) ? 1536 * 2 - 1 : 0;

	HDMIPRINTK("repetition count = %d\n\r", rpt_cnt);

	/* 24bit and manual mode */
	writel(((rpt_cnt&0xf) << 4) | wl, hdmi_base + S5P_SPDIFIN_USER_VALUE_1); 
	/* if PCM this value is 0 */
	writel((rpt_cnt >> 4)&0xff, hdmi_base + S5P_SPDIFIN_USER_VALUE_2);
	/* if PCM this value is 0 */
	writel(frame_size_code&0xff, hdmi_base + S5P_SPDIFIN_USER_VALUE_3);	
	/* if PCM this value is 0 */
	writel((frame_size_code >> 8)&0xff, hdmi_base + S5P_SPDIFIN_USER_VALUE_4);
}



void __s5p_hdmi_audio_irq_enable(u32 irq_en)
{
	writel(irq_en, hdmi_base + S5P_SPDIFIN_IRQ_MASK);
}


void __s5p_hdmi_audio_set_aui(s5p_tv_audio_codec_type audio_codec, 
			u32 sample_rate, 
			u32 bits)
{
	u8 sum_of_bits, bytes1, bytes2, bytes3, check_sum;
	u32 bit_rate;
	/* Ac3 16bit only */
	u32 bps = (audio_codec == PCM) ? bits : 16; 

	/* AUI Packet set. */
	u32 type = (audio_codec == PCM) ? 1 :  		/* PCM */
			(audio_codec == AC3) ? 2 : 0; 	/* AC3 or Refer stream header */
	u32 ch = (audio_codec == PCM) ? 1 : 0; 		/* 2ch or refer to stream header */

	u32 sample = (sample_rate == 32000) ? 1 :
			(sample_rate == 44100) ? 2 :
			(sample_rate == 48000) ? 3 :
			(sample_rate == 88200) ? 4 :
			(sample_rate == 96000) ? 5 :
			(sample_rate == 176400) ? 6 :
			(sample_rate == 192000) ? 7 : 0;

	u32 bpsType = (bps == 16) ? 1 :
			(bps == 20) ? 2 :
			(bps == 24) ? 3 : 0;

	
	bpsType = (audio_codec == PCM) ? bpsType : 0;

	sum_of_bits = (0x84 + 0x1 + 10);//header

	bytes1 = (u8)((type << 4) | ch);

	bytes2 = (u8)((sample << 2) | bpsType);

	bit_rate = 256; // AC3 Test

	bytes3 = (audio_codec == PCM) ? (u8)0 : (u8)(bit_rate / 8) ;


	sum_of_bits += (bytes1 + bytes2 + bytes3);
	check_sum = 256 - sum_of_bits;

	/* AUI Packet set. */
	writel(check_sum , hdmi_base + S5P_AUI_CHECK_SUM);
	writel(bytes1 , hdmi_base + S5P_AUI_BYTE1);
	writel(bytes2 , hdmi_base + S5P_AUI_BYTE2);
	writel(bytes3 , hdmi_base + S5P_AUI_BYTE3); 	/* Pcm or stream */
	writel(0x00 , hdmi_base + S5P_AUI_BYTE4); 	/* 2ch pcm or Stream */
	writel(0x00 , hdmi_base + S5P_AUI_BYTE5); 	/* 2ch pcm or Stream */


	writel(2 , hdmi_base + S5P_ACP_CON);
	writel(1 , hdmi_base + S5P_ACP_TYPE);

	writel(0x10 , hdmi_base + S5P_GCP_BYTE1);
	/* 
	 * packet will be transmitted within 384 cycles 
	 * after active sync. 
	 */
	writel(0x2 , hdmi_base + S5P_GCP_CON);  

}

void __s5p_hdmi_video_set_bluescreen(bool en,
				     u8 cb_b,
				     u8 y_g,
				     u8 cr_r)
{
	HDMIPRINTK("%d,%d,%d,%d\n\r", en, cb_b, y_g, cr_r);

	if (en) {
		writel(SET_BLUESCREEN_0(cb_b), hdmi_base + S5P_BLUE_SCREEN_0);
		writel(SET_BLUESCREEN_1(y_g), hdmi_base + S5P_BLUE_SCREEN_1);
		writel(SET_BLUESCREEN_2(cr_r), hdmi_base + S5P_BLUE_SCREEN_2);
		writel(readl(hdmi_base + S5P_HDMI_CON_0) | BLUE_SCR_EN, 
			hdmi_base + S5P_HDMI_CON_0);

		HDMIPRINTK("HDMI_BLUE_SCREEN0 = 0x%08x \n\r", 
			readl(hdmi_base + S5P_BLUE_SCREEN_0));
		HDMIPRINTK("HDMI_BLUE_SCREEN1 = 0x%08x \n\r", 
			readl(hdmi_base + S5P_BLUE_SCREEN_1));
		HDMIPRINTK("HDMI_BLUE_SCREEN2 = 0x%08x \n\r", 
			readl(hdmi_base + S5P_BLUE_SCREEN_2));
	} else {
		writel(readl(hdmi_base + S5P_HDMI_CON_0)&~BLUE_SCR_EN, 
			hdmi_base + S5P_HDMI_CON_0);
	}

	HDMIPRINTK("HDMI_CON0 = 0x%08x \n\r", readl(hdmi_base + S5P_HDMI_CON_0));
}


s5p_tv_hdmi_err __s5p_hdmi_init_spd_infoframe(s5p_hdmi_transmit trans_type,
					u8 *spd_header,
					u8 *spd_data)
{
	HDMIPRINTK("%d,%d,%d\n\r", (u32)trans_type,
		   (u32)spd_header,
		   (u32)spd_data);

	switch (trans_type) {

	case HDMI_DO_NOT_TANS:
		writel(SPD_TX_CON_NO_TRANS, hdmi_base + S5P_SPD_CON);
		break;

	case HDMI_TRANS_ONCE:
		writel(SPD_TX_CON_TRANS_ONCE, hdmi_base + S5P_SPD_CON);
		break;

	case HDMI_TRANS_EVERY_SYNC:
		writel(SPD_TX_CON_TRANS_EVERY_VSYNC, hdmi_base + S5P_SPD_CON);
		break;

	default:
		HDMIPRINTK(" invalid out_mode parameter(%d)\n\r", trans_type);
		return S5P_TV_HDMI_ERR_INVALID_PARAM;
		break;
	}

	writel(SET_SPD_HEADER(*(spd_header)), hdmi_base + S5P_SPD_HEADER0);

	writel(SET_SPD_HEADER(*(spd_header + 1)) , hdmi_base + S5P_SPD_HEADER1);
	writel(SET_SPD_HEADER(*(spd_header + 2)) , hdmi_base + S5P_SPD_HEADER2);

	writel(SET_SPD_DATA(*(spd_data)), hdmi_base + S5P_SPD_DATA0);
	writel(SET_SPD_DATA(*(spd_data + 1)) , hdmi_base + S5P_SPD_DATA1);
	writel(SET_SPD_DATA(*(spd_data + 2)) , hdmi_base + S5P_SPD_DATA2);
	writel(SET_SPD_DATA(*(spd_data + 3)) , hdmi_base + S5P_SPD_DATA3);
	writel(SET_SPD_DATA(*(spd_data + 4)) , hdmi_base + S5P_SPD_DATA4);
	writel(SET_SPD_DATA(*(spd_data + 5)) , hdmi_base + S5P_SPD_DATA5);
	writel(SET_SPD_DATA(*(spd_data + 6)) , hdmi_base + S5P_SPD_DATA6);
	writel(SET_SPD_DATA(*(spd_data + 7)) , hdmi_base + S5P_SPD_DATA7);
	writel(SET_SPD_DATA(*(spd_data + 8)) , hdmi_base + S5P_SPD_DATA8);
	writel(SET_SPD_DATA(*(spd_data + 9)) , hdmi_base + S5P_SPD_DATA9);
	writel(SET_SPD_DATA(*(spd_data + 10)) , hdmi_base + S5P_SPD_DATA10);
	writel(SET_SPD_DATA(*(spd_data + 11)) , hdmi_base + S5P_SPD_DATA11);
	writel(SET_SPD_DATA(*(spd_data + 12)) , hdmi_base + S5P_SPD_DATA12);
	writel(SET_SPD_DATA(*(spd_data + 13)) , hdmi_base + S5P_SPD_DATA13);
	writel(SET_SPD_DATA(*(spd_data + 14)) , hdmi_base + S5P_SPD_DATA14);
	writel(SET_SPD_DATA(*(spd_data + 15)) , hdmi_base + S5P_SPD_DATA15);
	writel(SET_SPD_DATA(*(spd_data + 16)) , hdmi_base + S5P_SPD_DATA16);
	writel(SET_SPD_DATA(*(spd_data + 17)) , hdmi_base + S5P_SPD_DATA17);
	writel(SET_SPD_DATA(*(spd_data + 18)) , hdmi_base + S5P_SPD_DATA18);
	writel(SET_SPD_DATA(*(spd_data + 19)) , hdmi_base + S5P_SPD_DATA19);
	writel(SET_SPD_DATA(*(spd_data + 20)) , hdmi_base + S5P_SPD_DATA20);
	writel(SET_SPD_DATA(*(spd_data + 21)) , hdmi_base + S5P_SPD_DATA21);
	writel(SET_SPD_DATA(*(spd_data + 22)) , hdmi_base + S5P_SPD_DATA22);
	writel(SET_SPD_DATA(*(spd_data + 23)) , hdmi_base + S5P_SPD_DATA23);
	writel(SET_SPD_DATA(*(spd_data + 24)) , hdmi_base + S5P_SPD_DATA24);
	writel(SET_SPD_DATA(*(spd_data + 25)) , hdmi_base + S5P_SPD_DATA25);
	writel(SET_SPD_DATA(*(spd_data + 26)) , hdmi_base + S5P_SPD_DATA26);
	writel(SET_SPD_DATA(*(spd_data + 27)) , hdmi_base + S5P_SPD_DATA27);

	HDMIPRINTK("SPD_CON = 0x%08x \n\r", 
		readl(hdmi_base + S5P_SPD_CON));
	HDMIPRINTK("SPD_HEADER0 = 0x%08x \n\r", 
		readl(hdmi_base + S5P_SPD_HEADER0));
	HDMIPRINTK("SPD_HEADER1 = 0x%08x \n\r", 
		readl(hdmi_base + S5P_SPD_HEADER1));
	HDMIPRINTK("SPD_HEADER2 = 0x%08x \n\r", 
		readl(hdmi_base + S5P_SPD_HEADER2));
	HDMIPRINTK("SPD_DATA0  = 0x%08x \n\r",
		readl(hdmi_base + S5P_SPD_DATA0));
	HDMIPRINTK("SPD_DATA1  = 0x%08x \n\r", 
		readl(hdmi_base + S5P_SPD_DATA1));
	HDMIPRINTK("SPD_DATA2  = 0x%08x \n\r", 
		readl(hdmi_base + S5P_SPD_DATA2));
	HDMIPRINTK("SPD_DATA3  = 0x%08x \n\r", 
		readl(hdmi_base + S5P_SPD_DATA3));
	HDMIPRINTK("SPD_DATA4  = 0x%08x \n\r", 
		readl(hdmi_base + S5P_SPD_DATA4));
	HDMIPRINTK("SPD_DATA5  = 0x%08x \n\r",
		readl(hdmi_base + S5P_SPD_DATA5));
	HDMIPRINTK("SPD_DATA6  = 0x%08x \n\r",
		readl(hdmi_base + S5P_SPD_DATA6));
	HDMIPRINTK("SPD_DATA7  = 0x%08x \n\r", 
		readl(hdmi_base + S5P_SPD_DATA7));
	HDMIPRINTK("SPD_DATA8  = 0x%08x \n\r", 
		readl(hdmi_base + S5P_SPD_DATA8));
	HDMIPRINTK("SPD_DATA9  = 0x%08x \n\r",
		readl(hdmi_base + S5P_SPD_DATA9));
	HDMIPRINTK("SPD_DATA10 = 0x%08x \n\r",
		readl(hdmi_base + S5P_SPD_DATA10));
	HDMIPRINTK("SPD_DATA11 = 0x%08x \n\r",
		readl(hdmi_base + S5P_SPD_DATA11));
	HDMIPRINTK("SPD_DATA12 = 0x%08x \n\r",
		readl(hdmi_base + S5P_SPD_DATA12));
	HDMIPRINTK("SPD_DATA13 = 0x%08x \n\r",
		readl(hdmi_base + S5P_SPD_DATA13));
	HDMIPRINTK("SPD_DATA14 = 0x%08x \n\r", 
		readl(hdmi_base + S5P_SPD_DATA14));
	HDMIPRINTK("SPD_DATA15 = 0x%08x \n\r", 
		readl(hdmi_base + S5P_SPD_DATA15));
	HDMIPRINTK("SPD_DATA16 = 0x%08x \n\r", 
		readl(hdmi_base + S5P_SPD_DATA16));
	HDMIPRINTK("SPD_DATA17 = 0x%08x \n\r", 
		readl(hdmi_base + S5P_SPD_DATA17));
	HDMIPRINTK("SPD_DATA18 = 0x%08x \n\r", 
		readl(hdmi_base + S5P_SPD_DATA18));
	HDMIPRINTK("SPD_DATA19 = 0x%08x \n\r", 
		readl(hdmi_base + S5P_SPD_DATA19));
	HDMIPRINTK("SPD_DATA20 = 0x%08x \n\r", 
		readl(hdmi_base + S5P_SPD_DATA20));
	HDMIPRINTK("SPD_DATA21 = 0x%08x \n\r", 
		readl(hdmi_base + S5P_SPD_DATA21));
	HDMIPRINTK("SPD_DATA22 = 0x%08x \n\r", 
		readl(hdmi_base + S5P_SPD_DATA22));
	HDMIPRINTK("SPD_DATA23 = 0x%08x \n\r", 
		readl(hdmi_base + S5P_SPD_DATA23));
	HDMIPRINTK("SPD_DATA24 = 0x%08x \n\r", 
		readl(hdmi_base + S5P_SPD_DATA24));
	HDMIPRINTK("SPD_DATA25 = 0x%08x \n\r", 
		readl(hdmi_base + S5P_SPD_DATA25));
	HDMIPRINTK("SPD_DATA26 = 0x%08x \n\r",
		readl(hdmi_base + S5P_SPD_DATA26));
	HDMIPRINTK("SPD_DATA27 = 0x%08x \n\r", 
		readl(hdmi_base + S5P_SPD_DATA27));

	return HDMI_NO_ERROR;
}

void __s5p_hdmi_init_hpd_onoff(bool on_off)
{
	HDMIPRINTK("%d\n\r", on_off);
	__s5p_hdmi_set_hpd_onoff(on_off);
	HDMIPRINTK("0x%08x\n\r", readl(hdmi_base + S5P_HPD));
}

s5p_tv_hdmi_err __s5p_hdmi_audio_init(s5p_tv_audio_codec_type audio_codec, 
				u32 sample_rate, u32 bits, u32 frame_size_code)
{
	__s5p_hdmi_audio_set_config(audio_codec);
	__s5p_hdmi_audio_set_repetition_time(audio_codec, bits, frame_size_code);
	__s5p_hdmi_audio_irq_enable(IRQ_BUFFER_OVERFLOW_ENABLE);
	__s5p_hdmi_audio_clock_enable();
	__s5p_hdmi_audio_set_asp();
	__s5p_hdmi_audio_set_acr(sample_rate);
	__s5p_hdmi_audio_set_aui(audio_codec, sample_rate, bits);

	return HDMI_NO_ERROR;
}

s5p_tv_hdmi_err __s5p_hdmi_video_init_display_mode(s5p_tv_disp_mode disp_mode,
						s5p_tv_o_mode out_mode)
{
	s5p_tv_hdmi_disp_mode hdmi_disp_num;

	HDMIPRINTK("%d,%d\n\r", disp_mode, out_mode);

	switch (disp_mode) {

	case TVOUT_480P_60_16_9:

	case TVOUT_480P_60_4_3:

	case TVOUT_576P_50_16_9:

	case TVOUT_576P_50_4_3:

	case TVOUT_720P_60:

	case TVOUT_720P_50:
		writel(INT_PRO_MODE_PROGRESSIVE, hdmi_base + S5P_INT_PRO_MODE);
		break;

	default:
		HDMIPRINTK("invalid disp_mode parameter(%d)\n\r", disp_mode);
		return S5P_TV_HDMI_ERR_INVALID_PARAM;
		break;
	}

	switch (disp_mode) {

	case TVOUT_480P_60_16_9:

	case TVOUT_480P_60_4_3:
		hdmi_disp_num = S5P_TV_HDMI_DISP_MODE_480P_60;
		break;

	case TVOUT_576P_50_16_9:

	case TVOUT_576P_50_4_3:
		hdmi_disp_num = S5P_TV_HDMI_DISP_MODE_576P_50;
		break;

	case TVOUT_720P_60:
		hdmi_disp_num = S5P_TV_HDMI_DISP_MODE_720P_60;
		break;

	case TVOUT_720P_50:
		hdmi_disp_num = S5P_TV_HDMI_DISP_MODE_720P_50;
		break;

	default:
		HDMIPRINTK(" invalid disp_mode parameter(%d)\n\r", disp_mode);
		return S5P_TV_HDMI_ERR_INVALID_PARAM;
		break;
	}

	switch (out_mode) {
	case TVOUT_OUTPUT_HDMI:
		writel(PX_LMT_CTRL_BYPASS, hdmi_base + S5P_HDMI_CON_1);
		writel(VID_PREAMBLE_EN | GUARD_BAND_EN, hdmi_base + S5P_HDMI_CON_2);
		writel(HDMI_MODE_EN | DVI_MODE_DIS, hdmi_base + S5P_MODE_SEL);
		break;

	default:
		HDMIPRINTK("invalid out_mode parameter(%d)\n\r", out_mode);
		return S5P_TV_HDMI_ERR_INVALID_PARAM;
		break;
	}

//FPGA: FOR C110 HDMI TEST
	hdmi_set_video_mode(v1280x720p_60Hz, HDMI_CD_24,HDMI_PIXEL_RATIO_16_9);
//FPGA: FOR C110 HDMI TEST
	
	return HDMI_NO_ERROR;
}

void __s5p_hdmi_video_init_bluescreen(bool en,
				    u8 cb_b,
				    u8 y_g,
				    u8 cr_r)
{
	__s5p_hdmi_video_set_bluescreen(en, cb_b, y_g, cr_r);

}

void __s5p_hdmi_video_init_color_range(u8 y_min,
				     u8 y_max,
				     u8 c_min,
				     u8 c_max)
{
	HDMIPRINTK("%d,%d,%d,%d\n\r", y_max, y_min, c_max, c_min);

	writel(y_max, hdmi_base + S5P_HDMI_YMAX);
	writel(y_min, hdmi_base + S5P_HDMI_YMIN);
	writel(c_max, hdmi_base + S5P_HDMI_CMAX);
	writel(c_min, hdmi_base + S5P_HDMI_CMIN);

	HDMIPRINTK("HDMI_YMAX = 0x%08x \n\r", readl(hdmi_base + S5P_HDMI_YMAX));
	HDMIPRINTK("HDMI_YMIN = 0x%08x \n\r", readl(hdmi_base + S5P_HDMI_YMIN));
	HDMIPRINTK("HDMI_CMAX = 0x%08x \n\r", readl(hdmi_base + S5P_HDMI_CMAX));
	HDMIPRINTK("HDMI_CMIN = 0x%08x \n\r", readl(hdmi_base + S5P_HDMI_CMIN));
}

s5p_tv_hdmi_err __s5p_hdmi_video_init_csc(s5p_tv_hdmi_csc_type csc_type)
{
	unsigned short us_csc_coeff[10];

	HDMIPRINTK("%d)\n\r", csc_type);

	switch (csc_type) {

	case HDMI_CSC_YUV601_TO_RGB_LR:
		us_csc_coeff[0] = 0x23;
		us_csc_coeff[1] = 256;
		us_csc_coeff[2] = 938;
		us_csc_coeff[3] = 846;
		us_csc_coeff[4] = 256;
		us_csc_coeff[5] = 443;
		us_csc_coeff[6] = 0;
		us_csc_coeff[7] = 256;
		us_csc_coeff[8] = 0;
		us_csc_coeff[9] = 350;
		break;

	case HDMI_CSC_YUV601_TO_RGB_FR:
		us_csc_coeff[0] = 0x03;
		us_csc_coeff[1] = 298;
		us_csc_coeff[2] = 924;
		us_csc_coeff[3] = 816;
		us_csc_coeff[4] = 298;
		us_csc_coeff[5] = 516;
		us_csc_coeff[6] = 0;
		us_csc_coeff[7] = 298;
		us_csc_coeff[8] = 0;
		us_csc_coeff[9] = 408;
		break;

	case HDMI_CSC_YUV709_TO_RGB_LR:
		us_csc_coeff[0] = 0x23;
		us_csc_coeff[1] = 256;
		us_csc_coeff[2] = 978;
		us_csc_coeff[3] = 907;
		us_csc_coeff[4] = 256;
		us_csc_coeff[5] = 464;
		us_csc_coeff[6] = 0;
		us_csc_coeff[7] = 256;
		us_csc_coeff[8] = 0;
		us_csc_coeff[9] = 394;
		break;

	case HDMI_CSC_YUV709_TO_RGB_FR:
		us_csc_coeff[0] = 0x03;
		us_csc_coeff[1] = 298;
		us_csc_coeff[2] = 970;
		us_csc_coeff[3] = 888;
		us_csc_coeff[4] = 298;
		us_csc_coeff[5] = 540;
		us_csc_coeff[6] = 0;
		us_csc_coeff[7] = 298;
		us_csc_coeff[8] = 0;
		us_csc_coeff[9] = 458;
		break;

	case HDMI_CSC_YUV601_TO_YUV709:
		us_csc_coeff[0] = 0x33;
		us_csc_coeff[1] = 256;
		us_csc_coeff[2] = 995;
		us_csc_coeff[3] = 971;
		us_csc_coeff[4] = 0;
		us_csc_coeff[5] = 260;
		us_csc_coeff[6] = 29;
		us_csc_coeff[7] = 0;
		us_csc_coeff[8] = 19;
		us_csc_coeff[9] = 262;
		break;

	case HDMI_CSC_RGB_FR_TO_RGB_LR:
		us_csc_coeff[0] = 0x20;
		us_csc_coeff[1] = 220;
		us_csc_coeff[2] = 0;
		us_csc_coeff[3] = 0;
		us_csc_coeff[4] = 0;
		us_csc_coeff[5] = 220;
		us_csc_coeff[6] = 0;
		us_csc_coeff[7] = 0;
		us_csc_coeff[8] = 0;
		us_csc_coeff[9] = 220;
		break;

	case HDMI_CSC_RGB_FR_TO_YUV601:
		us_csc_coeff[0] = 0x30;
		us_csc_coeff[1] = 129;
		us_csc_coeff[2] = 25;
		us_csc_coeff[3] = 65;
		us_csc_coeff[4] = 950; //-74
		us_csc_coeff[5] = 112;
		us_csc_coeff[6] = 986; //-38
		us_csc_coeff[7] = 930; //-94
		us_csc_coeff[8] = 1006; //-18
		us_csc_coeff[9] = 112;
		break;

	case HDMI_CSC_RGB_FR_TO_YUV709:
		us_csc_coeff[0] = 0x30;
		us_csc_coeff[1] = 157;
		us_csc_coeff[2] = 16;
		us_csc_coeff[3] = 47;
		us_csc_coeff[4] = 937; //-87
		us_csc_coeff[5] = 112;
		us_csc_coeff[6] = 999; //-25
		us_csc_coeff[7] = 922; //-102
		us_csc_coeff[8] = 1014; //-10
		us_csc_coeff[9] = 112;
		break;

	case HDMI_BYPASS:
		us_csc_coeff[0] = 0x33;
		us_csc_coeff[1] = 256;
		us_csc_coeff[2] = 0;
		us_csc_coeff[3] = 0;
		us_csc_coeff[4] = 0;
		us_csc_coeff[5] = 256;
		us_csc_coeff[6] = 0;
		us_csc_coeff[7] = 0;
		us_csc_coeff[8] = 0;
		us_csc_coeff[9] = 256;
		break;

	default:
		HDMIPRINTK("invalid out_mode parameter(%d)\n\r", csc_type);
		return S5P_TV_HDMI_ERR_INVALID_PARAM;
		break;
	}

/*	writel(us_csc_coeff[0], hdmi_base + S5P_HDMI_CSC_CON);;

	writel(SET_HDMI_CSC_COEF_L(us_csc_coeff[1]), 
		hdmi_base + S5P_HDMI_Y_G_COEF_L);
	writel(SET_HDMI_CSC_COEF_H(us_csc_coeff[1]), 
		hdmi_base + S5P_HDMI_Y_G_COEF_H);
	writel(SET_HDMI_CSC_COEF_L(us_csc_coeff[2]), 
		hdmi_base + S5P_HDMI_Y_B_COEF_L);
	writel(SET_HDMI_CSC_COEF_H(us_csc_coeff[2]), 
		hdmi_base + S5P_HDMI_Y_B_COEF_H);
	writel(SET_HDMI_CSC_COEF_L(us_csc_coeff[3]), 
		hdmi_base + S5P_HDMI_Y_R_COEF_L);
	writel(SET_HDMI_CSC_COEF_H(us_csc_coeff[3]), 
		hdmi_base + S5P_HDMI_Y_R_COEF_H);
	writel(SET_HDMI_CSC_COEF_L(us_csc_coeff[4]), 
		hdmi_base + S5P_HDMI_CB_G_COEF_L);
	writel(SET_HDMI_CSC_COEF_H(us_csc_coeff[4]), 
		hdmi_base + S5P_HDMI_CB_G_COEF_H);
	writel(SET_HDMI_CSC_COEF_L(us_csc_coeff[5]), 
		hdmi_base + S5P_HDMI_CB_B_COEF_L);
	writel(SET_HDMI_CSC_COEF_H(us_csc_coeff[5]), 
		hdmi_base + S5P_HDMI_CB_B_COEF_H);
	writel(SET_HDMI_CSC_COEF_L(us_csc_coeff[6]), 
		hdmi_base + S5P_HDMI_CB_R_COEF_L);
	writel(SET_HDMI_CSC_COEF_H(us_csc_coeff[6]), 
		hdmi_base + S5P_HDMI_CB_R_COEF_H);
	writel(SET_HDMI_CSC_COEF_L(us_csc_coeff[7]), 
		hdmi_base + S5P_HDMI_CR_G_COEF_L);
	writel(SET_HDMI_CSC_COEF_H(us_csc_coeff[7]), 
		hdmi_base + S5P_HDMI_CR_G_COEF_H);
	writel(SET_HDMI_CSC_COEF_L(us_csc_coeff[8]), 
		hdmi_base + S5P_HDMI_CR_B_COEF_L);
	writel(SET_HDMI_CSC_COEF_H(us_csc_coeff[8]), 
		hdmi_base + S5P_HDMI_CR_B_COEF_H);
	writel(SET_HDMI_CSC_COEF_L(us_csc_coeff[9]), 
		hdmi_base + S5P_HDMI_CR_R_COEF_L);
	writel(SET_HDMI_CSC_COEF_H(us_csc_coeff[9]), 
		hdmi_base + S5P_HDMI_CR_R_COEF_H);

	HDMIPRINTK("HDMI_CSC_CON = 0x%08x \n\r", 
		readl(hdmi_base + S5P_HDMI_CSC_CON));
	HDMIPRINTK("HDMI_Y_G_COEF_L = 0x%08x \n\r", 
		readl(hdmi_base + S5P_HDMI_Y_G_COEF_L));
	HDMIPRINTK("HDMI_Y_G_COEF_H = 0x%08x \n\r", 
		readl(hdmi_base + S5P_HDMI_Y_G_COEF_H));
	HDMIPRINTK("HDMI_Y_B_COEF_L = 0x%08x \n\r", 
		readl(hdmi_base + S5P_HDMI_Y_B_COEF_L));
	HDMIPRINTK("HDMI_Y_B_COEF_H = 0x%08x \n\r", 
		readl(hdmi_base + S5P_HDMI_Y_B_COEF_H));
	HDMIPRINTK("HDMI_Y_R_COEF_L = 0x%08x \n\r", 
		readl(hdmi_base + S5P_HDMI_Y_R_COEF_L));
	HDMIPRINTK("HDMI_Y_R_COEF_H = 0x%08x \n\r", 
		readl(hdmi_base + S5P_HDMI_Y_R_COEF_H));
	HDMIPRINTK("HDMI_CB_G_COEF_L = 0x%08x \n\r", 
		readl(hdmi_base + S5P_HDMI_CB_G_COEF_L));
	HDMIPRINTK("HDMI_CB_G_COEF_H = 0x%08x \n\r", 
		readl(hdmi_base + S5P_HDMI_CB_G_COEF_H));
	HDMIPRINTK("HDMI_CB_B_COEF_L = 0x%08x \n\r", 
		readl(hdmi_base + S5P_HDMI_CB_B_COEF_L));
	HDMIPRINTK("HDMI_CB_B_COEF_H = 0x%08x \n\r", 
		readl(hdmi_base + S5P_HDMI_CB_B_COEF_H));
	HDMIPRINTK("HDMI_CB_R_COEF_L = 0x%08x \n\r", 
		readl(hdmi_base + S5P_HDMI_CB_R_COEF_L));
	HDMIPRINTK("HDMI_CB_R_COEF_H = 0x%08x \n\r", 
		readl(hdmi_base + S5P_HDMI_CB_R_COEF_H));
	HDMIPRINTK("HDMI_CR_G_COEF_L = 0x%08x \n\r", 
		readl(hdmi_base + S5P_HDMI_CR_G_COEF_L));
	HDMIPRINTK("HDMI_CR_G_COEF_H = 0x%08x \n\r", 
		readl(hdmi_base + S5P_HDMI_CR_G_COEF_H));
	HDMIPRINTK("HDMI_CR_B_COEF_L = 0x%08x \n\r", 
		readl(hdmi_base + S5P_HDMI_CR_B_COEF_L));
	HDMIPRINTK("HDMI_CR_B_COEF_H = 0x%08x \n\r",
		readl(hdmi_base + S5P_HDMI_CR_B_COEF_H));
	HDMIPRINTK("HDMI_CR_R_COEF_L = 0x%08x \n\r", 
		readl(hdmi_base + S5P_HDMI_CR_R_COEF_L));
	HDMIPRINTK("HDMI_CR_R_COEF_H = 0x%08x \n\r", 
		readl(hdmi_base + S5P_HDMI_CR_R_COEF_H));
*/
	return HDMI_NO_ERROR;
}

s5p_tv_hdmi_err __s5p_hdmi_video_init_avi_infoframe(s5p_hdmi_transmit trans_type,
						  u8 check_sum,
						  u8 *avi_data)
{
	HDMIPRINTK("%d,%d,%d\n\r", (u32)trans_type, (u32)check_sum, (u32)avi_data);

	switch (trans_type) {

	case HDMI_DO_NOT_TANS:
		writel(AVI_TX_CON_NO_TRANS, hdmi_base + S5P_AVI_CON);
		break;

	case HDMI_TRANS_ONCE:
		writel(AVI_TX_CON_TRANS_ONCE, hdmi_base + S5P_AVI_CON);
		break;

	case HDMI_TRANS_EVERY_SYNC:
		writel(AVI_TX_CON_TRANS_EVERY_VSYNC, hdmi_base + S5P_AVI_CON);
		break;

	default:
		HDMIPRINTK(" invalid out_mode parameter(%d)\n\r", trans_type);
		return S5P_TV_HDMI_ERR_INVALID_PARAM;
		break;
	}

	writel(SET_AVI_CHECK_SUM(check_sum), hdmi_base + S5P_AVI_CHECK_SUM);

	writel(SET_AVI_BYTE(*(avi_data)), hdmi_base + S5P_AVI_BYTE1);
	writel(SET_AVI_BYTE(*(avi_data + 1)), hdmi_base + S5P_AVI_BYTE2);
	writel(SET_AVI_BYTE(*(avi_data + 2)), hdmi_base + S5P_AVI_BYTE3);
	writel(SET_AVI_BYTE(*(avi_data + 3)), hdmi_base + S5P_AVI_BYTE4);
	writel(SET_AVI_BYTE(*(avi_data + 4)), hdmi_base + S5P_AVI_BYTE5);
	writel(SET_AVI_BYTE(*(avi_data + 5)), hdmi_base + S5P_AVI_BYTE6);
	writel(SET_AVI_BYTE(*(avi_data + 6)), hdmi_base + S5P_AVI_BYTE7);
	writel(SET_AVI_BYTE(*(avi_data + 7)), hdmi_base + S5P_AVI_BYTE8);
	writel(SET_AVI_BYTE(*(avi_data + 8)), hdmi_base + S5P_AVI_BYTE9);
	writel(SET_AVI_BYTE(*(avi_data + 9)), hdmi_base + S5P_AVI_BYTE10);
	writel(SET_AVI_BYTE(*(avi_data + 10)), hdmi_base + S5P_AVI_BYTE11);
	writel(SET_AVI_BYTE(*(avi_data + 11)), hdmi_base + S5P_AVI_BYTE12);
	writel(SET_AVI_BYTE(*(avi_data + 12)), hdmi_base + S5P_AVI_BYTE13);

	HDMIPRINTK("AVI_CON = 0x%08x \n\r", readl(hdmi_base + S5P_AVI_CON));
	HDMIPRINTK("AVI_CHECK_SUM = 0x%08x \n\r", readl(hdmi_base + S5P_AVI_CHECK_SUM));
	HDMIPRINTK("AVI_BYTE1  = 0x%08x \n\r", readl(hdmi_base + S5P_AVI_BYTE1));
	HDMIPRINTK("AVI_BYTE2  = 0x%08x \n\r", readl(hdmi_base + S5P_AVI_BYTE2));
	HDMIPRINTK("AVI_BYTE3  = 0x%08x \n\r", readl(hdmi_base + S5P_AVI_BYTE3));
	HDMIPRINTK("AVI_BYTE4  = 0x%08x \n\r", readl(hdmi_base + S5P_AVI_BYTE4));
	HDMIPRINTK("AVI_BYTE5  = 0x%08x \n\r", readl(hdmi_base + S5P_AVI_BYTE5));
	HDMIPRINTK("AVI_BYTE6  = 0x%08x \n\r", readl(hdmi_base + S5P_AVI_BYTE6));
	HDMIPRINTK("AVI_BYTE7  = 0x%08x \n\r", readl(hdmi_base + S5P_AVI_BYTE7));
	HDMIPRINTK("AVI_BYTE8  = 0x%08x \n\r", readl(hdmi_base + S5P_AVI_BYTE8));
	HDMIPRINTK("AVI_BYTE9  = 0x%08x \n\r", readl(hdmi_base + S5P_AVI_BYTE9));
	HDMIPRINTK("AVI_BYTE10 = 0x%08x \n\r", readl(hdmi_base + S5P_AVI_BYTE10));
	HDMIPRINTK("AVI_BYTE11 = 0x%08x \n\r", readl(hdmi_base + S5P_AVI_BYTE11));
	HDMIPRINTK("AVI_BYTE12 = 0x%08x \n\r", readl(hdmi_base + S5P_AVI_BYTE12));
	HDMIPRINTK("AVI_BYTE13 = 0x%08x \n\r", readl(hdmi_base + S5P_AVI_BYTE13));

	return HDMI_NO_ERROR;
}

s5p_tv_hdmi_err __s5p_hdmi_video_init_mpg_infoframe(s5p_hdmi_transmit trans_type,
						u8 check_sum,
						u8 *mpg_data)
{
	HDMIPRINTK("trans_type : %d,%d,%d\n\r", (u32)trans_type, 
			(u32)check_sum, (u32)mpg_data);

	switch (trans_type) {

	case HDMI_DO_NOT_TANS:
		writel(MPG_TX_CON_NO_TRANS,
		       hdmi_base + S5P_MPG_CON);
		break;

	case HDMI_TRANS_ONCE:
		writel(MPG_TX_CON_TRANS_ONCE,
		       hdmi_base + S5P_MPG_CON);
		break;

	case HDMI_TRANS_EVERY_SYNC:
		writel(MPG_TX_CON_TRANS_EVERY_VSYNC,
		       hdmi_base + S5P_MPG_CON);
		break;

	default:
		HDMIPRINTK("invalid out_mode parameter(%d)\n\r",
			   trans_type);
		return S5P_TV_HDMI_ERR_INVALID_PARAM;
		break;
	}

	writel(SET_MPG_CHECK_SUM(check_sum),

	       hdmi_base + S5P_MPG_CHECK_SUM);

	writel(SET_MPG_BYTE(*(mpg_data)),
	       hdmi_base + S5P_MPEG_BYTE1);
	writel(SET_MPG_BYTE(*(mpg_data + 1)),
	       hdmi_base + S5P_MPEG_BYTE2);
	writel(SET_MPG_BYTE(*(mpg_data + 2)),
	       hdmi_base + S5P_MPEG_BYTE3);
	writel(SET_MPG_BYTE(*(mpg_data + 3)),
	       hdmi_base + S5P_MPEG_BYTE4);
	writel(SET_MPG_BYTE(*(mpg_data + 4)),
	       hdmi_base + S5P_MPEG_BYTE5);

	HDMIPRINTK("MPG_CON = 0x%08x \n\r",
		   readl(hdmi_base + S5P_MPG_CON));
	HDMIPRINTK("MPG_CHECK_SUM = 0x%08x \n\r",
		   readl(hdmi_base + S5P_MPG_CHECK_SUM));
	HDMIPRINTK("MPEG_BYTE1 = 0x%08x \n\r",
		   readl(hdmi_base + S5P_MPEG_BYTE1));
	HDMIPRINTK("MPEG_BYTE2 = 0x%08x \n\r",
		   readl(hdmi_base + S5P_MPEG_BYTE2));
	HDMIPRINTK("MPEG_BYTE3 = 0x%08x \n\r",
		   readl(hdmi_base + S5P_MPEG_BYTE3));
	HDMIPRINTK("MPEG_BYTE4 = 0x%08x \n\r",
		   readl(hdmi_base + S5P_MPEG_BYTE4));
	HDMIPRINTK("MPEG_BYTE5 = 0x%08x \n\r",
		   readl(hdmi_base + S5P_MPEG_BYTE5));

	return HDMI_NO_ERROR;
}

void __s5p_hdmi_video_init_tg_cmd(bool timing_correction_en,
				bool bt656_sync_en,
				s5p_tv_disp_mode disp_mode,
				bool tg_en)
{
	u32 temp_reg = 0;

	HDMIPRINTK("%d,%d,%d,%d\n\r", timing_correction_en, 
				bt656_sync_en, disp_mode, tg_en);

	temp_reg = (timing_correction_en) ? GETSYNC_TYPE_EN : GETSYNC_TYPE_DIS;
	temp_reg |= (bt656_sync_en) ? GETSYNC_EN : GETSYNC_DIS;
	temp_reg |= (tg_en) ? TG_EN : TG_DIS;

	writel(temp_reg, hdmi_base + S5P_TG_CMD);

	HDMIPRINTK("TG_CMD = 0x%08x \n\r",
		   readl(hdmi_base + S5P_TG_CMD));
}


bool __s5p_hdmi_start(s5p_hdmi_audio_type hdmi_audio_type,
				bool hdcp_en,
				struct i2c_client *ddc_port)
{
//	u32 temp_reg = PWDN_ENB_NORMAL | HDMI_EN;
	u32 temp_reg = HDMI_EN;

	HDMIPRINTK("aud type : %d, hdcp enable : %d\n\r",
		   hdmi_audio_type, hdcp_en);

	switch (hdmi_audio_type) {

	case HDMI_AUDIO_PCM:
		temp_reg |= ASP_EN;
		break;

	case HDMI_AUDIO_NO:
		break;

	default:
		HDMIPRINTK(" invalid hdmi_audio_type(%d)\n\r",
			   hdmi_audio_type);
		return false;
		break;
	}

	writel(readl(hdmi_base + S5P_HDMI_CON_0) | temp_reg,
	       hdmi_base + S5P_HDMI_CON_0);
/*
	if (hdcp_en) {
		__s5p_init_hdcp(true, ddc_port);
		
		if (!__s5p_start_hdcp()) {
			HDMIPRINTK("HDCP start failed\n");
		}
	}
*/
	HDMIPRINTK("HPD : 0x%08x, HDMI_CON_0 : 0x%08x\n\r",
	//	   readl(hdmi_base + S5P_HDCP_CTRL),
		   readl(hdmi_base + S5P_HPD),
		   readl(hdmi_base + S5P_HDMI_CON_0));

	return true;
}



/*
* stop  - stop functions are only called under running HDMI
*/
void __s5p_hdmi_stop(void)
{
	u32 temp=0, result=0;
	HDMIPRINTK("\n\r");

//	__s5p_stop_hdcp();
/*
	writel(readl(hdmi_base + S5P_HDMI_CON_0) &
//MANUAL:	       ~(PWDN_ENB_NORMAL | HDMI_EN | ASP_EN),
	       ~(HDMI_EN | ASP_EN),
	       hdmi_base + S5P_HDMI_CON_0);
	    
*/
	temp = readl(hdmi_base + S5P_HDMI_CON_0);
	result = temp & HDMI_EN;
	
	if( result )
		writel(temp &  ~(HDMI_EN | ASP_EN), hdmi_base + S5P_HDMI_CON_0);

//	HDMIPRINTK("HPD 0x%08x,HDMI_CON_0 0x%08x\n\r",
//		   readl(hdmi_base + S5P_HDCP_CTRL),
//		   readl(hdmi_base + S5P_HPD),
//		   readl(hdmi_base + S5P_HDMI_CON_0));
}

int __init __s5p_hdmi_probe(struct platform_device *pdev, u32 res_num, u32 res_num2)
{
	struct resource *res;
	size_t	size;
	int 	ret;

	res = platform_get_resource(pdev, IORESOURCE_MEM, res_num);

	if (res == NULL) {
		dev_err(&pdev->dev, 
			"failed to get memory region resource\n");
		ret = -ENOENT;
	}

	size = (res->end - res->start) + 1;

	hdmi_mem = request_mem_region(res->start, size, pdev->name);

	if (hdmi_mem == NULL) {
		dev_err(&pdev->dev,  
			"failed to get memory region\n");
		ret = -ENOENT;
	}

	hdmi_base = ioremap(res->start, size);

	if (hdmi_base == NULL) {
		dev_err(&pdev->dev,  
			"failed to ioremap address region\n");
		ret = -ENOENT;
	}

	return ret;

}

int __init __s5p_hdmi_release(struct platform_device *pdev)
{
	iounmap(hdmi_base);

	/* remove memory region */
	if (hdmi_mem != NULL) {
		if (release_resource(hdmi_mem))
			dev_err(&pdev->dev,
				"Can't remove tvout drv !!\n");

		kfree(hdmi_mem);

		hdmi_mem = NULL;
	}

	return 0;
}

