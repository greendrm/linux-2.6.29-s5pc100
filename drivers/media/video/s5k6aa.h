#define S5K6AA_COMPLETE
#undef S5K6AA_COMPLETE
/*
 * Driver for S5K6AA (SXGA camera) from Samsung Electronics
 * 
 * 1/6" 1.3Mp CMOS Image Sensor SoC with an Embedded Image Processor
 * supporting MIPI CSI-2
 *
 * Copyright (C) 2009, Dongsoo Nathaniel Kim<dongsoo45.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#ifndef __S5K6AA_H__
#define __S5K6AA_H__

struct s5k6aa_reg {
	unsigned short addr;
	unsigned short val;
};

struct s5k6aa_regset_type {
	unsigned char *regset;
	int len;
};

/*
 * Macro
 */
#define REGSET_LENGTH(x)	(sizeof(x)/sizeof(s5k6aa_reg))

/*
 * Host S/W Register interface (0x70000000-0x70002000)
 */
/* Initialization section */
#define REG_TC_IPRM_InClockLSBs			0x01B8
#define REG_TC_IPRM_InClockMSBs			0x01BA
#define REG_TC_IPRM_PllFreqDiv4			0x01BC
#define REG_TC_IPRM_AddHeader			0x01BE
#define REG_TC_IPRM_ValidVActiveLow		0x01C0
#define REG_TC_IPRM_SenI2CAdr			0x01C2
#define REG_TC_IPRM_MI2CDrSclMan		0x01C4
#define REG_TC_IPRM_UseNPviClocks		0x01C6
#define REG_TC_IPRM_UseNMipiClocks		0x01C8
#define REG_TC_IPRM_bBlockInternalPllCalc	0x01CA

#define REG_TC_IPRM_OpClk4KHz_0			0x01CC
#define REG_TC_IPRM_MinOutRate4KHz_0		0x01CE
#define REG_TC_IPRM_MaxOutRate4KHz_0		0x01D0

#define REG_TC_IPRM_OpClk4KHz_1			0x01D2
#define REG_TC_IPRM_MinOutRate4KHz_1		0x01D4
#define REG_TC_IPRM_MaxOutRate4KHz_1		0x01D6

#define REG_TC_IPRM_OpClk4KHz_2			0x01D8
#define REG_TC_IPRM_MinOutRate4KHz_2		0x01DA
#define REG_TC_IPRM_MaxOutRate4KHz_2		0x01DC

#define REG_TC_IPRM_UseRegsAPI			0x01DE
#define REG_TC_IPRM_InitParamsUpdated		0x01E0
#define REG_TC_IPRM_ErrorInfo			0x01E2

/* Preview control section */
#define REG_0TC_PCFG_usWidth			0x0242
#define REG_0TC_PCFG_usHeight			0x0244
#define REG_0TC_PCFG_Format			0x0246
#define REG_0TC_PCFG_usMaxOut4KHzRate		0x0248
#define REG_0TC_PCFG_usMinOut4KHzRate		0x024A
#define REG_0TC_PCFG_PVIMask			0x024C
#define REG_0TC_PCFG_uClockInd			0x024E
#define REG_0TC_PCFG_usFrTimeType		0x0250
#define REG_0TC_PCFG_FrRateQualityType		0x0252
#define REG_0TC_PCFG_usMaxFrTimeMsecMult10	0x0254
#define REG_0TC_PCFG_usMinFrTimeMsecMult10	0x0256
#define REG_0TC_PCFG_sSaturation		0x0258
#define REG_0TC_PCFG_sSharpBlur			0x025A
#define REG_0TC_PCFG_sGlamour			0x025C
#define REG_0TC_PCFG_sColorTemp			0x025E
#define REG_0TC_PCFG_uDeviceGammaIndex		0x0260
#define REG_0TC_PCFG_uPrevMirror		0x0262
#define REG_0TC_PCFG_uCaptureMirror		0x0264
#define REG_0TC_PCFG_uRotation			0x0266

#define REG_1TC_PCFG_usWidth			0x0268
#define REG_1TC_PCFG_usHeight			0x026A
#define REG_1TC_PCFG_Format			0x026C
#define REG_1TC_PCFG_usMaxOut4KHzRate		0x026E
#define REG_1TC_PCFG_usMinOut4KHzRate		0x0270
#define REG_1TC_PCFG_PVIMask			0x0272
#define REG_1TC_PCFG_uClockInd			0x0274
#define REG_1TC_PCFG_usFrTimeType		0x0276
#define REG_1TC_PCFG_FrRateQualityType		0x0278
#define REG_1TC_PCFG_usMaxFrTimeMsecMult10	0x027A
#define REG_1TC_PCFG_usMinFrTimeMsecMult10	0x027C
#define REG_1TC_PCFG_sSaturation		0x027E
#define REG_1TC_PCFG_sSharpBlur			0x0280
#define REG_1TC_PCFG_sGlamour			0x0282
#define REG_1TC_PCFG_sColorTemp			0x0284
#define REG_1TC_PCFG_uDeviceGammaIndex		0x0286
#define REG_1TC_PCFG_uPrevMirror		0x0288
#define REG_1TC_PCFG_uCaptureMirror		0x028A
#define REG_1TC_PCFG_uRotation			0x028C

#define REG_2TC_PCFG_usWidth			0x028E
#define REG_2TC_PCFG_usHeight			0x0290
#define REG_2TC_PCFG_Format			0x0292
#define REG_2TC_PCFG_usMaxOut4KHzRate		0x0294
#define REG_2TC_PCFG_usMinOut4KHzRate		0x0296
#define REG_2TC_PCFG_PVIMask			0x0298
#define REG_2TC_PCFG_uClockInd			0x029A
#define REG_2TC_PCFG_usFrTimeType		0x029C
#define REG_2TC_PCFG_FrRateQualityType		0x029E
#define REG_2TC_PCFG_usMaxFrTimeMsecMult10	0x02A0
#define REG_2TC_PCFG_usMinFrTimeMsecMult10	0x02A2
#define REG_2TC_PCFG_sSaturation		0x02A4
#define REG_2TC_PCFG_sSharpBlur			0x02A6
#define REG_2TC_PCFG_sGlamour			0x02A8
#define REG_2TC_PCFG_sColorTemp			0x02AA
#define REG_2TC_PCFG_uDeviceGammaIndex		0x02AC
#define REG_2TC_PCFG_uPrevMirror		0x02AE
#define REG_2TC_PCFG_uCaptureMirror		0x02B0
#define REG_2TC_PCFG_uRotation			0x02B2

#define REG_3TC_PCFG_usWidth			0x02B4
#define REG_3TC_PCFG_usHeight			0x02B6
#define REG_3TC_PCFG_Format			0x02B8
#define REG_3TC_PCFG_usMaxOut4KHzRate		0x02BA
#define REG_3TC_PCFG_usMinOut4KHzRate		0x02BC
#define REG_3TC_PCFG_PVIMask			0x02BE
#define REG_3TC_PCFG_uClockInd			0x02C0
#define REG_3TC_PCFG_usFrTimeType		0x02C2
#define REG_3TC_PCFG_FrRateQualityType		0x02C4
#define REG_3TC_PCFG_usMaxFrTimeMsecMult10	0x02C6
#define REG_3TC_PCFG_usMinFrTimeMsecMult10	0x02C8
#define REG_3TC_PCFG_sSaturation		0x02CA
#define REG_3TC_PCFG_sSharpBlur			0x02CC
#define REG_3TC_PCFG_sGlamour			0x02CE
#define REG_3TC_PCFG_sColorTemp			0x02D0
#define REG_3TC_PCFG_uDeviceGammaIndex		0x02D2
#define REG_3TC_PCFG_uPrevMirror		0x02D4
#define REG_3TC_PCFG_uCaptureMirror		0x02D6
#define REG_3TC_PCFG_uRotation			0x02D8

#define REG_4TC_PCFG_usWidth			0x02DA
#define REG_4TC_PCFG_usHeight			0x02DC
#define REG_4TC_PCFG_Format			0x02DE
#define REG_4TC_PCFG_usMaxOut4KHzRate		0x02E0
#define REG_4TC_PCFG_usMinOut4KHzRate		0x02E2
#define REG_4TC_PCFG_PVIMask			0x02E4
#define REG_4TC_PCFG_uClockInd			0x02E6
#define REG_4TC_PCFG_usFrTimeType		0x02E8
#define REG_4TC_PCFG_FrRateQualityType		0x02EA
#define REG_4TC_PCFG_usMaxFrTimeMsecMult10	0x02EC
#define REG_4TC_PCFG_usMinFrTimeMsecMult10	0x02EE
#define REG_4TC_PCFG_sSaturation		0x02F0
#define REG_4TC_PCFG_sSharpBlur			0x02F2
#define REG_4TC_PCFG_sGlamour			0x02F4
#define REG_4TC_PCFG_sColorTemp			0x02F6
#define REG_4TC_PCFG_uDeviceGammaIndex		0x02F8
#define REG_4TC_PCFG_uPrevMirror		0x02FA
#define REG_4TC_PCFG_uCaptureMirror		0x02FC
#define REG_4TC_PCFG_uRotation			0x02FEa

#define REG_AC_TC_PCFG_usWidth			0x11C8
#define REG_AC_TC_PCFG_usHeight			0x11CA
#define REG_AC_TC_PCFG_Format			0x11CC
#define REG_AC_TC_PCFG_usMaxOut4KHzRate		0x11CE
#define REG_AC_TC_PCFG_usMinOut4KHzRate		0x11D0
#define REG_AC_TC_PCFG_PVIMask			0x11D2
#define REG_AC_TC_PCFG_uClockInd		0x11D4
#define REG_AC_TC_PCFG_usFrTimeType		0x11D6
#define REG_AC_TC_PCFG_FrRateQualityType	0x11D8
#define REG_AC_TC_PCFG_usMaxFrTimeMsecMult10	0x11DA
#define REG_AC_TC_PCFG_usMinFrTimeMsecMult10	0x11DC
#define REG_AC_TC_PCFG_sSaturation		0x11DE
#define REG_AC_TC_PCFG_sSharpBlur		0x11E0
#define REG_AC_TC_PCFG_sGlamour			0x11E2
#define REG_AC_TC_PCFG_sColorTemp		0x11E4
#define REG_AC_TC_PCFG_uDeviceGammaIndex	0x11E6
#define REG_AC_TC_PCFG_uPrevMirror		0x11E8
#define REG_AC_TC_PCFG_uCaptureMirror		0x11EA
#define REG_AC_TC_PCFG_uRotation		0x11EC

/* Capture control section */
#define REG_0TC_CCFG_uCaptureMode		0x030C
#define REG_0TC_CCFG_usWidth			0x030E
#define REG_0TC_CCFG_usHeight			0x0310
#define REG_0TC_CCFG_Format			0x3012
#define REG_0TC_CCFG_usMaxOut4KHzRate		0x0314
#define REG_0TC_CCFG_usMinOut4KHzRate		0x0316
#define REG_0TC_CCFG_PVIMask			0x0318
#define REG_0TC_CCFG_uClockInd			0x031A
#define REG_0TC_CCFG_usFrTimeType		0x031C
#define REG_0TC_CCFG_FrRateQualityType		0x031E
#define REG_0TC_CCFG_usMaxFrTimeMsecMult10	0x0320
#define REG_0TC_CCFG_usMinFrTimeMsecMult10	0x0322
#define REG_0TC_CCFG_sSaturation		0x0324
#define REG_0TC_CCFG_sSharpBlur			0x0326
#define REG_0TC_CCFG_sGlamour			0x0328
#define REG_0TC_CCFG_sColorTemp			0x032A
#define REG_0TC_CCFG_uDeviceGammaIndex		0x032C

#define REG_1TC_CCFG_uCaptureMode		0x032E
#define REG_1TC_CCFG_usWidth			0x0330
#define REG_1TC_CCFG_usHeight			0x0332
#define REG_1TC_CCFG_Format			0x0334
#define REG_1TC_CCFG_usMaxOut4KHzRate		0x0336
#define REG_1TC_CCFG_usMinOut4KHzRate		0x0338
#define REG_1TC_CCFG_PVIMask			0x033A
#define REG_1TC_CCFG_uClockInd			0x033C
#define REG_1TC_CCFG_usFrTimeType		0x033E
#define REG_1TC_CCFG_FrRateQualityType		0x0340
#define REG_1TC_CCFG_usMaxFrTimeMsecMult10	0x0342
#define REG_1TC_CCFG_usMinFrTimeMsecMult10	0x0344
#define REG_1TC_CCFG_sSaturation		0x0346
#define REG_1TC_CCFG_sSharpBlur			0x0348
#define REG_1TC_CCFG_sGlamour			0x034A
#define REG_1TC_CCFG_sColorTemp			0x034C
#define REG_1TC_CCFG_uDeviceGammaIndex		0x034E

#define REG_2TC_CCFG_uCaptureMode		0x0350
#define REG_2TC_CCFG_usWidth			0x0352
#define REG_2TC_CCFG_usHeight			0x0354
#define REG_2TC_CCFG_Format			0x0356
#define REG_2TC_CCFG_usMaxOut4KHzRate		0x0358
#define REG_2TC_CCFG_usMinOut4KHzRate		0x035A
#define REG_2TC_CCFG_PVIMask			0x035C
#define REG_2TC_CCFG_uClockInd			0x035E
#define REG_2TC_CCFG_usFrTimeType		0x0360
#define REG_2TC_CCFG_FrRateQualityType		0x0362
#define REG_2TC_CCFG_usMaxFrTimeMsecMult10	0x0364
#define REG_2TC_CCFG_usMinFrTimeMsecMult10	0x0366
#define REG_2TC_CCFG_sSaturation		0x0368
#define REG_2TC_CCFG_sSharpBlur			0x036A
#define REG_2TC_CCFG_sGlamour			0x036C
#define REG_2TC_CCFG_sColorTemp			0x036E
#define REG_2TC_CCFG_uDeviceGammaIndex		0x0370

#define REG_3TC_CCFG_uCaptureMode		0x0372
#define REG_3TC_CCFG_usWidth			0x0374
#define REG_3TC_CCFG_usHeight			0x0376
#define REG_3TC_CCFG_Format			0x0378
#define REG_3TC_CCFG_usMaxOut4KHzRate		0x037A
#define REG_3TC_CCFG_usMinOut4KHzRate		0x037C
#define REG_3TC_CCFG_PVIMask			0x037E
#define REG_3TC_CCFG_uClockInd			0x0380
#define REG_3TC_CCFG_usFrTimeType		0x0382
#define REG_3TC_CCFG_FrRateQualityType		0x0384
#define REG_3TC_CCFG_usMaxFrTimeMsecMult10	0x0386
#define REG_3TC_CCFG_usMinFrTimeMsecMult10	0x0388
#define REG_3TC_CCFG_sSaturation		0x038A
#define REG_3TC_CCFG_sSharpBlur			0x038C
#define REG_3TC_CCFG_sGlamour			0x038E
#define REG_3TC_CCFG_sColorTemp			0x0390
#define REG_3TC_CCFG_uDeviceGammaIndex		0x0392

#define REG_4TC_CCFG_uCaptureMode		0x0394
#define REG_4TC_CCFG_usWidth			0x0396
#define REG_4TC_CCFG_usHeight			0x0398
#define REG_4TC_CCFG_Format			0x039A
#define REG_4TC_CCFG_usMaxOut4KHzRate		0x039C
#define REG_4TC_CCFG_usMinOut4KHzRate		0x039E
#define REG_4TC_CCFG_PVIMask			0x03A0
#define REG_4TC_CCFG_uClockInd			0x03A2
#define REG_4TC_CCFG_usFrTimeType		0x03A4
#define REG_4TC_CCFG_FrRateQualityType		0x03A6
#define REG_4TC_CCFG_usMaxFrTimeMsecMult10	0x03A8
#define REG_4TC_CCFG_usMinFrTimeMsecMult10	0x03AA
#define REG_4TC_CCFG_sSaturation		0x03AC
#define REG_4TC_CCFG_sSharpBlur			0x03AE
#define REG_4TC_CCFG_sGlamour			0x03B0
#define REG_4TC_CCFG_sColorTemp			0x03B2
#define REG_4TC_CCFG_uDeviceGammaIndex		0x03B4

#define REG_AC_TC_CCFG_uCaptureMode		0x1214
#define REG_AC_TC_CCFG_usWidth			0x1216
#define REG_AC_TC_CCFG_usHeight			0x1218
#define REG_AC_TC_CCFG_Format			0x121A
#define REG_AC_TC_CCFG_usMaxOut4KHzRate		0x121C
#define REG_AC_TC_CCFG_usMinOut4KHzRate		0x121E
#define REG_AC_TC_CCFG_PVIMask			0x1220
#define REG_AC_TC_CCFG_uClockInd		0x1222
#define REG_AC_TC_CCFG_usFrTimeType		0x1224
#define REG_AC_TC_CCFG_FrRateQualityType	0x1226
#define REG_AC_TC_CCFG_usMaxFrTimeMsecMult10	0x1228
#define REG_AC_TC_CCFG_usMinFrTimeMsecMult10	0x122A
#define REG_AC_TC_CCFG_sSaturation		0x122C
#define REG_AC_TC_CCFG_sSharpBlur		0x122E
#define REG_AC_TC_CCFG_sGlamour			0x1230
#define REG_AC_TC_CCFG_sColorTemp		0x1232
#define REG_AC_TC_CCFG_uDeviceGammaIndex	0x1234

/* Configuration value section */
/* Frame rate */
/* for REG_[NUM]TC_PCFG_usFrTimeType */
#define TC_FR_TIME_DYNAMIC			0
#define TC_FR_TIME_FIXED_NOT_ACCURATE		1
#define TC_FR_TIME_FIXED_ACCURATE		2
/* for REG_[NUM]TC_PCFG_FrRateQualityType */
#define TC_FRVSQ_BEST_FRRATE			1	/* Binning enabled */
#define TC_FRVSQ_BEST_QUALITY			2 	/* Binning disabled */

/* General purpose section */
#define REG_TC_GP_SpecialEffects		0x01EE
#define REG_TC_GP_EnablePreview			0x01F0
#define REG_TC_GP_EnablePreviewChanged		0x01F2
#define REG_TC_GP_EnableCapture			0x01F4
#define REG_TC_GP_EnableCaptureChanged		0x01F6
#define REG_TC_GP_NewConfigSync			0x01F8
#define REG_TC_GP_PrevReqInputWidth		0x01FA
#define REG_TC_GP_PrevReqInputHeight		0x01FC
#define REG_TC_GP_PrevInputWidthOfs		0x01FE
#define REG_TC_GP_PrevInputHeightOfs		0x0200
#define REG_TC_GP_CapReqInputWidth		0x0202
#define REG_TC_GP_CapReqInputHeight		0x0204
#define REG_TC_GP_CapInputWidthOfs		0x0206
#define REG_TC_GP_CapInputHeightOfs		0x0208
#define REG_TC_GP_PrevZoomReqInputWidth		0x020A
#define REG_TC_GP_PrevZoomReqInputHeight	0x020C
#define REG_TC_GP_PrevZoomReqInputWidthOfs	0x020E
#define REG_TC_GP_PrevZoomReqInputHeightOfs	0x0210
#define REG_TC_GP_CapZoomReqInputWidth		0x0212
#define REG_TC_GP_CapZoomReqInputHeight		0x0214
#define REG_TC_GP_CapZoomReqInputWidthOfs	0x0216
#define REG_TC_GP_CapZoomReqInputHeightOfs	0x0218
#define REG_TC_GP_InputsChangeRequest		0x021A
#define REG_TC_GP_ActivePrevConfig		0x021C
#define REG_TC_GP_PrevConfigChanged		0x021E
#define REG_TC_GP_PrevOpenAfterChange		0x0220
#define REG_TC_GP_ErrorPrevConfig		0x0222
#define REG_TC_GP_ActiveCapConfig		0x0224
#define REG_TC_GP_CapConfigChanged		0x0226
#define REG_TC_GP_ErrorCapConfig		0x0228
#define REG_TC_GP_PrevConfigBypassChanged	0x022A
#define REG_TC_GP_CapConfigBypassChanged	0x022C
#define REG_TC_GP_SleepMode			0x022E
#define REG_TC_GP_SleepModeChanged		0x0230
#define REG_TC_GP_SRA_AddLow			0x0232
#define REG_TC_GP_SRA_AddHigh			0x0234
#define REG_TC_GP_SRA_AccessType		0x0236
#define REG_TC_GP_SRA_Changed			0x0238
#define REG_TC_GP_PrevMinFrTimeMsecMult10	0x023A
#define REG_TC_GP_PrevOutKHzRate		0x023C
#define REG_TC_GP_CapMinFrTimeMsecMult10	0x023E
#define REG_TC_GP_CapOutKHzRate			0x0240

/* Image property control section */
#define REG_TC_UserBrightness			0x01E4
#define REG_TC_UserContrast			0x01E6
#define REG_TC_UserSaturation			0x01E8
#define REG_TC_UserSharpBlur			0x01EA
#define REG_TC_UserGlamour			0x01EC

/* Flash control section */
#define REG_TC_FLS_Mode				0x03B6
#define REG_TC_FLS_Threshold			0x03B8
#define REG_TC_FLS_Polarity			0x03BA
#define REG_TC_FLS_XenonMode			0x03BC
#define REG_TC_FLS_XenonPreFlashCnt		0x03BE

/* Extended image property control section */
#define REG_SF_USER_LeiLow			0x03C0
#define REG_SF_USER_LeiHigh			0x03C2
#define REG_SF_USER_LeiChanged			0x03C4
#define REG_SF_USER_Exposure			0x03C6
#define REG_SF_USER_ExposureChanged		0x03CA
#define REG_SF_USER_TotalGain			0x03CC
#define REG_SF_USER_TotalGainChanged		0x03CE
#define REG_SF_USER_Rgain			0x03D0
#define REG_SF_USER_RgainChanged		0x03D2
#define REG_SF_USER_Ggain			0x03D4
#define REG_SF_USER_GgainChanged		0x03D6
#define REG_SF_USER_Bgain			0x03D8
#define REG_SF_USER_BgainChanged		0x03DA
#define REG_SF_USER_FlickerQuant		0x03DC
#define REG_SF_USER_FlickerQuantChanged		0x03DE
#define REG_SF_USER_GASRAlphaVal		0x03E0
#define REG_SF_USER_GASRAlphaChanged		0x03E2
#define REG_SF_USER_GASGAlphaVal		0x03E4
#define REG_SF_USER_GASGAlphaChanged		0x03E6
#define REG_SF_USER_GASBAlphaVal		0x03E8
#define REG_SF_USER_GASBAlphaChanged		0x03EA
#define REG_SF_USER_DbgIdx			0x03EC
#define REG_SF_USER_DbgVal			0x03EE
#define REG_SF_USER_DbgChanged			0x03F0
#define REG_SF_USER_aGain			0x03F2
#define REG_SF_USER_aGainChanged		0x03F4
#define REG_SF_USER_dGain			0x03F6
#define REG_SF_USER_dGainChanged		0x03F8

/* Output interface control section */
#define REG_TC_OIF_EnMipiLanes			0x03FA
#define REG_TC_OIF_EnPackets			0x03FC
#define REG_TC_OIF_CfgChanged			0x03FE

/* Debug control section */
#define REG_TC_DBG_AutoAlgEnBits		0x0400
#define REG_TC_DBG_IspBypass			0x0402
#define REG_TC_DBG_ReInitCmd			0x0404

/* Version information section */
#define REG_FWdate				0x012C
#define REG_FWapiVer				0x012E
#define REG_FWrevision				0x0130
#define REG_FWpid				0x0132
#define REG_FWprjName				0x0134
#define REG_FWcompDate				0x0140
#define REG_FWSFC_VER				0x014C
#define REG_FWTC_VER				0x014E
#define REG_FWrealImageLine			0x0150
#define REG_FWsenId				0x0152
#define REG_FWusDevIdQaVersion			0x0154
#define REG_FWusFwCompilationBits		0x0156
#define REG_ulSVNrevision			0x0158
#define REG_SVNpathRomAddress			0x015C
#define REG_TRAP_N_PATCH_START_ADD		0x1B00


/*
 * User defined commands
 */
/* S/W defined features for tune */
#define REG_DELAY	0xFF00	/* in ms */
#define REG_CMD		0xFFFF	/* Followed by command */

/* Following order should not be changed */
enum image_size_s5k6aa {
	/* This SoC supports upto SXGA (1280*1024) */
#if 0
	QQVGA,	/* 160*120*/
	QCIF,	/* 176*144 */
	QVGA,	/* 320*240 */
	CIF,	/* 352*288 */
#endif
	VGA,	/* 640*480 */
#if 0
	SVGA,	/* 800*600 */
	HD720P,	/* 1280*720 */
	SXGA,	/* 1280*1024 */
#endif
};

/*
 * Following values describe controls of camera
 * in user aspect and must be match with index of s5k6aa_regset[]
 * These values indicates each controls and should be used
 * to control each control
 */
enum s5k6aa_control {
	S5K6AA_INIT,
	S5K6AA_EV,
	S5K6AA_AWB,
	S5K6AA_MWB,
	S5K6AA_EFFECT,
	S5K6AA_CONTRAST,
	S5K6AA_SATURATION,
	S5K6AA_SHARPNESS,
};

#define S5K6AA_REGSET(x)	{	\
	.regset = x,			\
	.len = sizeof(x)/sizeof(s5k6aa_reg),}


/*
 * User tuned register setting values
 */
static const struct s5k6aa_reg s5k6aa_init[] = {
	/* Arm initialize */
	{0x0010, 0x0001},	/* Reset */
	{0x1030, 0x0000},	/* Clear host interrupt so main will wait */
	{0x0014, 0x0001},	/* ARM go */
	{REG_DELAY, 100},	/* Wait100mSec */

	/* Start of Trap and Patch */
	{0x0028, 0x7000},	/* start add MSW */
	{0x002A, 0x1d60},	/* start add LSW */
	{0x0F12, 0xb570}, 
	{0x0F12, 0x4928},
	{0x0F12, 0x4828},
	{0x0F12, 0x2205},
	{0x0F12, 0xf000},
	{0x0F12, 0xf922},
	{0x0F12, 0x4927},
	{0x0F12, 0x2002},
	{0x0F12, 0x83c8},
	{0x0F12, 0x2001},
	{0x0F12, 0x3120},
	{0x0F12, 0x8088},
	{0x0F12, 0x4925},
	{0x0F12, 0x4826},
	{0x0F12, 0x2204},
	{0x0F12, 0xf000},
	{0x0F12, 0xf917},
	{0x0F12, 0x4925},
	{0x0F12, 0x4825},
	{0x0F12, 0x2206},
	{0x0F12, 0xf000},
	{0x0F12, 0xf912},
	{0x0F12, 0x4924},
	{0x0F12, 0x4825},
	{0x0F12, 0x2207},
	{0x0F12, 0xf000},
	{0x0F12, 0xf90d},
	{0x0F12, 0x4924},
	{0x0F12, 0x4824},
	{0x0F12, 0x2208},
	{0x0F12, 0xf000},
	{0x0F12, 0xf908},
	{0x0F12, 0x4923},
	{0x0F12, 0x4824},
	{0x0F12, 0x2209},
	{0x0F12, 0xf000},
	{0x0F12, 0xf903},
	{0x0F12, 0x4923},
	{0x0F12, 0x4823},
	{0x0F12, 0x60c1},
	{0x0F12, 0x6882},
	{0x0F12, 0x1a51},
	{0x0F12, 0x8201},
	{0x0F12, 0x4c22},
	{0x0F12, 0x2607},
	{0x0F12, 0x6821},
	{0x0F12, 0x0736},
	{0x0F12, 0x42b1},
	{0x0F12, 0xda05},
	{0x0F12, 0x4820},
	{0x0F12, 0x22d8},
	{0x0F12, 0x1c05},
	{0x0F12, 0xf000},
	{0x0F12, 0xf8fa},
	{0x0F12, 0x6025},
	{0x0F12, 0x68a1},
	{0x0F12, 0x42b1},
	{0x0F12, 0xda07},
	{0x0F12, 0x481b},
	{0x0F12, 0x2224},
	{0x0F12, 0x3824},
	{0x0F12, 0xf000},
	{0x0F12, 0xf8f1},
	{0x0F12, 0x4819},
	{0x0F12, 0x3824},
	{0x0F12, 0x60a0},
	{0x0F12, 0x4d18},
	{0x0F12, 0x6d29},
	{0x0F12, 0x42b1},
	{0x0F12, 0xda07},
	{0x0F12, 0x4815},
	{0x0F12, 0x228f},
	{0x0F12, 0x00d2},
	{0x0F12, 0x30d8},
	{0x0F12, 0x1c04},
	{0x0F12, 0xf000},
	{0x0F12, 0xf8e3},
	{0x0F12, 0x652c},
	{0x0F12, 0xbc70},
	{0x0F12, 0xbc08},
	{0x0F12, 0x4718},
	{0x0F12, 0x0000},
	{0x0F12, 0x1f53},
	{0x0F12, 0x7000},
	{0x0F12, 0x127b},
	{0x0F12, 0x0000},
	{0x0F12, 0x0398},
	{0x0F12, 0x7000},
	{0x0F12, 0x1e4d},
	{0x0F12, 0x7000},
	{0x0F12, 0x890d},
	{0x0F12, 0x0000},
	{0x0F12, 0x1e73},
	{0x0F12, 0x7000},
	{0x0F12, 0x27a9},
	{0x0F12, 0x0000},
	{0x0F12, 0x1e91},
	{0x0F12, 0x7000},
	{0x0F12, 0x27c5},
	{0x0F12, 0x0000},
	{0x0F12, 0x1ef7},
	{0x0F12, 0x7000},
	{0x0F12, 0x285f},
	{0x0F12, 0x0000},
	{0x0F12, 0x1eb3},
	{0x0F12, 0x7000},
	{0x0F12, 0x28ff},
	{0x0F12, 0x0000},
	{0x0F12, 0x206c},
	{0x0F12, 0x7000},
	{0x0F12, 0x04ac},
	{0x0F12, 0x7000},
	{0x0F12, 0x06cc},
	{0x0F12, 0x7000},
	{0x0F12, 0x23a4},
	{0x0F12, 0x7000},
	{0x0F12, 0x0704},
	{0x0F12, 0x7000},
	{0x0F12, 0xb510},
	{0x0F12, 0x1c04},
	{0x0F12, 0x484d},
	{0x0F12, 0xf000},
	{0x0F12, 0xf8bb},
	{0x0F12, 0x4a4d},
	{0x0F12, 0x4b4d},
	{0x0F12, 0x8811},
	{0x0F12, 0x885b},
	{0x0F12, 0x8852},
	{0x0F12, 0x4359},
	{0x0F12, 0x1889},
	{0x0F12, 0x4288},
	{0x0F12, 0xd800},
	{0x0F12, 0x1c08},
	{0x0F12, 0x6020},
	{0x0F12, 0xbc10},
	{0x0F12, 0xbc08},
	{0x0F12, 0x4718},
	{0x0F12, 0xb510},
	{0x0F12, 0x1c04},
	{0x0F12, 0xf000},
	{0x0F12, 0xf8b1},
	{0x0F12, 0x4944},
	{0x0F12, 0x8989},
	{0x0F12, 0x4348},
	{0x0F12, 0x0200},
	{0x0F12, 0x0c00},
	{0x0F12, 0x2101},
	{0x0F12, 0x0349},
	{0x0F12, 0xf000},
	{0x0F12, 0xf8b0},
	{0x0F12, 0x6020},
	{0x0F12, 0xe7ed},
	{0x0F12, 0xb510},
	{0x0F12, 0x1c04},
	{0x0F12, 0xf000},
	{0x0F12, 0xf8b2},
	{0x0F12, 0x6821},
	{0x0F12, 0x0409},
	{0x0F12, 0x0c09},
	{0x0F12, 0x1a40},
	{0x0F12, 0x493a},
	{0x0F12, 0x6849},
	{0x0F12, 0x4281},
	{0x0F12, 0xd800},
	{0x0F12, 0x1c08},
	{0x0F12, 0xf000},
	{0x0F12, 0xf8af},
	{0x0F12, 0x6020},
	{0x0F12, 0xe7dc},
	{0x0F12, 0xb570},
	{0x0F12, 0x6801},
	{0x0F12, 0x040d},
	{0x0F12, 0x0c2d},
	{0x0F12, 0x6844},
	{0x0F12, 0x4833},
	{0x0F12, 0x8981},
	{0x0F12, 0x1c28},
	{0x0F12, 0xf000},
	{0x0F12, 0xf893},
	{0x0F12, 0x8060},
	{0x0F12, 0x4932},
	{0x0F12, 0x69c9},
	{0x0F12, 0xf000},
	{0x0F12, 0xf8a6},
	{0x0F12, 0x1c01},
	{0x0F12, 0x80a0},
	{0x0F12, 0x0228},
	{0x0F12, 0xf000},
	{0x0F12, 0xf8a9},
	{0x0F12, 0x0400},
	{0x0F12, 0x0c00},
	{0x0F12, 0x8020},
	{0x0F12, 0x492d},
	{0x0F12, 0x2300},
	{0x0F12, 0x5ec9},
	{0x0F12, 0x4288},
	{0x0F12, 0xda02},
	{0x0F12, 0x20ff},
	{0x0F12, 0x3001},
	{0x0F12, 0x8020},
	{0x0F12, 0xbc70},
	{0x0F12, 0xbc08},
	{0x0F12, 0x4718},
	{0x0F12, 0xb570},
	{0x0F12, 0x1c04},
	{0x0F12, 0x4828},
	{0x0F12, 0x4926},
	{0x0F12, 0x7803},
	{0x0F12, 0x6a8a},
	{0x0F12, 0x2b00},
	{0x0F12, 0xd100},
	{0x0F12, 0x6a0a},
	{0x0F12, 0x4d20},
	{0x0F12, 0x2b00},
	{0x0F12, 0x68a8},
	{0x0F12, 0xd100},
	{0x0F12, 0x6868},
	{0x0F12, 0x6823},
	{0x0F12, 0x8dc9},
	{0x0F12, 0x434a},
	{0x0F12, 0x0a12},
	{0x0F12, 0x429a},
	{0x0F12, 0xd30d},
	{0x0F12, 0x4d20},
	{0x0F12, 0x26ff},
	{0x0F12, 0x8828},
	{0x0F12, 0x3601},
	{0x0F12, 0x43b0},
	{0x0F12, 0x8028},
	{0x0F12, 0x6820},
	{0x0F12, 0xf000},
	{0x0F12, 0xf884},
	{0x0F12, 0x6020},
	{0x0F12, 0x8828},
	{0x0F12, 0x4330},
	{0x0F12, 0x8028},
	{0x0F12, 0xe7da},
	{0x0F12, 0x1c0a},
	{0x0F12, 0x4342},
	{0x0F12, 0x0a12},
	{0x0F12, 0x429a},
	{0x0F12, 0xd304},
	{0x0F12, 0x0218},
	{0x0F12, 0xf000},
	{0x0F12, 0xf871},
	{0x0F12, 0x6020},
	{0x0F12, 0xe7f4},
	{0x0F12, 0x6020},
	{0x0F12, 0xe7f2},
	{0x0F12, 0xb510},
	{0x0F12, 0x4913},
	{0x0F12, 0x8fc8},
	{0x0F12, 0x2800},
	{0x0F12, 0xd007},
	{0x0F12, 0x2000},
	{0x0F12, 0x87c8},
	{0x0F12, 0x8f88},
	{0x0F12, 0x4c11},
	{0x0F12, 0x2800},
	{0x0F12, 0xd002},
	{0x0F12, 0x2008},
	{0x0F12, 0x8020},
	{0x0F12, 0xe77e},
	{0x0F12, 0x480d},
	{0x0F12, 0x3060},
	{0x0F12, 0x8900},
	{0x0F12, 0x2800},
	{0x0F12, 0xd103},
	{0x0F12, 0x480c},
	{0x0F12, 0x2101},
	{0x0F12, 0xf000},
	{0x0F12, 0xf864},
	{0x0F12, 0x2010},
	{0x0F12, 0x8020},
	{0x0F12, 0xe7f2},
	{0x0F12, 0x0000},
	{0x0F12, 0xf4b0},
	{0x0F12, 0x0000},
	{0x0F12, 0x2058},
	{0x0F12, 0x7000},
	{0x0F12, 0x1554},
	{0x0F12, 0x7000},
	{0x0F12, 0x0080},
	{0x0F12, 0x7000},
	{0x0F12, 0x046c},
	{0x0F12, 0x7000},
	{0x0F12, 0x0468},
	{0x0F12, 0x7000},
	{0x0F12, 0x1100},
	{0x0F12, 0xd000},
	{0x0F12, 0x01b8},
	{0x0F12, 0x7000},
	{0x0F12, 0x044e},
	{0x0F12, 0x7000},
	{0x0F12, 0x0450},
	{0x0F12, 0x7000},
	{0x0F12, 0x4778},
	{0x0F12, 0x46c0},
	{0x0F12, 0xc000},
	{0x0F12, 0xe59f},
	{0x0F12, 0xff1c},
	{0x0F12, 0xe12f},
	{0x0F12, 0x9ce7},
	{0x0F12, 0x0000},
	{0x0F12, 0x4778},
	{0x0F12, 0x46c0},
	{0x0F12, 0xf004},
	{0x0F12, 0xe51f},
	{0x0F12, 0x9fb8},
	{0x0F12, 0x0000},
	{0x0F12, 0x4778},
	{0x0F12, 0x46c0},
	{0x0F12, 0xc000},
	{0x0F12, 0xe59f},
	{0x0F12, 0xff1c},
	{0x0F12, 0xe12f},
	{0x0F12, 0x88df},
	{0x0F12, 0x0000},
	{0x0F12, 0x4778},
	{0x0F12, 0x46c0},
	{0x0F12, 0xc000},
	{0x0F12, 0xe59f},
	{0x0F12, 0xff1c},
	{0x0F12, 0xe12f},
	{0x0F12, 0x275d},
	{0x0F12, 0x0000},
	{0x0F12, 0x4778},
	{0x0F12, 0x46c0},
	{0x0F12, 0xc000},
	{0x0F12, 0xe59f},
	{0x0F12, 0xff1c},
	{0x0F12, 0xe12f},
	{0x0F12, 0x1ed3},
	{0x0F12, 0x0000},
	{0x0F12, 0x4778},
	{0x0F12, 0x46c0},
	{0x0F12, 0xc000},
	{0x0F12, 0xe59f},
	{0x0F12, 0xff1c},
	{0x0F12, 0xe12f},
	{0x0F12, 0x26f9},
	{0x0F12, 0x0000},
	{0x0F12, 0x4778},
	{0x0F12, 0x46c0},
	{0x0F12, 0xc000},
	{0x0F12, 0xe59f},
	{0x0F12, 0xff1c},
	{0x0F12, 0xe12f},
	{0x0F12, 0x4027},
	{0x0F12, 0x0000},
	{0x0F12, 0x4778},
	{0x0F12, 0x46c0},
	{0x0F12, 0xc000},
	{0x0F12, 0xe59f},
	{0x0F12, 0xff1c},
	{0x0F12, 0xe12f},
	{0x0F12, 0x9f03},
	{0x0F12, 0x0000},
	{0x0F12, 0x4778},
	{0x0F12, 0x46c0},
	{0x0F12, 0xf004},
	{0x0F12, 0xe51f},
	{0x0F12, 0xa144},
	{0x0F12, 0x0000},
	{0x0F12, 0x4778},
	{0x0F12, 0x46c0},
	{0x0F12, 0xc000},
	{0x0F12, 0xe59f},
	{0x0F12, 0xff1c},
	{0x0F12, 0xe12f},
	{0x0F12, 0x285f},
	{0x0F12, 0x0000},
	{0x0F12, 0x4778},
	{0x0F12, 0x46c0},
	{0x0F12, 0xc000},
	{0x0F12, 0xe59f},
	{0x0F12, 0xff1c},
	{0x0F12, 0xe12f},
	{0x0F12, 0x2001},
	{0x0F12, 0x0000},
	{0x0F12, 0x0000},
	{0x0F12, 0x0000},
	{0x0F12, 0xe848},
	{0x0F12, 0x0001},
	{0x0F12, 0xe848},
	{0x0F12, 0x0001},
	{0x0F12, 0x0500},
	{0x0F12, 0x0000},
	{0x0F12, 0x0000},
	{0x0F12, 0x0000},
	/* Set host interrupt so main start run */
	{0x1000, 0x0001},
	{REG_DELAY, 10},	/* Wait10mSec */
	/* Finishing user init script */
	{REG_TC_DBG_AutoAlgEnBits, 0x005F},
	{REG_SF_USER_FlickerQuant, 0x0002},
	{REG_SF_USER_FlickerQuantChanged, 0x0001},
};

/* Clock configuration: should be expanded to various clock input */
static const struct s5k6aa_reg s5k6aa_mclk_24mhz[] = {
	{0x002A, REG_TC_IPRM_InClockLSBs},
	{0x0F12, 0x5DC0},	/* input=24MHz */
	{0x002A, REG_TC_IPRM_InClockMSBs},
	{0x0F12, 0x0000},
	{0x002A, REG_TC_IPRM_UseNPviClocks},
	{0x0F12, 0x0001},	/* 1 PLL configurations */
	{0x002A, REG_TC_IPRM_OpClk4KHz_0},
	{0x0F12, 0x1770},	/* 1st system CLK 24MHz */
	{0x002A, REG_TC_IPRM_MinOutRate4KHz_0},
	{0x0F12, 0x2EE0},
	{0x002A, REG_TC_IPRM_MaxOutRate4KHz_0},
	{0x0F12, 0x2EE0},
	{0x002A, REG_TC_IPRM_InitParamsUpdated},
	{0x0F12, 0x0001},	/* FIXME:Preset number ??? */
	{REG_DELAY, 100},	/* 100ms delay */
};

/*
 * Image resolution configuration
 * This SoC supports 5 presets for each preview and capture resolutions
 */
/* Preview configuration preset #0 */
static const struct s5k6aa_reg s5k6aa_preview_vga[] = {
	/* Resolution, Pixel format */
	{0x002A, REG_0TC_PCFG_usWidth},
	{0x0F12, 0x0280},	/* 640 */
	{0x002A, REG_0TC_PCFG_usHeight},
	{0x0F12, 0x01E0},	/* 480 */
	{0x002A, REG_0TC_PCFG_Format},
	{0x0F12, 0x0005},	/* YUV */

	/* PLL config */
	{0x002A, REG_0TC_PCFG_uClockInd},
	{0x0F12, 0x0000},

	/* PCLK max */
	{0x002A, REG_0TC_PCFG_usMaxOut4KHzRate},
	{0x0F12, 0x2EE0},
	/* PCLK min */
	{0x002A, REG_0TC_PCFG_usMinOut4KHzRate},
	{0x0F12, 0x2EE0},
	{0x002A, REG_0TC_PCFG_PVIMask},
	{0x0F12, 0x0042},

	/* Frame rate */
	{0x002A, REG_0TC_PCFG_FrRateQualityType},
	{0x0F12, TC_FRVSQ_BEST_QUALITY},	/* binning disabled */
	{0x002A, REG_0TC_PCFG_usFrTimeType},
	{0x0F12, TC_FR_TIME_FIXED_ACCURATE},	/* Fixed */
	{0x002A, REG_0TC_PCFG_uPrevMirror},
	{0x0F12, 0x0000},	/* no mirror */
	/* max frame time : 15fps 029a */
	{0x002A, REG_0TC_PCFG_usMaxFrTimeMsecMult10},
	{0x0F12, 0x0535},
	{0x002A, REG_0TC_PCFG_usMinFrTimeMsecMult10},
	{0x0F12, 0x0000},
};
/* Preview configuration preset #1 */
/* Preview configuration preset #2 */
/* Preview configuration preset #3 */
/* Preview configuration preset #4 */

/* Capture configuration preset #0 */
/* Capture configuration preset #1 */
/* Capture configuration preset #2 */
/* Capture configuration preset #3 */
/* Capture configuration preset #4 */

/*
 * EV bias
 */

static const struct s5k6aa_reg s5k6aa_ev_m6[] = {
};

static const struct s5k6aa_reg s5k6aa_ev_m5[] = {
};

static const struct s5k6aa_reg s5k6aa_ev_m4[] = {
};

static const struct s5k6aa_reg s5k6aa_ev_m3[] = {
};

static const struct s5k6aa_reg s5k6aa_ev_m2[] = {
};

static const struct s5k6aa_reg s5k6aa_ev_m1[] = {
};

static const struct s5k6aa_reg s5k6aa_ev_default[] = {
};

static const struct s5k6aa_reg s5k6aa_ev_p1[] = {
};

static const struct s5k6aa_reg s5k6aa_ev_p2[] = {
};

static const struct s5k6aa_reg s5k6aa_ev_p3[] = {
};

static const struct s5k6aa_reg s5k6aa_ev_p4[] = {
};

static const struct s5k6aa_reg s5k6aa_ev_p5[] = {
};

static const struct s5k6aa_reg s5k6aa_ev_p6[] = {
};

#ifdef S5K6AA_COMPLETE
/* Order of this array should be following the querymenu data */
static const unsigned char *s5k6aa_regs_ev_bias[] = {
	(unsigned char *)s5k6aa_ev_m6, (unsigned char *)s5k6aa_ev_m5,
	(unsigned char *)s5k6aa_ev_m4, (unsigned char *)s5k6aa_ev_m3,
	(unsigned char *)s5k6aa_ev_m2, (unsigned char *)s5k6aa_ev_m1,
	(unsigned char *)s5k6aa_ev_default, (unsigned char *)s5k6aa_ev_p1,
	(unsigned char *)s5k6aa_ev_p2, (unsigned char *)s5k6aa_ev_p3,
	(unsigned char *)s5k6aa_ev_p4, (unsigned char *)s5k6aa_ev_p5,
	(unsigned char *)s5k6aa_ev_p6,
};

/*
 * Auto White Balance configure
 */
static const struct s5k6aa_reg s5k6aa_awb_off[] = {
};

static const struct s5k6aa_reg s5k6aa_awb_on[] = {
};

static const unsigned char *s5k6aa_regs_awb_enable[] = {
	(unsigned char *)s5k6aa_awb_off,
	(unsigned char *)s5k6aa_awb_on,
};

/*
 * Manual White Balance (presets)
 */
static const struct s5k6aa_reg s5k6aa_wb_tungsten[] = {

};

static const struct s5k6aa_reg s5k6aa_wb_fluorescent[] = {

};

static const struct s5k6aa_reg s5k6aa_wb_sunny[] = {

};

static const struct s5k6aa_reg s5k6aa_wb_cloudy[] = {

};

/* Order of this array should be following the querymenu data */
static const unsigned char *s5k6aa_regs_wb_preset[] = {
	(unsigned char *)s5k6aa_wb_tungsten,
	(unsigned char *)s5k6aa_wb_fluorescent,
	(unsigned char *)s5k6aa_wb_sunny,
	(unsigned char *)s5k6aa_wb_cloudy,
};

/*
 * Color Effect (COLORFX)
 */
static const struct s5k6aa_reg s5k6aa_color_sepia[] = {
};

static const struct s5k6aa_reg s5k6aa_color_aqua[] = {
};

static const struct s5k6aa_reg s5k6aa_color_monochrome[] = {
};

static const struct s5k6aa_reg s5k6aa_color_negative[] = {
};

static const struct s5k6aa_reg s5k6aa_color_sketch[] = {
};

/* Order of this array should be following the querymenu data */
static const unsigned char *s5k6aa_regs_color_effect[] = {
	(unsigned char *)s5k6aa_color_sepia,
	(unsigned char *)s5k6aa_color_aqua,
	(unsigned char *)s5k6aa_color_monochrome,
	(unsigned char *)s5k6aa_color_negative,
	(unsigned char *)s5k6aa_color_sketch,
};

/*
 * Contrast bias
 */
static const struct s5k6aa_reg s5k6aa_contrast_m2[] = {
};

static const struct s5k6aa_reg s5k6aa_contrast_m1[] = {
};

static const struct s5k6aa_reg s5k6aa_contrast_default[] = {
};

static const struct s5k6aa_reg s5k6aa_contrast_p1[] = {
};

static const struct s5k6aa_reg s5k6aa_contrast_p2[] = {
};

static const unsigned char *s5k6aa_regs_contrast_bias[] = {
	(unsigned char *)s5k6aa_contrast_m2,
	(unsigned char *)s5k6aa_contrast_m1,
	(unsigned char *)s5k6aa_contrast_default,
	(unsigned char *)s5k6aa_contrast_p1,
	(unsigned char *)s5k6aa_contrast_p2,
};

/*
 * Saturation bias
 */
static const struct s5k6aa_reg s5k6aa_saturation_m2[] = {
};

static const struct s5k6aa_reg s5k6aa_saturation_m1[] = {
};

static const struct s5k6aa_reg s5k6aa_saturation_default[] = {
};

static const struct s5k6aa_reg s5k6aa_saturation_p1[] = {
};

static const struct s5k6aa_reg s5k6aa_saturation_p2[] = {
};

static const unsigned char *s5k6aa_regs_saturation_bias[] = {
	(unsigned char *)s5k6aa_saturation_m2,
	(unsigned char *)s5k6aa_saturation_m1,
	(unsigned char *)s5k6aa_saturation_default,
	(unsigned char *)s5k6aa_saturation_p1,
	(unsigned char *)s5k6aa_saturation_p2,
};

/*
 * Sharpness bias
 */
static const struct s5k6aa_reg s5k6aa_sharpness_m2[] = {
};

static const struct s5k6aa_reg s5k6aa_sharpness_m1[] = {
};

static const struct s5k6aa_reg s5k6aa_sharpness_default[] = {
};

static const struct s5k6aa_reg s5k6aa_sharpness_p1[] = {
};

static const struct s5k6aa_reg s5k6aa_sharpness_p2[] = {
};

static const unsigned char *s5k6aa_regs_sharpness_bias[] = {
	(unsigned char *)s5k6aa_sharpness_m2,
	(unsigned char *)s5k6aa_sharpness_m1,
	(unsigned char *)s5k6aa_sharpness_default,
	(unsigned char *)s5k6aa_sharpness_p1,
	(unsigned char *)s5k6aa_sharpness_p2,
};
#endif /* S5K6AA_COMPLETE */

#endif
