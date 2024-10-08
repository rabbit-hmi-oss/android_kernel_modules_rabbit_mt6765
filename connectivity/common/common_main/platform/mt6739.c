/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */
/*! \file
*    \brief  Declaration of library functions
*
*    Any definitions in this file will be shared among GLUE Layer and internal Driver Stack.
*/
/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

#ifdef DFT_TAG
#undef DFT_TAG
#endif
#define DFT_TAG "[WMT-CONSYS-HW]"

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/
#include <connectivity_build_in_adapter.h>

#if defined(CONFIG_MTK_CLKMGR)
#include <mtk_clkmgr.h>
#else
#include <linux/clk.h>
#endif /* defined(CONFIG_MTK_LEGACY) */
#include <linux/delay.h>
#include <linux/memblock.h>
#include <linux/platform_device.h>
#include "osal_typedef.h"
#include "mt6739.h"
#include "mtk_wcn_consys_hw.h"

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
#if IS_ENABLED(CONFIG_MTK_EMI_LEGACY_V0)
#include "soc/mediatek/emi_legacy_v0.h"
#endif
#else
#if CONSYS_EMI_MPU_SETTING
#include <mach/emi_mpu.h>
#endif
#endif

#if CONSYS_PMIC_CTRL_ENABLE
#include <linux/regulator/consumer.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
#include <linux/regulator/consumer.h>
#include <linux/mfd/mt6397/core.h>
#include <linux/mfd/mt6357/registers.h>
#include <linux/regmap.h>
#else
#include <upmu_common.h>
#endif
#endif

#ifdef CONFIG_MTK_HIBERNATION
#include <mtk_hibernate_dpm.h>
#endif

#include <linux/of_reserved_mem.h>

#if (!COMMON_KERNEL_CLK_SUPPORT)
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
#include <mtk-clkbuf-bridge.h>
#else
#if CONSYS_CLOCK_BUF_CTRL
#include <mtk_clkbuf_ctl.h>
#endif
#endif
#endif

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
static INT32 consys_clock_buffer_ctrl(MTK_WCN_BOOL enable);
static VOID consys_hw_reset_bit_set(MTK_WCN_BOOL enable);
static VOID consys_hw_spm_clk_gating_enable(VOID);
static INT32 consys_hw_power_ctrl(MTK_WCN_BOOL enable);
static INT32 consys_ahb_clock_ctrl(MTK_WCN_BOOL enable);
static INT32 polling_consys_chipid(VOID);
static VOID consys_acr_reg_setting(VOID);
static VOID consys_afe_reg_setting(VOID);
static INT32 consys_hw_vcn18_ctrl(MTK_WCN_BOOL enable);
static VOID consys_vcn28_hw_mode_ctrl(UINT32 enable);
static INT32 consys_hw_vcn28_ctrl(UINT32 enable);
static INT32 consys_hw_wifi_vcn33_ctrl(UINT32 enable);
static INT32 consys_hw_bt_vcn33_ctrl(UINT32 enable);
static UINT32 consys_soc_chipid_get(VOID);
static INT32 consys_emi_mpu_set_region_protection(VOID);
static UINT32 consys_emi_set_remapping_reg(VOID);
static INT32 bt_wifi_share_v33_spin_lock_init(VOID);
static INT32 consys_clk_get_from_dts(struct platform_device *pdev);
static INT32 consys_pmic_get_from_dts(struct platform_device *pdev);
static INT32 consys_read_irq_info_from_dts(struct platform_device *pdev, PINT32 irq_num, PUINT32 irq_flag);
static INT32 consys_read_reg_from_dts(struct platform_device *pdev);
static UINT32 consys_read_cpupcr(VOID);
static INT32 consys_poll_cpupcr_dump(UINT32 times, UINT32 sleep_ms);
static VOID force_trigger_assert_debug_pin(VOID);
static INT32 consys_co_clock_type(VOID);
static P_CONSYS_EMI_ADDR_INFO consys_soc_get_emi_phy_add(VOID);
static INT32 consys_emi_coredump_remapping(UINT8 __iomem **addr, UINT32 enable);
static INT32 consys_reset_emi_coredump(UINT8 __iomem *addr);
static UINT64 consys_get_options(VOID);
static INT32 consys_jtag_set_for_mcu(VOID);
static UINT32 consys_jtag_flag_ctrl(UINT32 enable);

#if (COMMON_KERNEL_PMIC_SUPPORT)
static INT32 consys_pmic_register_device(VOID);
static int consys_pmic_mt6357_probe(struct platform_device *pdev);
#endif

#if (COMMON_KERNEL_CLK_SUPPORT)
static MTK_WCN_BOOL consys_need_store_pdev(VOID);
static UINT32 consys_store_pdev(struct platform_device *pdev);
#endif

enum connsys_debug_cr {
	CONNSYS_CPU_CLK = 0,
	CONNSYS_BUS_CLK = 1,
	CONNSYS_DEBUG_CR1 = 2,
	CONNSYS_DEBUG_CR2 = 3,
	CONNSYS_EMI_REMAP = 4,
	CONNSYS_CR_MAX
};

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/
#if CONSYS_BT_WIFI_SHARE_V33
struct bt_wifi_v33_status gBtWifiV33;
#endif

#if (COMMON_KERNEL_CLK_SUPPORT)
static struct platform_device *connsys_pdev;
#else
/* CCF part */
static struct clk *clk_scp_conn_main;	/*ctrl conn_power_on/off */
#endif

/* PMIC part */
#if CONSYS_PMIC_CTRL_ENABLE
#if !defined(CONFIG_MTK_LEGACY)
static struct regulator *reg_VCN18;
static struct regulator *reg_VCN28;
static struct regulator *reg_VCN33_BT;
static struct regulator *reg_VCN33_WIFI;
#endif
#if (COMMON_KERNEL_PMIC_SUPPORT)
struct regmap *g_regmap_mt6357;
#endif
#endif

static EMI_CTRL_STATE_OFFSET mtk_wcn_emi_state_off = {
	.emi_apmem_ctrl_state = EXP_APMEM_CTRL_STATE,
	.emi_apmem_ctrl_host_sync_state = EXP_APMEM_CTRL_HOST_SYNC_STATE,
	.emi_apmem_ctrl_host_sync_num = EXP_APMEM_CTRL_HOST_SYNC_NUM,
	.emi_apmem_ctrl_chip_sync_state = EXP_APMEM_CTRL_CHIP_SYNC_STATE,
	.emi_apmem_ctrl_chip_sync_num = EXP_APMEM_CTRL_CHIP_SYNC_NUM,
	.emi_apmem_ctrl_chip_sync_addr = EXP_APMEM_CTRL_CHIP_SYNC_ADDR,
	.emi_apmem_ctrl_chip_sync_len = EXP_APMEM_CTRL_CHIP_SYNC_LEN,
	.emi_apmem_ctrl_chip_print_buff_start = EXP_APMEM_CTRL_CHIP_PRINT_BUFF_START,
	.emi_apmem_ctrl_chip_print_buff_len = EXP_APMEM_CTRL_CHIP_PRINT_BUFF_LEN,
	.emi_apmem_ctrl_chip_print_buff_idx = EXP_APMEM_CTRL_CHIP_PRINT_BUFF_IDX,
	.emi_apmem_ctrl_chip_int_status = EXP_APMEM_CTRL_CHIP_INT_STATUS,
	.emi_apmem_ctrl_chip_paded_dump_end = EXP_APMEM_CTRL_CHIP_PAGED_DUMP_END,
	.emi_apmem_ctrl_host_outband_assert_w1 = EXP_APMEM_CTRL_HOST_OUTBAND_ASSERT_W1,
	.emi_apmem_ctrl_chip_page_dump_num = EXP_APMEM_CTRL_CHIP_PAGE_DUMP_NUM,
	.emi_apmem_ctrl_assert_flag = EXP_APMEM_CTRL_ASSERT_FLAG,
};

static CONSYS_EMI_ADDR_INFO mtk_wcn_emi_addr_info = {
	.emi_phy_addr = CONSYS_EMI_FW_PHY_BASE,
	.paged_trace_off = CONSYS_EMI_PAGED_TRACE_OFFSET,
	.paged_dump_off = CONSYS_EMI_PAGED_DUMP_OFFSET,
	.full_dump_off = CONSYS_EMI_FULL_DUMP_OFFSET,
	.p_ecso = &mtk_wcn_emi_state_off,
	.emi_core_dump_offset = CONSYS_EMI_COREDUMP_OFFSET,
};

WMT_CONSYS_IC_OPS consys_ic_ops_mt6739 = {
	.consys_ic_clock_buffer_ctrl = consys_clock_buffer_ctrl,
	.consys_ic_hw_reset_bit_set = consys_hw_reset_bit_set,
	.consys_ic_hw_spm_clk_gating_enable = consys_hw_spm_clk_gating_enable,
	.consys_ic_hw_power_ctrl = consys_hw_power_ctrl,
	.consys_ic_ahb_clock_ctrl = consys_ahb_clock_ctrl,
	.polling_consys_ic_chipid = polling_consys_chipid,
	.consys_ic_acr_reg_setting = consys_acr_reg_setting,
	.consys_ic_afe_reg_setting = consys_afe_reg_setting,
	.consys_ic_hw_vcn18_ctrl = consys_hw_vcn18_ctrl,
	.consys_ic_vcn28_hw_mode_ctrl = consys_vcn28_hw_mode_ctrl,
	.consys_ic_hw_vcn28_ctrl = consys_hw_vcn28_ctrl,
	.consys_ic_hw_wifi_vcn33_ctrl = consys_hw_wifi_vcn33_ctrl,
	.consys_ic_hw_bt_vcn33_ctrl = consys_hw_bt_vcn33_ctrl,
	.consys_ic_soc_chipid_get = consys_soc_chipid_get,
	.consys_ic_emi_mpu_set_region_protection = consys_emi_mpu_set_region_protection,
	.consys_ic_emi_set_remapping_reg = consys_emi_set_remapping_reg,
	.ic_bt_wifi_share_v33_spin_lock_init = bt_wifi_share_v33_spin_lock_init,
	.consys_ic_clk_get_from_dts = consys_clk_get_from_dts,
	.consys_ic_pmic_get_from_dts = consys_pmic_get_from_dts,
	.consys_ic_read_irq_info_from_dts = consys_read_irq_info_from_dts,
	.consys_ic_read_reg_from_dts = consys_read_reg_from_dts,
	.consys_ic_read_cpupcr = consys_read_cpupcr,
	.consys_ic_poll_cpupcr_dump = consys_poll_cpupcr_dump,
	.ic_force_trigger_assert_debug_pin = force_trigger_assert_debug_pin,
	.consys_ic_co_clock_type = consys_co_clock_type,
	.consys_ic_soc_get_emi_phy_add = consys_soc_get_emi_phy_add,
	.consys_ic_emi_coredump_remapping = consys_emi_coredump_remapping,
	.consys_ic_reset_emi_coredump = consys_reset_emi_coredump,
	.consys_ic_get_options = consys_get_options,
	.consys_ic_jtag_set_for_mcu = consys_jtag_set_for_mcu,
	.consys_ic_jtag_flag_ctrl = consys_jtag_flag_ctrl,

#if COMMON_KERNEL_PMIC_SUPPORT
	.consys_ic_pmic_register_device = consys_pmic_register_device,
#endif
#if (COMMON_KERNEL_CLK_SUPPORT)
	.consys_ic_need_store_pdev = consys_need_store_pdev,
	.consys_ic_store_pdev = consys_store_pdev,
#endif
};

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/
static UINT32 gJtagCtrl;

#if (COMMON_KERNEL_PMIC_SUPPORT)
#ifdef CONFIG_OF
const struct of_device_id consys_pmic_mt6357_of_ids[] = {
	{.compatible = "mediatek,mt6357-consys",},
	{}
};
#endif

static struct platform_driver consys_pmic_mt6357_dev_drv = {
	.probe = consys_pmic_mt6357_probe,
	.driver = {
		.name = "mt6357-consys",
#ifdef CONFIG_OF
		.of_match_table = consys_pmic_mt6357_of_ids,
#endif
		},
};
#endif

#if CONSYS_ENALBE_SET_JTAG
#define JTAG_ADDR1_BASE 0x10002000
PINT8 jtag_addr1 = (PINT8)JTAG_ADDR1_BASE;
#define JTAG1_REG_WRITE(addr, value) \
	writel(value, ((PUINT32)(jtag_addr1+(addr-JTAG_ADDR1_BASE))))
#define JTAG1_REG_READ(addr) \
	readl(((PUINT32)(jtag_addr1+(addr-JTAG_ADDR1_BASE))))
#endif

static INT32 consys_jtag_set_for_mcu(VOID)
{
#if CONSYS_ENALBE_SET_JTAG

	if (gJtagCtrl) {
#if 0
		INT32 iRet = -1;

		WMT_PLAT_PR_INFO("WCN jtag_set_for_mcu start...\n");
		jtag_addr1 = ioremap(JTAG_ADDR1_BASE, 0x5000);
		if (jtag_addr1 == 0) {
			WMT_PLAT_PR_ERR("remap jtag_addr1 fail!\n");
			return iRet;
		}
		WMT_PLAT_PR_INFO("jtag_addr1 = 0x%p\n", jtag_addr1);

		JTAG1_REG_WRITE(0x100053c4, 0x11111100);
		JTAG1_REG_WRITE(0x100053d4, 0x00111111);

		/*Enable IES of all pins */
		JTAG1_REG_WRITE(0x10002014, 0x00000003);
		JTAG1_REG_WRITE(0x10005334, 0x55000000);
		JTAG1_REG_WRITE(0x10005344, 0x00555555);
		JTAG1_REG_WRITE(0x10005008, 0xc0000000);
		JTAG1_REG_WRITE(0x10005018, 0x0000000d);
		JTAG1_REG_WRITE(0x10005014, 0x00000032);
		JTAG1_REG_WRITE(0x100020a4, 0x000000ff);
		JTAG1_REG_WRITE(0x100020d4, 0x000000b4);

		JTAG1_REG_WRITE(0x100020d8, 0x0000004b);

		WMT_PLAT_PR_INFO("WCN jtag set for mcu start...\n");
		kal_uint32 tmp = 0;
		kal_int32 addr = 0;
		kal_int32 remap_addr1 = 0;
		kal_int32 remap_addr2 = 0;

		remap_addr1 = ioremap(JTAG_ADDR1_BASE, 0x1000);
		if (remap_addr1 == 0) {
			WMT_PLAT_PR_ERR("remap jtag_addr1 fail!\n");
			return -1;
		}

		remap_addr2 = ioremap(JTAG_ADDR2_BASE, 0x100);
		if (remap_addr2 == 0) {
			WMT_PLAT_PR_ERR("remap jtag_addr2 fail!\n");
			return -1;
		}

		/*Pinmux setting for MT6625 I/F */
		addr = remap_addr1 + 0x03C0;
		tmp = DRV_Reg32(addr);
		tmp = tmp & 0xff;
		tmp = tmp | 0x11111100;
		DRV_WriteReg32(addr, tmp);
		WMT_PLAT_PR_INFO("(RegAddr, RegVal):(0x%08x, 0x%08x)", addr, DRV_Reg32(addr));

		addr = remap_addr1 + 0x03D0;
		tmp = DRV_Reg32(addr);
		tmp = tmp & 0xff000000;
		tmp = tmp | 0x00111111;
		DRV_WriteReg32(addr, tmp);
		WMT_PLAT_PR_INFO("(RegAddr, RegVal):(0x%08x, 0x%08x)", addr, DRV_Reg32(addr));

		/*AP GPIO Setting 1 <default use> */
		/*Enable IES */
		/* addr = 0x10002014; */
		addr = remap_addr2 + 0x0014;
		tmp = 0x00000003;
		DRV_WriteReg32(addr, tmp);
		WMT_PLAT_PR_INFO("(RegAddr, RegVal):(0x%08x, 0x%08x)", addr, DRV_Reg32(addr));
		/*GPIO mode setting */
		/* addr = 0x10005334; */
		addr = remap_addr1 + 0x0334;
		tmp = 0x55000000;
		DRV_WriteReg32(addr, tmp);
		WMT_PLAT_PR_INFO("(RegAddr, RegVal):(0x%08x, 0x%08x)", addr, DRV_Reg32(addr));

		/* addr = 0x10005344; */
		addr = remap_addr1 + 0x0344;
		tmp = 0x00555555;
		DRV_WriteReg32(addr, tmp);
		WMT_PLAT_PR_INFO("(RegAddr, RegVal):(0x%08x, 0x%08x)", addr, DRV_Reg32(addr));
		/*GPIO direction control */
		/* addr = 0x10005008; */
		addr = remap_addr1 + 0x0008;
		tmp = 0xc0000000;
		DRV_WriteReg32(addr, tmp);
		WMT_PLAT_PR_INFO("(RegAddr, RegVal):(0x%08x, 0x%08x)", addr, DRV_Reg32(addr));

		/* addr = 0x10005018; */
		addr = remap_addr1 + 0x0018;
		tmp = 0x0000000d;
		DRV_WriteReg32(addr, tmp);
		WMT_PLAT_PR_INFO("(RegAddr, RegVal):(0x%08x, 0x%08x)", addr, DRV_Reg32(addr));

		/* addr = 0x10005014; */
		addr = remap_addr1 + 0x0014;
		tmp = 0x00000032;
		DRV_WriteReg32(addr, tmp);
		WMT_PLAT_PR_INFO("(RegAddr, RegVal):(0x%08x, 0x%08x)", addr, DRV_Reg32(addr));

		/*PULL Enable */
		/* addr = 0x100020a4; */
		addr = remap_addr2 + 0x00a4;
		tmp = 0x000000ff;
		DRV_WriteReg32(addr, tmp);
		WMT_PLAT_PR_INFO("(RegAddr, RegVal):(0x%08x, 0x%08x)", addr, DRV_Reg32(addr));

		/*PULL select enable */
		/* addr = 0x100020d4; */
		addr = remap_addr2 + 0x00d4;
		tmp = 0x000000b4;
		DRV_WriteReg32(addr, tmp);
		WMT_PLAT_PR_INFO("(RegAddr, RegVal):(0x%08x, 0x%08x)", addr, DRV_Reg32(addr));

		/* addr = 0x100020d8; */
		addr = remap_addr2 + 0x00d8;
		tmp = 0x0000004b;
		DRV_WriteReg32(addr, tmp);
		WMT_PLAT_PR_INFO("(RegAddr, RegVal):(0x%08x, 0x%08x)", addr, DRV_Reg32(addr));
#endif
	}
#endif
	return 0;
}

static UINT32 consys_jtag_flag_ctrl(UINT32 enable)
{
	WMT_PLAT_PR_INFO("%s jtag set for MCU\n", enable ? "enable" : "disable");
	gJtagCtrl = enable;
	return 0;
}

#if COMMON_KERNEL_PMIC_SUPPORT
static INT32 consys_pmic_register_device(VOID)
{
	int ret;

	ret = platform_driver_register(&consys_pmic_mt6357_dev_drv);
	if (ret)
		WMT_PLAT_PR_INFO("WMT pmic mt6357 driver registered failed(%d)\n", ret);
	else
		WMT_PLAT_PR_INFO("%s mt6357 ok.\n", __func__);

	return 0;
}
#endif

#if CONSYS_PMIC_CTRL_ENABLE
#if (COMMON_KERNEL_PMIC_SUPPORT)
static int consys_pmic_mt6357_probe(struct platform_device *pdev)
{
	struct mt6397_chip *mt6397 = dev_get_drvdata(pdev->dev.parent);

	if (!mt6397) {
		WMT_PLAT_PR_INFO("%s mt6397 is NULL\n", __func__);
		return -1;
	}

	g_regmap_mt6357 = mt6397->regmap;
	if (!g_regmap_mt6357) {
		WMT_PLAT_PR_INFO("%s mt6397->regmap is NULL\n", __func__);
		return -1;
	}
	WMT_PLAT_PR_INFO("%s get regmap_mt6357 success!!\n", __func__);
	return 0;
}
#endif
#endif

static INT32 consys_clk_get_from_dts(struct platform_device *pdev)
{
#if (!COMMON_KERNEL_CLK_SUPPORT)
#ifdef CONFIG_OF		/*use DT */
#if !defined(CONFIG_MTK_CLKMGR)
	clk_scp_conn_main = devm_clk_get(&pdev->dev, "conn");
	if (IS_ERR(clk_scp_conn_main)) {
		WMT_PLAT_PR_ERR("[CCF]cannot get clk_scp_conn_main clock.\n");
		return PTR_ERR(clk_scp_conn_main);
	}
	WMT_PLAT_PR_DBG("[CCF]clk_scp_conn_main=%p\n", clk_scp_conn_main);
#if 0
	clk_infra_conn_main = devm_clk_get(&pdev->dev, "infra-sys-conn-main");
	if (IS_ERR(clk_infra_conn_main)) {
		WMT_PLAT_PR_ERR("[CCF]cannot get clk_infra_conn_main clock.\n");
		return PTR_ERR(clk_infra_conn_main);
	}
	WMT_PLAT_PR_DBG("[CCF]clk_infra_conn_main=%p\n", clk_infra_conn_main);
#endif

#endif /* !defined(CONFIG_MTK_LEGACY) */
#endif
#endif
	return 0;
}

static INT32 consys_pmic_get_from_dts(struct platform_device *pdev)
{
#ifdef CONFIG_OF		/*use DT */
#if CONSYS_PMIC_CTRL_ENABLE
#if !defined(CONFIG_MTK_LEGACY)
	reg_VCN18 = regulator_get(&pdev->dev, "vcn18");
	if (!reg_VCN18)
		WMT_PLAT_PR_ERR("Regulator_get VCN_1V8 fail\n");
	reg_VCN28 = regulator_get(&pdev->dev, "vcn28");
	if (!reg_VCN28)
		WMT_PLAT_PR_ERR("Regulator_get VCN_2V8 fail\n");
	reg_VCN33_BT = regulator_get(&pdev->dev, "vcn33_bt");
	if (!reg_VCN33_BT)
		WMT_PLAT_PR_ERR("Regulator_get VCN33_BT fail\n");
	reg_VCN33_WIFI = regulator_get(&pdev->dev, "vcn33_wifi");
	if (!reg_VCN33_WIFI)
		WMT_PLAT_PR_ERR("Regulator_get VCN33_WIFI fail\n");
#endif
#endif
#endif
	return 0;
}

static INT32 consys_co_clock_type(VOID)
{
#if COMMON_KERNEL_PMIC_SUPPORT
	return 0;
#else
	UINT32 retval = 0;
	UINT32 back_up = 0;
	UINT32 co_clock_type = 0;

	/* co-clock auto detection:backup cw15,write cw15,read cw16,restore cw15, */
	KERNEL_pmic_read_interface(PMIC_DCXO_CW15, &retval, 0xFFFF, 0);
	back_up = retval;
	KERNEL_pmic_config_interface(PMIC_DCXO_CW15, PMIC_DCXO_CW15_VAL, 0xFFFF, 0);
	KERNEL_pmic_read_interface(PMIC_DCXO_CW16, &retval, 0xFFFF, 0);
	KERNEL_pmic_config_interface(PMIC_DCXO_CW15, back_up, 0xFFFF, 0);
	if ((retval & AP_CONSYS_NOCO_CLOCK_BITA) || (retval & AP_CONSYS_NOCO_CLOCK_BITB)) {
		co_clock_type = 0;
		WMT_PLAT_PR_INFO("pmic_register_val = 0x%x, co_clock_type = %d,TCXO mode\n", retval, co_clock_type);
	} else if ((retval & AP_CONSYS_CO_CLOCK_BITA) || (retval & AP_CONSYS_CO_CLOCK_BITB)) {
		co_clock_type = 1;
		WMT_PLAT_PR_INFO("pmic_register_val = 0x%x, co_clock_type = %d,co-TSX mode\n", retval, co_clock_type);
	}
	return co_clock_type;
#endif
}

static INT32 consys_clock_buffer_ctrl(MTK_WCN_BOOL enable)
{
#if (!COMMON_KERNEL_CLK_SUPPORT)
#if CONSYS_CLOCK_BUF_CTRL
	if (enable == MTK_WCN_BOOL_TRUE)
		KERNEL_clk_buf_ctrl(CLK_BUF_CONN, 1);
	else if (enable == MTK_WCN_BOOL_FALSE)
		KERNEL_clk_buf_ctrl(CLK_BUF_CONN, 0);
#endif
#endif
	return 0;
}

static VOID consys_hw_reset_bit_set(MTK_WCN_BOOL enable)
{
#ifdef CONFIG_OF		/*use DT */
	if (enable) {
		/*3.assert CONNSYS CPU SW reset  0x10007018 "[12]=1'b1  [31:24]=8'h88 (key)" */
		CONSYS_REG_WRITE((conn_reg.ap_rgu_base + CONSYS_CPU_SW_RST_OFFSET),
				CONSYS_REG_READ(conn_reg.ap_rgu_base + CONSYS_CPU_SW_RST_OFFSET) |
				CONSYS_CPU_SW_RST_BIT | CONSYS_CPU_SW_RST_CTRL_KEY);
	} else {
		/*16.deassert CONNSYS CPU SW reset 0x10007018 "[12]=1'b0 [31:24] =8'h88 (key)" */
		CONSYS_REG_WRITE(conn_reg.ap_rgu_base + CONSYS_CPU_SW_RST_OFFSET,
				(CONSYS_REG_READ(conn_reg.ap_rgu_base + CONSYS_CPU_SW_RST_OFFSET) &
				 ~CONSYS_CPU_SW_RST_BIT) | CONSYS_CPU_SW_RST_CTRL_KEY);
	}
#else
	if (enable) {
		/*3.assert CONNSYS CPU SW reset  0x10007018  "[12]=1'b1  [31:24]=8'h88 (key)" */
		CONSYS_REG_WRITE(CONSYS_CPU_SW_RST_REG,
				(CONSYS_REG_READ(CONSYS_CPU_SW_RST_REG) | CONSYS_CPU_SW_RST_BIT |
				 CONSYS_CPU_SW_RST_CTRL_KEY));
	} else {
		/*16.deassert CONNSYS CPU SW reset 0x10007018 "[12]=1'b0 [31:24] =8'h88(key)" */
		CONSYS_REG_WRITE(CONSYS_CPU_SW_RST_REG,
				(CONSYS_REG_READ(CONSYS_CPU_SW_RST_REG) & ~CONSYS_CPU_SW_RST_BIT) |
				CONSYS_CPU_SW_RST_CTRL_KEY);
	}
#endif
}

static VOID consys_hw_spm_clk_gating_enable(VOID)
{
#ifdef CONFIG_OF		/*use DT */
	/*turn on SPM clock gating enable PWRON_CONFG_EN  0x10006000  32'h0b160001 */
	/*CONSYS_REG_WRITE((conn_reg.spm_base + CONSYS_PWRON_CONFG_EN_OFFSET), CONSYS_PWRON_CONFG_EN_VALUE);*/
#else
	/*turn on SPM clock gating enable PWRON_CONFG_EN 0x10006000 32'h0b160001 */
	CONSYS_REG_WRITE(CONSYS_PWRON_CONFG_EN_REG, CONSYS_PWRON_CONFG_EN_VALUE);
#endif
}

static INT32 consys_hw_power_ctrl(MTK_WCN_BOOL enable)
{
#if CONSYS_PWR_ON_OFF_API_AVAILABLE
	INT32 iRet = -1;
#endif

	if (enable) {
#if CONSYS_PWR_ON_OFF_API_AVAILABLE
#if (COMMON_KERNEL_CLK_SUPPORT)
		iRet = pm_runtime_get_sync(&connsys_pdev->dev);
		if (iRet)
			WMT_PLAT_PR_INFO("pm_runtime_get_sync() fail(%d)\n", iRet);
		else
			WMT_PLAT_PR_INFO("pm_runtime_get_sync() CONSYS ok\n");

		iRet = device_init_wakeup(&connsys_pdev->dev, true);
		if (iRet)
			WMT_PLAT_PR_INFO("device_init_wakeup(true) fail.\n");
		else
			WMT_PLAT_PR_INFO("device_init_wakeup(true) CONSYS ok\n");
#else
#if defined(CONFIG_MTK_CLKMGR)
		iRet = conn_power_on();	/* consult clkmgr owner. */
		if (iRet)
			WMT_PLAT_PR_ERR("conn_power_on fail(%d)\n", iRet);
		WMT_PLAT_PR_DBG("conn_power_on ok\n");
#else
		iRet = clk_prepare_enable(clk_scp_conn_main);
		if (iRet)
			WMT_PLAT_PR_ERR("clk_prepare_enable(clk_scp_conn_main) fail(%d)\n", iRet);
		WMT_PLAT_PR_DBG("clk_prepare_enable(clk_scp_conn_main) ok\n");
#endif /* defined(CONFIG_MTK_LEGACY) */
#endif
#else

#ifdef CONFIG_OF		/*use DT */
		/*2.write conn_top1_pwr_on=1, power on conn_top1 0x1000632c [2]  1'b1 */
		CONSYS_REG_WRITE(conn_reg.spm_base + CONSYS_TOP1_PWR_CTRL_OFFSET,
				 CONSYS_REG_READ(conn_reg.spm_base + CONSYS_TOP1_PWR_CTRL_OFFSET) |
				 CONSYS_SPM_PWR_ON_BIT);
		/*3.read conn_top1_pwr_on_ack =1, power on ack ready 0x10006180 [1] */
		while (0 == (CONSYS_PWR_ON_ACK_BIT & CONSYS_REG_READ(conn_reg.spm_base + CONSYS_PWR_CONN_ACK_OFFSET)))
			NULL;
		/*5.write conn_top1_pwr_on_s=1, power on conn_top1 0x1000632c [3]  1'b1 */
		CONSYS_REG_WRITE(conn_reg.spm_base + CONSYS_TOP1_PWR_CTRL_OFFSET,
				 CONSYS_REG_READ(conn_reg.spm_base + CONSYS_TOP1_PWR_CTRL_OFFSET) |
				 CONSYS_SPM_PWR_ON_S_BIT);
		/*6.write conn_clk_dis=0, enable connsys clock 0x1000632c [4]  1'b0 */
		CONSYS_REG_WRITE(conn_reg.spm_base + CONSYS_TOP1_PWR_CTRL_OFFSET,
				 CONSYS_REG_READ(conn_reg.spm_base + CONSYS_TOP1_PWR_CTRL_OFFSET) &
				 ~CONSYS_CLK_CTRL_BIT);
		/*7.wait 1us    */
		udelay(1);
		/*8.read conn_top1_pwr_on_ack_s =1, power on ack ready 0x10006184 [1] */
		while (0 == (CONSYS_PWR_CONN_ACK_S_BIT &
			CONSYS_REG_READ(conn_reg.spm_base + CONSYS_PWR_CONN_ACK_S_OFFSET)))
			NULL;
		/*9.release connsys ISO, conn_top1_iso_en=0 0x1000632c [1]  1'b0 */
		CONSYS_REG_WRITE(conn_reg.spm_base + CONSYS_TOP1_PWR_CTRL_OFFSET,
				 CONSYS_REG_READ(conn_reg.spm_base + CONSYS_TOP1_PWR_CTRL_OFFSET) &
				 ~CONSYS_SPM_PWR_ISO_S_BIT);
		/*10.release SW reset of connsys, conn_ap_sw_rst_b=1  0x1000632c[0]   1'b1 */
		CONSYS_REG_WRITE(conn_reg.spm_base + CONSYS_TOP1_PWR_CTRL_OFFSET,
				 CONSYS_REG_READ(conn_reg.spm_base + CONSYS_TOP1_PWR_CTRL_OFFSET) |
				 CONSYS_SPM_PWR_RST_BIT);
		/*disable AXI BUS protect 0x10001220[13] [14] */
		CONSYS_REG_WRITE(conn_reg.topckgen_base + CONSYS_TOPAXI_PROT_EN_OFFSET,
				 CONSYS_REG_READ(conn_reg.topckgen_base + CONSYS_TOPAXI_PROT_EN_OFFSET) &
				 ~CONSYS_PROT_MASK);
		while (CONSYS_REG_READ(conn_reg.topckgen_base + CONSYS_TOPAXI_PROT_STA1_OFFSET) & CONSYS_PROT_MASK)
			NULL;
#else
		/*2.write conn_top1_pwr_on=1, power on conn_top1 0x10006280 [2]  1'b1 */
		CONSYS_REG_WRITE(CONSYS_TOP1_PWR_CTRL_REG,
				 CONSYS_REG_READ(CONSYS_TOP1_PWR_CTRL_REG) | CONSYS_SPM_PWR_ON_BIT);
		/*3.read conn_top1_pwr_on_ack =1, power on ack ready 0x1000660C [1] */
		while (0 == (CONSYS_PWR_ON_ACK_BIT & CONSYS_REG_READ(CONSYS_PWR_CONN_ACK_REG)))
			NULL;
		/*5.write conn_top1_pwr_on_s=1, power on conn_top1 0x10006280 [3]  1'b1 */
		CONSYS_REG_WRITE(CONSYS_TOP1_PWR_CTRL_REG,
				 CONSYS_REG_READ(CONSYS_TOP1_PWR_CTRL_REG) | CONSYS_SPM_PWR_ON_S_BIT);
		/*6.write conn_clk_dis=0, enable connsys clock 0x10006280 [4]  1'b0 */
		CONSYS_REG_WRITE(CONSYS_TOP1_PWR_CTRL_REG,
				 CONSYS_REG_READ(CONSYS_TOP1_PWR_CTRL_REG) & ~CONSYS_CLK_CTRL_BIT);
		/*7.wait 1us    */
		udelay(1);
		/*8.read conn_top1_pwr_on_ack_s =1, power on ack ready 0x10006610 [1]  */
		while (0 == (CONSYS_PWR_CONN_ACK_S_BIT & CONSYS_REG_READ(CONSYS_PWR_CONN_ACK_S_REG)))
			NULL;
		/*9.release connsys ISO, conn_top1_iso_en=0 0x10006280 [1]  1'b0 */
		CONSYS_REG_WRITE(CONSYS_TOP1_PWR_CTRL_REG,
				 CONSYS_REG_READ(CONSYS_TOP1_PWR_CTRL_REG) & ~CONSYS_SPM_PWR_ISO_S_BIT);
		/*10.release SW reset of connsys, conn_ap_sw_rst_b=1 0x10006280[0] 1'b1 */
		CONSYS_REG_WRITE(CONSYS_TOP1_PWR_CTRL_REG,
				 CONSYS_REG_READ(CONSYS_TOP1_PWR_CTRL_REG) | CONSYS_SPM_PWR_RST_BIT);
		/*disable AXI BUS protect */
		CONSYS_REG_WRITE(CONSYS_TOPAXI_PROT_EN, CONSYS_REG_READ(CONSYS_TOPAXI_PROT_EN) & ~CONSYS_PROT_MASK);
		while (CONSYS_REG_READ(CONSYS_TOPAXI_PROT_STA1) & CONSYS_PROT_MASK)
			NULL;
#endif /* CONFIG_OF */
#endif /* CONSYS_PWR_ON_OFF_API_AVAILABLE */
	} else {
#if CONSYS_PWR_ON_OFF_API_AVAILABLE
#if (COMMON_KERNEL_CLK_SUPPORT)
		iRet = device_init_wakeup(&connsys_pdev->dev, false);
		if (iRet)
			WMT_PLAT_PR_INFO("device_init_wakeup(false) fail.\n");
		else
			WMT_PLAT_PR_INFO("device_init_wakeup(false) CONSYS ok\n");

		iRet = pm_runtime_put_sync(&connsys_pdev->dev);
		if (iRet)
			WMT_PLAT_PR_INFO("pm_runtime_put_sync() fail.\n");
		else
			WMT_PLAT_PR_INFO("pm_runtime_put_sync() CONSYS ok\n");
#else
#if defined(CONFIG_MTK_CLKMGR)
		/*power off connsys by API (MT6582, MT6572 are different) API: conn_power_off() */
		iRet = conn_power_off();	/* consult clkmgr owner */
		if (iRet)
			WMT_PLAT_PR_ERR("conn_power_off fail(%d)\n", iRet);
		WMT_PLAT_PR_DBG("conn_power_off ok\n");
#else
		clk_disable_unprepare(clk_scp_conn_main);
		WMT_PLAT_PR_DBG("clk_disable_unprepare(clk_scp_conn_main) calling\n");
#endif /* defined(CONFIG_MTK_LEGACY) */
#endif
#else

#ifdef CONFIG_OF		/*use DT */
		{
			INT32 count = 0;

			CONSYS_REG_WRITE(conn_reg.topckgen_base + CONSYS_TOPAXI_PROT_EN_OFFSET,
					 CONSYS_REG_READ(conn_reg.topckgen_base + CONSYS_TOPAXI_PROT_EN_OFFSET) |
					 CONSYS_PROT_MASK);
			while ((CONSYS_REG_READ(conn_reg.topckgen_base + CONSYS_TOPAXI_PROT_STA1_OFFSET) &
				CONSYS_PROT_MASK) != CONSYS_PROT_MASK) {
				count++;
				if (count > 1000)
					break;
			}
		}
		/*release connsys ISO, conn_top1_iso_en=1  0x1000632c [1]  1'b1 */
		CONSYS_REG_WRITE(conn_reg.spm_base + CONSYS_TOP1_PWR_CTRL_OFFSET,
				 CONSYS_REG_READ(conn_reg.spm_base + CONSYS_TOP1_PWR_CTRL_OFFSET) |
				 CONSYS_SPM_PWR_ISO_S_BIT);
		/*assert SW reset of connsys, conn_ap_sw_rst_b=0  0x1000632c[0] 1'b0 */
		CONSYS_REG_WRITE(conn_reg.spm_base + CONSYS_TOP1_PWR_CTRL_OFFSET,
				 CONSYS_REG_READ(conn_reg.spm_base + CONSYS_TOP1_PWR_CTRL_OFFSET) &
				 ~CONSYS_SPM_PWR_RST_BIT);
		/*write conn_clk_dis=1, disable connsys clock  0x1000632c [4]  1'b1 */
		CONSYS_REG_WRITE(conn_reg.spm_base + CONSYS_TOP1_PWR_CTRL_OFFSET,
				 CONSYS_REG_READ(conn_reg.spm_base + CONSYS_TOP1_PWR_CTRL_OFFSET) |
				 CONSYS_CLK_CTRL_BIT);
		/*wait 1us      */
		udelay(1);
		/*write conn_top1_pwr_on=0, power off conn_top1 0x1000632c [3:2] 2'b00 */
		CONSYS_REG_WRITE(conn_reg.spm_base + CONSYS_TOP1_PWR_CTRL_OFFSET,
				 CONSYS_REG_READ(conn_reg.spm_base + CONSYS_TOP1_PWR_CTRL_OFFSET) &
				 ~(CONSYS_SPM_PWR_ON_BIT | CONSYS_SPM_PWR_ON_S_BIT));
#else
		{
			INT32 count = 0;

			CONSYS_REG_WRITE(CONSYS_TOPAXI_PROT_EN,
					 CONSYS_REG_READ(CONSYS_TOPAXI_PROT_EN) | CONSYS_PROT_MASK);
			while ((CONSYS_REG_READ(CONSYS_TOPAXI_PROT_STA1) & CONSYS_PROT_MASK) != CONSYS_PROT_MASK) {
				count++;
				if (count > 1000)
					break;
			}

		}
		/*release connsys ISO, conn_top1_iso_en=1 0x1000632c [1]  1'b1 */
		CONSYS_REG_WRITE(CONSYS_TOP1_PWR_CTRL_REG,
				 CONSYS_REG_READ(CONSYS_TOP1_PWR_CTRL_REG) | CONSYS_SPM_PWR_ISO_S_BIT);
		/*assert SW reset of connsys, conn_ap_sw_rst_b=0 0x1000632c[0] 1'b0  */
		CONSYS_REG_WRITE(CONSYS_TOP1_PWR_CTRL_REG,
				 CONSYS_REG_READ(CONSYS_TOP1_PWR_CTRL_REG) & ~CONSYS_SPM_PWR_RST_BIT);
		/*write conn_clk_dis=1, disable connsys clock 0x1000632c [4]  1'b1 */
		CONSYS_REG_WRITE(CONSYS_TOP1_PWR_CTRL_REG,
				 CONSYS_REG_READ(CONSYS_TOP1_PWR_CTRL_REG) | CONSYS_CLK_CTRL_BIT);
		/*wait 1us      */
		udelay(1);
		/*write conn_top1_pwr_on=0, power off conn_top1 0x1000632c [3:2] 2'b00 */
		CONSYS_REG_WRITE(CONSYS_TOP1_PWR_CTRL_REG, CONSYS_REG_READ(CONSYS_TOP1_PWR_CTRL_REG) &
				~(CONSYS_SPM_PWR_ON_BIT | CONSYS_SPM_PWR_ON_S_BIT));
#endif /* CONFIG_OF */
#endif /* CONSYS_PWR_ON_OFF_API_AVAILABLE */
	}

#if CONSYS_PWR_ON_OFF_API_AVAILABLE
	return iRet;
#else
	return 0;
#endif
}

static INT32 consys_ahb_clock_ctrl(MTK_WCN_BOOL enable)
{
	return 0;
}

static INT32 polling_consys_chipid(VOID)
{
	UINT32 retry = 10;
	UINT32 consysHwChipId = 0;

#ifdef CONFIG_OF		/*use DT */
	/*12.poll CONNSYS CHIP ID until chipid is returned  0x18070008 */
	while (retry-- > 0) {
		consysHwChipId = CONSYS_REG_READ(conn_reg.mcu_base + CONSYS_CHIP_ID_OFFSET);
		if (consysHwChipId == 0x0699) {
			WMT_PLAT_PR_INFO("retry(%d)consys chipId(0x%08x)\n", retry, consysHwChipId);
			break;
		}
		msleep(20);
	}

	if ((retry == 0) || (consysHwChipId == 0)) {
		WMT_PLAT_PR_ERR("Maybe has a consys power on issue,(0x%08x)\n", consysHwChipId);
		WMT_PLAT_PR_INFO("reg dump:CONSYS_CPU_SW_RST_REG(0x%x)\n",
				CONSYS_REG_READ(conn_reg.ap_rgu_base + CONSYS_CPU_SW_RST_OFFSET));
		WMT_PLAT_PR_INFO("reg dump:CONSYS_PWR_CONN_ACK_REG(0x%x)\n",
				CONSYS_REG_READ(conn_reg.spm_base + CONSYS_PWR_CONN_ACK_OFFSET));
		WMT_PLAT_PR_INFO("reg dump:CONSYS_PWR_CONN_ACK_S_REG(0x%x)\n",
				CONSYS_REG_READ(conn_reg.spm_base + CONSYS_PWR_CONN_ACK_S_OFFSET));
		WMT_PLAT_PR_INFO("reg dump:CONSYS_TOP1_PWR_CTRL_REG(0x%x)\n",
				CONSYS_REG_READ(conn_reg.spm_base + CONSYS_TOP1_PWR_CTRL_OFFSET));
	}
#else
	/*12.poll CONNSYS CHIP ID until 6739 is returned 0x18070008 32'h6739 */
	while (retry-- > 0) {
		WMT_PLAT_PR_DBG("CONSYS_CHIP_ID_REG(0x%08x)", CONSYS_REG_READ(CONSYS_CHIP_ID_REG));
		consysHwChipId = CONSYS_REG_READ(CONSYS_CHIP_ID_REG);
		if (consysHwChipId == 0x0699) {
			WMT_PLAT_PR_INFO("retry(%d)consys chipId(0x%08x)\n", retry, consysHwChipId);
			break;
		}
		msleep(20);
	}

	if ((retry == 0) || (consysHwChipId == 0)) {
		WMT_PLAT_PR_ERR("Maybe has a consys power on issue,(0x%08x)\n", consysHwChipId);
		WMT_PLAT_PR_INFO("reg dump:CONSYS_CPU_SW_RST_REG(0x%x)\n",
				CONSYS_REG_READ(CONSYS_CPU_SW_RST_REG));
		WMT_PLAT_PR_INFO("reg dump:CONSYS_PWR_CONN_ACK_REG(0x%x)\n",
				CONSYS_REG_READ(CONSYS_PWR_CONN_ACK_REG));
		WMT_PLAT_PR_INFO("reg dump:CONSYS_PWR_CONN_ACK_S_REG(0x%x)\n",
				CONSYS_REG_READ(CONSYS_PWR_CONN_ACK_S_REG));
		WMT_PLAT_PR_INFO("reg dump:CONSYS_TOP1_PWR_CTRL_REG(0x%x)\n",
				CONSYS_REG_READ(CONSYS_TOP1_PWR_CTRL_REG));
	}
#endif
	return 0;
}

static VOID consys_acr_reg_setting(VOID)
{
	/*
	 *14.write 1 to conn_mcu_confg ACR[1] if real speed MBIST
	 *(default write "1") ACR 0x18070110[18] 1'b1
	 *if this bit is 0, HW will do memory auto test under low CPU frequence (26M Hz)
	 *if this bit is 0, HW will do memory auto test under high CPU frequence(138M Hz)
	 *inclulding low CPU frequence
	 */
#ifdef CONFIG_OF		/*use DT */
	CONSYS_REG_WRITE(conn_reg.mcu_base + CONSYS_MCU_CFG_ACR_OFFSET,
			CONSYS_REG_READ(conn_reg.mcu_base + CONSYS_MCU_CFG_ACR_OFFSET) |
			CONSYS_MCU_CFG_ACR_MBIST_BIT);
#else
	CONSYS_REG_WRITE(CONSYS_MCU_CFG_ACR_REG,
			CONSYS_REG_READ(CONSYS_MCU_CFG_ACR_REG) | CONSYS_MCU_CFG_ACR_MBIST_BIT);
#endif
}

static VOID consys_afe_reg_setting(VOID)
{
#if CONSYS_AFE_REG_SETTING
	UINT8 *consys_afe_reg_base = NULL;
	UINT8 i = 0;

	/*15.default no need,update ANA_WBG(AFE) CR if needed, CONSYS_AFE_REG */
	consys_afe_reg_base = ioremap(CONSYS_AFE_REG_BASE, 0x100);
	if (consys_afe_reg_base) {
		CONSYS_REG_WRITE(consys_afe_reg_base + CONSYS_AFE_REG_WBG_AFE_01_OFFSET,
				CONSYS_AFE_REG_WBG_AFE_01_VALUE);
		CONSYS_REG_WRITE(consys_afe_reg_base + CONSYS_AFE_REG_WBG_PLL_03_OFFSET,
				CONSYS_AFE_REG_WBG_PLL_03_VALUE);
		CONSYS_REG_WRITE(consys_afe_reg_base + CONSYS_AFE_REG_WBG_PLL_05_OFFSET,
				CONSYS_AFE_REG_WBG_PLL_05_VALUE);
		CONSYS_REG_WRITE(consys_afe_reg_base + CONSYS_AFE_REG_WBG_WB_TX_01_OFFSET,
				CONSYS_AFE_REG_WBG_WB_TX_01_VALUE);

		WMT_PLAT_PR_DBG("Dump AFE register\n");
		for (i = 0; i < 64; i++) {
			WMT_PLAT_PR_DBG("reg:0x%08x|val:0x%08x\n", CONSYS_AFE_REG_BASE + 4*i,
					CONSYS_REG_READ(consys_afe_reg_base + 4*i));
		}
		iounmap(consys_afe_reg_base);
	} else
		WMT_PLAT_PR_ERR("AFE base(0x%x) ioremap fail!\n", CONSYS_AFE_REG_BASE);
#endif
}

/* Wayne Chen - This function was copied from mt6759.c */
static INT32 consys_hw_vcn18_ctrl(MTK_WCN_BOOL enable)
{
#if CONSYS_PMIC_CTRL_ENABLE
	if (enable) {
		/*need PMIC driver provide new API protocol */
		/*1.AP power on VCN_1V8 LDO (with PMIC_WRAP API) VCN_1V8  */
#if COMMON_KERNEL_PMIC_SUPPORT
		regmap_update_bits(g_regmap_mt6357, MT6357_RG_LDO_VCN18_EN_ADDR,
			MT6357_RG_LDO_VCN18_EN_MASK << MT6357_RG_LDO_VCN18_EN_SHIFT,
			1 << MT6357_RG_LDO_VCN18_EN_SHIFT);
		regmap_update_bits(g_regmap_mt6357, MT6357_RG_LDO_VCN18_SW_OP_EN_ADDR,
			MT6357_RG_LDO_VCN18_SW_OP_EN_MASK << MT6357_RG_LDO_VCN18_SW_OP_EN_SHIFT,
			1 << MT6357_RG_LDO_VCN18_SW_OP_EN_SHIFT);
#else
#if defined(CONFIG_MTK_PMIC_CHIP_MT6357)
		KERNEL_pmic_set_register_value(PMIC_RG_LDO_VCN18_EN, 1);
		KERNEL_pmic_set_register_value(PMIC_RG_LDO_VCN18_SW_OP_EN, 1);
#else
		KERNEL_pmic_set_register_value(MT6351_PMIC_RG_VCN18_ON_CTRL, 0);
#endif
#endif

		/* VOL_DEFAULT, VOL_1200, VOL_1300, VOL_1500, VOL_1800... */
#if defined(CONFIG_MTK_LEGACY)
		hwPowerOn(MT6351_POWER_LDO_VCN18, VOL_1800 * 1000, "wcn_drv");
#else
		if (reg_VCN18) {
			regulator_set_voltage(reg_VCN18, 1800000, 1800000);
			if (regulator_enable(reg_VCN18))
				WMT_PLAT_PR_ERR("enable VCN18 fail\n");
			else
				WMT_PLAT_PR_DBG("enable VCN18 ok\n");
		}
#endif

		if (reg_VCN33_BT) {
			regulator_set_voltage(reg_VCN33_BT, 3300000, 3300000);
			if (regulator_enable(reg_VCN33_BT))
				WMT_PLAT_PR_ERR("WMT do BT PMIC on fail!\n");
		}

#if COMMON_KERNEL_PMIC_SUPPORT
		regmap_update_bits(g_regmap_mt6357, MT6357_RG_LDO_VCN33_HW0_OP_EN_ADDR,
			MT6357_RG_LDO_VCN33_HW0_OP_EN_MASK << MT6357_RG_LDO_VCN33_HW0_OP_EN_SHIFT,
			0 << MT6357_RG_LDO_VCN33_HW0_OP_EN_SHIFT);
		regmap_update_bits(g_regmap_mt6357, MT6357_RG_LDO_VCN33_HW0_OP_CFG_ADDR,
			MT6357_RG_LDO_VCN33_HW0_OP_CFG_MASK << MT6357_RG_LDO_VCN33_HW0_OP_CFG_SHIFT,
			0 << MT6357_RG_LDO_VCN33_HW0_OP_CFG_SHIFT);
#else
#if defined(CONFIG_MTK_PMIC_CHIP_MT6357)
		KERNEL_pmic_set_register_value(PMIC_RG_LDO_VCN33_HW0_OP_EN, 0);
		KERNEL_pmic_set_register_value(PMIC_RG_LDO_VCN33_HW0_OP_CFG, 0);
#else
		KERNEL_pmic_set_register_value(MT6351_PMIC_RG_VCN33_ON_CTRL_BT, 1);
#endif
#endif
	} else {
#if COMMON_KERNEL_PMIC_SUPPORT
		regmap_update_bits(g_regmap_mt6357, MT6357_RG_LDO_VCN18_EN_ADDR,
			MT6357_RG_LDO_VCN18_EN_MASK << MT6357_RG_LDO_VCN18_EN_SHIFT,
			1 << MT6357_RG_LDO_VCN18_EN_SHIFT);
		regmap_update_bits(g_regmap_mt6357, MT6357_RG_LDO_VCN18_SW_OP_EN_ADDR,
			MT6357_RG_LDO_VCN18_SW_OP_EN_MASK << MT6357_RG_LDO_VCN18_SW_OP_EN_SHIFT,
			1 << MT6357_RG_LDO_VCN18_SW_OP_EN_SHIFT);
#else
#if defined(CONFIG_MTK_PMIC_CHIP_MT6357)
		KERNEL_pmic_set_register_value(PMIC_RG_LDO_VCN18_EN, 1);
		KERNEL_pmic_set_register_value(PMIC_RG_LDO_VCN18_SW_OP_EN, 1);
#else
		KERNEL_pmic_set_register_value(MT6351_PMIC_RG_VCN33_ON_CTRL_BT, 0);
#endif
#endif
		if (reg_VCN33_BT)
			regulator_disable(reg_VCN33_BT);

#if COMMON_KERNEL_PMIC_SUPPORT
		regmap_update_bits(g_regmap_mt6357, MT6357_RG_LDO_VCN18_EN_ADDR,
			MT6357_RG_LDO_VCN18_EN_MASK << MT6357_RG_LDO_VCN18_EN_SHIFT,
			1 << MT6357_RG_LDO_VCN18_EN_SHIFT);
		regmap_update_bits(g_regmap_mt6357, MT6357_RG_LDO_VCN18_SW_OP_EN_ADDR,
			MT6357_RG_LDO_VCN18_SW_OP_EN_MASK << MT6357_RG_LDO_VCN18_SW_OP_EN_SHIFT,
			1 << MT6357_RG_LDO_VCN18_SW_OP_EN_SHIFT);
#else
		/*AP power off MT6351L VCN_1V8 LDO */
#if defined(CONFIG_MTK_PMIC_CHIP_MT6357)
		KERNEL_pmic_set_register_value(PMIC_RG_LDO_VCN18_EN, 1);
		KERNEL_pmic_set_register_value(PMIC_RG_LDO_VCN18_SW_OP_EN, 1);
#else
		KERNEL_pmic_set_register_value(MT6351_PMIC_RG_VCN18_ON_CTRL, 0);
#endif
#endif
#if defined(CONFIG_MTK_LEGACY)
		hwPowerDown(MT6351_POWER_LDO_VCN18, "wcn_drv");
#else
		if (reg_VCN18) {
			if (regulator_disable(reg_VCN18))
				WMT_PLAT_PR_ERR("disable VCN_1V8 fail!\n");
			else
				WMT_PLAT_PR_DBG("disable VCN_1V8 ok\n");
		}
#endif
	}
#endif
	return 0;
}

/* Wayne Chen - This function was copied from mt6759.c */
static VOID consys_vcn28_hw_mode_ctrl(UINT32 enable)
{
#if CONSYS_PMIC_CTRL_ENABLE
	if (enable) {
#if COMMON_KERNEL_PMIC_SUPPORT
		regmap_update_bits(g_regmap_mt6357, MT6357_RG_LDO_VCN28_HW0_OP_EN_ADDR,
			MT6357_RG_LDO_VCN28_HW0_OP_EN_MASK << MT6357_RG_LDO_VCN28_HW0_OP_EN_SHIFT,
			1 << MT6357_RG_LDO_VCN28_HW0_OP_EN_SHIFT);
		regmap_update_bits(g_regmap_mt6357, MT6357_RG_LDO_VCN28_HW0_OP_CFG_ADDR,
			MT6357_RG_LDO_VCN28_HW0_OP_CFG_MASK << MT6357_RG_LDO_VCN28_HW0_OP_CFG_SHIFT,
			0 << MT6357_RG_LDO_VCN28_HW0_OP_CFG_SHIFT);
#else
#if defined(CONFIG_MTK_PMIC_CHIP_MT6357)
		KERNEL_pmic_set_register_value(PMIC_RG_LDO_VCN28_HW0_OP_EN, 1);
		KERNEL_pmic_set_register_value(PMIC_RG_LDO_VCN28_HW0_OP_CFG, 0);
#else
		KERNEL_pmic_set_register_value(MT6351_PMIC_RG_VCN28_ON_CTRL, 1);
#endif
#endif
	} else {
#if COMMON_KERNEL_PMIC_SUPPORT
		regmap_update_bits(g_regmap_mt6357, MT6357_RG_LDO_VCN28_HW0_OP_EN_ADDR,
			MT6357_RG_LDO_VCN28_HW0_OP_EN_MASK << MT6357_RG_LDO_VCN28_HW0_OP_EN_SHIFT,
			0 << MT6357_RG_LDO_VCN28_HW0_OP_EN_SHIFT);
		regmap_update_bits(g_regmap_mt6357, MT6357_RG_LDO_VCN28_HW0_OP_CFG_ADDR,
			MT6357_RG_LDO_VCN28_HW0_OP_CFG_MASK << MT6357_RG_LDO_VCN28_HW0_OP_CFG_SHIFT,
			0 << MT6357_RG_LDO_VCN28_HW0_OP_CFG_SHIFT);
#else
#if defined(CONFIG_MTK_PMIC_CHIP_MT6357)
		KERNEL_pmic_set_register_value(PMIC_RG_LDO_VCN28_HW0_OP_EN, 0);
		KERNEL_pmic_set_register_value(PMIC_RG_LDO_VCN28_HW0_OP_CFG, 0);
#else
		KERNEL_pmic_set_register_value(MT6351_PMIC_RG_VCN28_ON_CTRL, 0);
#endif
#endif
	}
#endif
}

/* Wayne Chen - This function was copied from mt6759.c */
static INT32 consys_hw_vcn28_ctrl(UINT32 enable)
{
#if CONSYS_PMIC_CTRL_ENABLE
	if (enable) {
		/*in co-clock mode,need to turn on vcn28 when fm on */
		if (reg_VCN28) {
			regulator_set_voltage(reg_VCN28, 2800000, 2800000);
			if (regulator_enable(reg_VCN28))
				WMT_PLAT_PR_ERR("WMT do VCN28 PMIC on fail!\n");
		}
		WMT_PLAT_PR_INFO("turn on vcn28 for fm/gps usage in co-clock mode\n");
	} else {
		/*in co-clock mode,need to turn off vcn28 when fm off */
		if (reg_VCN28)
			regulator_disable(reg_VCN28);
		WMT_PLAT_PR_INFO("turn off vcn28 for fm/gps usage in co-clock mode\n");
	}
#endif
	return 0;
}

static INT32 consys_hw_bt_vcn33_ctrl(UINT32 enable)
{
#if CONSYS_BT_WIFI_SHARE_V33
	/* spin_lock_irqsave(&gBtWifiV33.lock,gBtWifiV33.flags); */
	if (enable) {
		if (gBtWifiV33.counter == 1) {
			gBtWifiV33.counter++;
			WMT_PLAT_PR_DBG("V33 has been enabled,counter(%d)\n", gBtWifiV33.counter);
		} else if (gBtWifiV33.counter == 2) {
			WMT_PLAT_PR_DBG("V33 has been enabled,counter(%d)\n", gBtWifiV33.counter);
		} else {
#if CONSYS_PMIC_CTRL_ENABLE
			/*do BT PMIC on,depenency PMIC API ready */
			/*switch BT PALDO control from SW mode to HW mode:0x416[5]-->0x1 */
			/* VOL_DEFAULT, VOL_3300, VOL_3400, VOL_3500, VOL_3600 */
			hwPowerOn(MT6351_POWER_LDO_VCN33_BT, VOL_3300 * 1000, "wcn_drv");
			mt6351_upmu_set_rg_vcn33_on_ctrl(1);
#endif
			WMT_PLAT_PR_INFO("WMT do BT/WIFI v3.3 on\n");
			gBtWifiV33.counter++;
		}

	} else {
		if (gBtWifiV33.counter == 1) {
			/*do BT PMIC off */
			/*switch BT PALDO control from HW mode to SW mode:0x416[5]-->0x0 */
#if CONSYS_PMIC_CTRL_ENABLE
			mt6351_upmu_set_rg_vcn33_on_ctrl(0);
			hwPowerDown(MT6351_POWER_LDO_VCN33_BT, "wcn_drv");
#endif
			WMT_PLAT_PR_INFO("WMT do BT/WIFI v3.3 off\n");
			gBtWifiV33.counter--;
		} else if (gBtWifiV33.counter == 2) {
			gBtWifiV33.counter--;
			WMT_PLAT_PR_DBG("V33 no need disabled,counter(%d)\n", gBtWifiV33.counter);
		} else {
			WMT_PLAT_PR_DBG("V33 has been disabled,counter(%d)\n", gBtWifiV33.counter);
		}

	}
	/* spin_unlock_irqrestore(&gBtWifiV33.lock,gBtWifiV33.flags); */
#else
	if (enable) {
		/*do BT PMIC on,depenency PMIC API ready */
		/*switch BT PALDO control from SW mode to HW mode:0x416[5]-->0x1 */
#if CONSYS_PMIC_CTRL_ENABLE
		/* VOL_DEFAULT, VOL_3300, VOL_3400, VOL_3500, VOL_3600 */
#if defined(CONFIG_MTK_LEGACY)
		hwPowerOn(MT6351_POWER_LDO_VCN33_BT, VOL_3300 * 1000, "wcn_drv");
#else
		if (reg_VCN33_BT) {
			regulator_set_voltage(reg_VCN33_BT, 3300000, 3300000);
			if (regulator_enable(reg_VCN33_BT))
				WMT_PLAT_PR_ERR("WMT do BT PMIC on fail!\n");
		}
#endif

#if COMMON_KERNEL_PMIC_SUPPORT
		regmap_update_bits(g_regmap_mt6357, MT6357_RG_LDO_VCN33_HW0_OP_EN_ADDR,
			MT6357_RG_LDO_VCN33_HW0_OP_EN_MASK << MT6357_RG_LDO_VCN33_HW0_OP_EN_SHIFT,
			1 << MT6357_RG_LDO_VCN33_HW0_OP_EN_SHIFT);
		regmap_update_bits(g_regmap_mt6357, MT6357_RG_LDO_VCN33_HW0_OP_CFG_ADDR,
			MT6357_RG_LDO_VCN33_HW0_OP_CFG_MASK << MT6357_RG_LDO_VCN33_HW0_OP_CFG_SHIFT,
			0 << MT6357_RG_LDO_VCN33_HW0_OP_CFG_SHIFT);
#else
#if defined(CONFIG_MTK_PMIC_CHIP_MT6357)
		KERNEL_pmic_set_register_value(PMIC_RG_LDO_VCN33_HW0_OP_EN, 1);
		KERNEL_pmic_set_register_value(PMIC_RG_LDO_VCN33_HW0_OP_CFG, 0);
#else
		KERNEL_pmic_set_register_value(MT6351_PMIC_RG_VCN33_ON_CTRL_BT, 1);
#endif
#endif

#endif
		WMT_PLAT_PR_DBG("WMT do BT PMIC on\n");
	} else {
		/*do BT PMIC off */
		/*switch BT PALDO control from HW mode to SW mode:0x416[5]-->0x0 */
#if CONSYS_PMIC_CTRL_ENABLE
#if COMMON_KERNEL_PMIC_SUPPORT
		regmap_update_bits(g_regmap_mt6357, MT6357_RG_LDO_VCN33_HW0_OP_EN_ADDR,
			MT6357_RG_LDO_VCN33_HW0_OP_EN_MASK << MT6357_RG_LDO_VCN33_HW0_OP_EN_SHIFT,
			0 << MT6357_RG_LDO_VCN33_HW0_OP_EN_SHIFT);
		regmap_update_bits(g_regmap_mt6357, MT6357_RG_LDO_VCN33_HW0_OP_CFG_ADDR,
			MT6357_RG_LDO_VCN33_HW0_OP_CFG_MASK << MT6357_RG_LDO_VCN33_HW0_OP_CFG_SHIFT,
			0 << MT6357_RG_LDO_VCN33_HW0_OP_CFG_SHIFT);
#else
#if defined(CONFIG_MTK_PMIC_CHIP_MT6357)
		KERNEL_pmic_set_register_value(PMIC_RG_LDO_VCN33_HW0_OP_EN, 0);
		KERNEL_pmic_set_register_value(PMIC_RG_LDO_VCN33_HW0_OP_CFG, 0);
#else
		KERNEL_pmic_set_register_value(MT6351_PMIC_RG_VCN33_ON_CTRL_BT, 0);
#endif
#endif
#if defined(CONFIG_MTK_LEGACY)
		hwPowerDown(MT6351_POWER_LDO_VCN33_BT, "wcn_drv");
#else
		if (reg_VCN33_BT)
			regulator_disable(reg_VCN33_BT);
#endif
#endif
		WMT_PLAT_PR_DBG("WMT do BT PMIC off\n");
	}
#endif

	return 0;
}

static INT32 consys_hw_wifi_vcn33_ctrl(UINT32 enable)
{
#if CONSYS_BT_WIFI_SHARE_V33
	mtk_wcn_consys_hw_bt_paldo_ctrl(enable);
#else
	if (enable) {
		/*do WIFI PMIC on,depenency PMIC API ready */
		/*switch WIFI PALDO control from SW mode to HW mode:0x418[14]-->0x1 */
#if CONSYS_PMIC_CTRL_ENABLE
#if defined(CONFIG_MTK_LEGACY)
		hwPowerOn(MT6351_POWER_LDO_VCN33_WIFI, VOL_3300 * 1000, "wcn_drv");
#else
		if (reg_VCN33_WIFI) {
			regulator_set_voltage(reg_VCN33_WIFI, 3300000, 3300000);
			if (regulator_enable(reg_VCN33_WIFI))
				WMT_PLAT_PR_ERR("WMT do WIFI PMIC on fail!\n");
		}
#endif
#if COMMON_KERNEL_PMIC_SUPPORT
		regmap_update_bits(g_regmap_mt6357, MT6357_RG_LDO_VCN33_HW0_OP_EN_ADDR,
			MT6357_RG_LDO_VCN33_HW0_OP_EN_MASK << MT6357_RG_LDO_VCN33_HW0_OP_EN_SHIFT,
			1 << MT6357_RG_LDO_VCN33_HW0_OP_EN_SHIFT);
		regmap_update_bits(g_regmap_mt6357, MT6357_RG_LDO_VCN33_HW0_OP_CFG_ADDR,
			MT6357_RG_LDO_VCN33_HW0_OP_CFG_MASK << MT6357_RG_LDO_VCN33_HW0_OP_CFG_SHIFT,
			0 << MT6357_RG_LDO_VCN33_HW0_OP_CFG_SHIFT);
#else
#if defined(CONFIG_MTK_PMIC_CHIP_MT6357)
		KERNEL_pmic_set_register_value(PMIC_RG_LDO_VCN33_HW0_OP_EN, 1);
		KERNEL_pmic_set_register_value(PMIC_RG_LDO_VCN33_HW0_OP_CFG, 0);
#else
		KERNEL_pmic_set_register_value(MT6351_PMIC_RG_VCN33_ON_CTRL_WIFI, 1);
#endif
#endif
#endif
		WMT_PLAT_PR_DBG("WMT do WIFI PMIC on\n");
	} else {
		/*do WIFI PMIC off */
		/*switch WIFI PALDO control from HW mode to SW mode:0x418[14]-->0x0 */
#if CONSYS_PMIC_CTRL_ENABLE
#if COMMON_KERNEL_PMIC_SUPPORT
		regmap_update_bits(g_regmap_mt6357, MT6357_RG_LDO_VCN33_HW0_OP_EN_ADDR,
			MT6357_RG_LDO_VCN33_HW0_OP_EN_MASK << MT6357_RG_LDO_VCN33_HW0_OP_EN_SHIFT,
			0 << MT6357_RG_LDO_VCN33_HW0_OP_EN_SHIFT);
		regmap_update_bits(g_regmap_mt6357, MT6357_RG_LDO_VCN33_HW0_OP_CFG_ADDR,
			MT6357_RG_LDO_VCN33_HW0_OP_CFG_MASK << MT6357_RG_LDO_VCN33_HW0_OP_CFG_SHIFT,
			0 << MT6357_RG_LDO_VCN33_HW0_OP_CFG_SHIFT);
#else
#if defined(CONFIG_MTK_PMIC_CHIP_MT6357)
		KERNEL_pmic_set_register_value(PMIC_RG_LDO_VCN33_HW0_OP_EN, 0);
		KERNEL_pmic_set_register_value(PMIC_RG_LDO_VCN33_HW0_OP_CFG, 0);
#else
		KERNEL_pmic_set_register_value(MT6351_PMIC_RG_VCN33_ON_CTRL_WIFI, 0);
#endif
#endif
#if defined(CONFIG_MTK_LEGACY)
		hwPowerDown(MT6351_POWER_LDO_VCN33_WIFI, "wcn_drv");
#else
		if (reg_VCN33_WIFI)
			regulator_disable(reg_VCN33_WIFI);
#endif

#endif
		WMT_PLAT_PR_DBG("WMT do WIFI PMIC off\n");
	}

#endif

	return 0;
}

static INT32 consys_emi_mpu_set_region_protection(VOID)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
#if IS_ENABLED(CONFIG_MTK_EMI_LEGACY_V0)
	/*set MPU for EMI share Memory */
	WMT_PLAT_PR_INFO("setting MPU for EMI share memory\n");

	/* 5 = Forbidden, 0 = No_protect */
	emi_mpu_set_region_protection(gConEmiPhyBase + SZ_1M / 2,
			gConEmiPhyBase + gConEmiSize - 1,
			22,
			SET_ACCESS_PERMISSION(LOCK, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN,
				FORBIDDEN, NO_PROTECTION, FORBIDDEN, NO_PROTECTION));
#endif
#else
#if CONSYS_EMI_MPU_SETTING
	/*set MPU for EMI share Memory */
	WMT_PLAT_PR_INFO("setting MPU for EMI share memory\n");

	/* 5 = Forbidden, 0 = No_protect */
	emi_mpu_set_region_protection(gConEmiPhyBase + SZ_1M / 2,
			gConEmiPhyBase + gConEmiSize - 1,
			22,
			SET_ACCESS_PERMISSON(LOCK, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN,
				FORBIDDEN, NO_PROTECTION, FORBIDDEN, NO_PROTECTION));
#endif
#endif
	return 0;
}

static UINT32 consys_emi_set_remapping_reg(VOID)
{
#ifdef CONFIG_OF		/*use DT */
	UINT32 addrPhy = 0;

	/*consys to ap emi remapping register:10001380, cal remapping address */
	addrPhy = (gConEmiPhyBase >> 21) & 0x1FFF;

	/*enable consys to ap emi remapping bit13 */
	addrPhy = addrPhy | 0x2000;

	CONSYS_REG_WRITE(conn_reg.topckgen_base + CONSYS_EMI_MAPPING_OFFSET,
			CONSYS_REG_READ(conn_reg.topckgen_base + CONSYS_EMI_MAPPING_OFFSET) | addrPhy);

	WMT_PLAT_PR_INFO("CONSYS_EMI_MAPPING dump in restore cb(0x%08x)\n",
			CONSYS_REG_READ(conn_reg.topckgen_base + CONSYS_EMI_MAPPING_OFFSET));
#endif
	return 0;
}

static INT32 bt_wifi_share_v33_spin_lock_init(VOID)
{
#if CONSYS_BT_WIFI_SHARE_V33
	gBtWifiV33.counter = 0;
	spin_lock_init(&gBtWifiV33.lock);
#endif
	return 0;
}

static INT32 consys_read_irq_info_from_dts(struct platform_device *pdev, PINT32 irq_num, PUINT32 irq_flag)
{
#ifdef CONFIG_OF		/*use DT */
	struct device_node *node;
	UINT32 irq_info[3] = { 0, 0, 0 };

	INT32 iret = -1;

	node = pdev->dev.of_node;
	if (node) {
		*irq_num = irq_of_parse_and_map(node, 0);
		/* get the interrupt line behaviour */
		if (of_property_read_u32_array(node, "interrupts", irq_info, ARRAY_SIZE(irq_info))) {
			WMT_PLAT_PR_ERR("get irq flags from DTS fail!!\n");
			return iret;
		}
		*irq_flag = irq_info[2];
		WMT_PLAT_PR_INFO("get irq id(%d) and irq trigger flag(%d) from DT\n", *irq_num,
				   *irq_flag);
	} else {
		WMT_PLAT_PR_ERR("[%s] can't find CONSYS compatible node\n", __func__);
		return iret;
	}
#endif
	return 0;
}

static INT32 consys_read_reg_from_dts(struct platform_device *pdev)
{
#ifdef CONFIG_OF		/*use DT */
	INT32 iRet = -1;
	struct device_node *node = NULL;

	node = pdev->dev.of_node;
	if (node) {
		/* registers base address */
		conn_reg.mcu_base = (SIZE_T) of_iomap(node, 0);
		WMT_PLAT_PR_DBG("Get mcu register base(0x%zx)\n", conn_reg.mcu_base);
		conn_reg.ap_rgu_base = (SIZE_T) of_iomap(node, 1);
		WMT_PLAT_PR_DBG("Get ap_rgu register base(0x%zx)\n", conn_reg.ap_rgu_base);
		conn_reg.topckgen_base = (SIZE_T) of_iomap(node, 2);
		WMT_PLAT_PR_DBG("Get topckgen register base(0x%zx)\n", conn_reg.topckgen_base);
		conn_reg.spm_base = (SIZE_T) of_iomap(node, 3);
		WMT_PLAT_PR_DBG("Get spm register base(0x%zx)\n", conn_reg.spm_base);
	} else {
		WMT_PLAT_PR_ERR("[%s] can't find CONSYS compatible node\n", __func__);
		return iRet;
	}
#endif
	return 0;
}

static VOID force_trigger_assert_debug_pin(VOID)
{
#ifdef CONFIG_OF		/*use DT */
	CONSYS_REG_WRITE(conn_reg.topckgen_base + CONSYS_AP2CONN_OSC_EN_OFFSET,
			CONSYS_REG_READ(conn_reg.topckgen_base +
				CONSYS_AP2CONN_OSC_EN_OFFSET) & ~CONSYS_AP2CONN_WAKEUP_BIT);
	WMT_PLAT_PR_INFO("enable:dump CONSYS_AP2CONN_OSC_EN_REG(0x%x)\n",
			CONSYS_REG_READ(conn_reg.topckgen_base + CONSYS_AP2CONN_OSC_EN_OFFSET));
	usleep_range(64, 96);
	CONSYS_REG_WRITE(conn_reg.topckgen_base + CONSYS_AP2CONN_OSC_EN_OFFSET,
			CONSYS_REG_READ(conn_reg.topckgen_base +
				CONSYS_AP2CONN_OSC_EN_OFFSET) | CONSYS_AP2CONN_WAKEUP_BIT);
	WMT_PLAT_PR_INFO("disable:dump CONSYS_AP2CONN_OSC_EN_REG(0x%x)\n",
			CONSYS_REG_READ(conn_reg.topckgen_base + CONSYS_AP2CONN_OSC_EN_OFFSET));
#endif
}

static UINT32 consys_read_cpupcr(VOID)
{
#ifdef CONFIG_OF		/*use DT */
	if (conn_reg.mcu_base == 0)
		return 0;

	if (mtk_consys_check_reg_readable_by_addr(conn_reg.mcu_base + CONSYS_CPUPCR_OFFSET) == 0)
		return 0;

	return CONSYS_REG_READ(conn_reg.mcu_base + CONSYS_CPUPCR_OFFSET);
#else
	return 0;
#endif
}

static UINT32 consys_read_debug_crs(enum connsys_debug_cr cr)
{
#ifdef CONFIG_OF		/*use DT */
	P_CONSYS_EMI_ADDR_INFO emi_phy_addr;

	emi_phy_addr = consys_soc_get_emi_phy_add();

	if (cr == CONNSYS_EMI_REMAP) {
		if (emi_phy_addr != NULL && emi_phy_addr->emi_remap_offset)
			return CONSYS_REG_READ(conn_reg.topckgen_base +
					emi_phy_addr->emi_remap_offset);
		else
			WMT_PLAT_PR_INFO("EMI remap has no value\n");
	}

	if (mtk_consys_check_reg_readable_by_addr(conn_reg.mcu_base + CONSYS_CPUPCR_OFFSET) == 0)
		return 0;

	if (conn_reg.mcu_base) {

		switch (cr) {
		case CONNSYS_CPU_CLK:
			if (mtk_consys_check_reg_readable_by_addr(
					conn_reg.mcu_base + CONSYS_CPU_CLK_STATUS_OFFSET) == 0)
				return 0;
			return CONSYS_REG_READ(conn_reg.mcu_base + CONSYS_CPU_CLK_STATUS_OFFSET);
		case CONNSYS_BUS_CLK:
			if (mtk_consys_check_reg_readable_by_addr(
					conn_reg.mcu_base + CONSYS_BUS_CLK_STATUS_OFFSET) == 0)
				return 0;
			return CONSYS_REG_READ(conn_reg.mcu_base + CONSYS_BUS_CLK_STATUS_OFFSET);
		case CONNSYS_DEBUG_CR1:
			if (mtk_consys_check_reg_readable_by_addr(
					conn_reg.mcu_base + CONSYS_DBG_CR1_OFFSET) == 0)
				return 0;
			return CONSYS_REG_READ(conn_reg.mcu_base + CONSYS_DBG_CR1_OFFSET);
		case CONNSYS_DEBUG_CR2:
			if (mtk_consys_check_reg_readable_by_addr(
					conn_reg.mcu_base + CONSYS_DBG_CR2_OFFSET) == 0)
				return 0;
			return CONSYS_REG_READ(conn_reg.mcu_base + CONSYS_DBG_CR2_OFFSET);
		default:
			return 0;
		}
	}
#endif
	return 0;
}

static INT32 consys_poll_cpupcr_dump(UINT32 times, UINT32 sleep_ms)
{
	UINT64 ts;
	ULONG nsec;
	INT32 str_len = 0, i;
	char str[DBG_LOG_STR_SIZE] = {""};
	unsigned int remain = DBG_LOG_STR_SIZE;
	char *p = NULL;

	p = str;
	for (i = 0; i < times; i++) {
		osal_get_local_time(&ts, &nsec);
		str_len = snprintf(p, remain, "%llu.%06lu/0x%08x;", ts, nsec,
								consys_read_cpupcr());
		if (str_len < 0) {
			WMT_PLAT_PR_WARN("%s snprintf fail", __func__);
			continue;
		}
		p += str_len;
		remain -= str_len;

		if (sleep_ms > 0)
			osal_sleep_ms(sleep_ms);
	}

	WMT_PLAT_PR_INFO("TIME/CPUPCR: %s", str);
	if (mtk_consys_check_reg_readable()) {
		WMT_PLAT_PR_INFO("CONNSYS cpu:0x%x/bus:0x%x/dbg_cr1:0x%x/dbg_cr2:0x%x/EMIaddr:0x%x\n",
					  consys_read_debug_crs(CONNSYS_CPU_CLK),
					  consys_read_debug_crs(CONNSYS_BUS_CLK),
					  consys_read_debug_crs(CONNSYS_DEBUG_CR1),
					  consys_read_debug_crs(CONNSYS_DEBUG_CR2),
					  consys_read_debug_crs(CONNSYS_EMI_REMAP));
	}
	return 0;
}

static UINT32 consys_soc_chipid_get(VOID)
{
	return PLATFORM_SOC_CHIP;
}

static P_CONSYS_EMI_ADDR_INFO consys_soc_get_emi_phy_add(VOID)
{
	return &mtk_wcn_emi_addr_info;
}

static INT32 consys_emi_coredump_remapping(UINT8 __iomem **addr, UINT32 enable)
{
	if (enable) {
		*addr = ioremap(gConEmiPhyBase + CONSYS_EMI_COREDUMP_OFFSET, CONSYS_EMI_MEM_SIZE);
		if (*addr) {
			WMT_PLAT_PR_INFO("COREDUMP EMI mapping OK virtual(0x%p) physical(0x%x)\n",
					   *addr, (UINT32) gConEmiPhyBase + CONSYS_EMI_COREDUMP_OFFSET);
			memset_io(*addr, 0, CONSYS_EMI_MEM_SIZE);
		} else {
			WMT_PLAT_PR_ERR("EMI mapping fail\n");
			return -1;
		}
	} else {
		if (*addr) {
			iounmap(*addr);
			*addr = NULL;
		}
	}
	return 0;
}

static INT32 consys_reset_emi_coredump(UINT8 __iomem *addr)
{
	if (!addr) {
		WMT_PLAT_PR_ERR("get virtual address fail\n");
		return -1;
	}
	WMT_PLAT_PR_INFO("Reset EMI(0xF0080000 ~ 0xF0080400) and (0xF0088400 ~ 0xF0090400)\n");
	/* reset 0xF0080000 ~ 0xF0080400 (1K) */
	memset_io(addr, 0, 0x400);
	/* reset 0xF0088400 ~ 0xF0090400 (32K)  */
	memset_io(addr + CONSYS_EMI_PAGED_DUMP_OFFSET, 0, 0x8000);
	return 0;
}

#if (COMMON_KERNEL_CLK_SUPPORT)
static MTK_WCN_BOOL consys_need_store_pdev(VOID)
{
	return MTK_WCN_BOOL_TRUE;
}

static UINT32 consys_store_pdev(struct platform_device *pdev)
{
	connsys_pdev = pdev;
	return 0;
}
#endif

static UINT64 consys_get_options(VOID)
{
	UINT64 options = OPT_POWER_ON_DLM_TABLE |
			OPT_SET_MCUCLK_TABLE_1_2 |
			OPT_WIFI_LTE_COEX |
			OPT_INIT_COEX_AFTER_RF_CALIBRATION |
			OPT_SET_OSC_TYPE |
			OPT_SET_COREDUMP_LEVEL |
			OPT_GPS_SYNC |
			OPT_WIFI_LTE_COEX_TABLE_1 |
			OPT_NORMAL_PATCH_DWN_0;
	return options;
}

