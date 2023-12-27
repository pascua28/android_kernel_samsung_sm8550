// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Juhyung Park <qkrwngud825@gmail.com>
 *
 * Partially based on kernel/module.c.
 */

#ifdef CONFIG_LAZY_INITCALL_DEBUG
#define DEBUG
#define __fatal pr_err
#else
#define __fatal panic
#endif

#define pr_fmt(fmt) "lazy_initcall: " fmt

#include <linux/syscalls.h>
#include <linux/kmemleak.h>
#include <uapi/linux/module.h>
#include <uapi/linux/time.h>

#include "module-internal.h"

static void __init show_errors(struct work_struct *unused);
static __initdata DECLARE_DELAYED_WORK(show_errors_work, show_errors);
static DEFINE_MUTEX(lazy_initcall_mutex);
static bool completed;

/*
 * Why is this here, instead of defconfig?
 *
 * Data used in defconfig isn't freed in free_initmem() and putting a list this
 * big into the defconfig isn't really ideal anyways.
 *
 * Since lazy_initcall isn't meant to be generic, set this here.
 *
 * This list is generatable by putting .ko modules from vendor, vendor_boot and
 * vendor_dlkm to a directory and running the following:
 *
 * MODDIR=/path/to/modules
 * find "$MODDIR" -name '*.ko' -exec modinfo {} + | grep '^name:' | awk '{print $2}' | sort | uniq | while read f; do printf '\t"'$f'",\n'; done
 * find "$MODDIR" -name '*.ko' | while read f; do if ! modinfo $f | grep -q "^name:"; then n=$(basename $f); n="${n%.*}"; printf '\t"'$n'",\n'; fi; done | sort | uniq
 */

static const __initconst char * const targets_list[] = {
	"camera",
	"kiwi_v2",
	"ipa_clientsm",
	"ipanetm",
	"rndisipam",
	"stm_ts_spi",
	"msm_kgsl",
	"mhi_dev_uci",
	"snd_soc_cs35l45_i2c",
	"sec_qc_qcom_wdt_core",
	"ipam",
	"sec_thermistor",
	"machine_dlkm",
	"cpufreq_stats_scmi",
	"usb_notifier_qcom",
	"coresight_hwevent",
	"coresight_tmc",
	"sec_direct_charger",
	"msm_drm",
	"qdss_bridge",
	"usb_f_qdss",
	"spcom",
	"lpass_cdc_wsa_macro_dlkm",
	"cnss2",
	"lpass_cdc_tx_macro_dlkm",
	"max77705_fuelgauge",
	"lpass_cdc_rx_macro_dlkm",
	"spss_utils",
	"abc_hub",
	"usb_f_gsi",
	"flicker_sensor",
	"qcom_pon",
	"bt_fm_slim",
	"gsim",
	"lpass_cdc_wsa2_macro_dlkm",
	"lpass_cdc_va_macro_dlkm",
	"mac80211",
	"sec_tsp_dumpkey",
	"mhi_dev_dtr",
	"swr_dmic_dlkm",
	"sec_ap_pmic",
	"mhi_dev_drv",
	"snd_soc_cs35l43_i2c",
	"wlan_firmware_service",
	"isg6320",
	"uwb",
	"msm_eva",
	"mhi_dev_satellite",
	"flicker_test",
	"adsp_loader_dlkm",
	"swr_ctrl_dlkm",
	"max77705_charger",
	"coresight_tpda",
	"audio_pkt_dlkm",
	"mhi_dev_netdev",
	"qrtr_mhi",
	"cps4038_charger",
	"coresight_cti",
	"memlat_scmi",
	"pdic_max77705",
	"qcrypto_msm_dlkm",
	"sdhci_msm_sec",
	"rmnet_wlan",
	"btpower",
	"stmvl53l8",
	"rmnet_perf",
	"wcd938x_dlkm",
	"pinctrl_lpi_dlkm",
	"coresight_dummy",
	"stm_p_basic",
	"snd_soc_hdmi_codec",
	"coresight_funnel",
	"lpass_cdc_dlkm",
	"wcd9xxx_dlkm",
	"mhi_cntrl_qcom",
	"audio_prm_dlkm",
	"spf_core_dlkm",
	"gpr_dlkm",
	"snd_soc_cirrus_amp",
	"q6_notifier_dlkm",
	"sec_qc_qcom_reboot_reason",
	"mbhc_dlkm",
	"qcom_q6v5_pas",
	"qbt2000_spidev",
	"fingerprint",
	"f_fs_ipc_log",
	"rmnet_shs",
	"bcl_soc",
	"pca9481_charger",
	"cnss_utils",
	"coresight_csr",
	"wez02",
	"hdm",
	"sec_common_fn",
	"sec_qc_rbcmd",
	"rmnet_sch",
	"rmnet_aps",
	"snd_soc_cs40l26",
	"qrtr_smd",
	"input_booster_lkm",
	"coresight_remote_etm",
	"radio_i2c_rtc6226_qca",
	"usb_f_diag",
	"cnss_nl",
	"wcd938x_slave_dlkm",
	"smcinvoke_dlkm",
	"stm_console",
	"mfd_max77705",
	"coresight_tpdm",
	"sec_battery",
	"sec_qc_rst_exinfo",
	"rmnet_offload",
	"cnss_prealloc",
	"stm_p_ost",
	"hdmi_dlkm",
	"dropdump",
	"usb_f_conn_gadget",
	"dwc3_msm",
	"qti_qmi_sensor_v2",
	"synx_driver",
	"cnss_plat_ipc_qmi_svc",
	"cpufreq_limit",
	"snd_usb_audio_qmi",
	"coresight_replicator",
	"coresight_stm",
	"repeater_qti_pmic_eusb2",
	"qti_userspace_cdev",
	"qcom_cpufreq_hw_debug",
	"i2c_msm_geni",
	"sb_core",
	"smsc95xx",
	"lt9611uxc",
	"sg",
	"qcom_hv_haptics",
	"frpc_adsprpc",
	"msm_video",
	"qcom_va_minidump",
	"leds_qti_flash",
	"gh_irq_lend",
	"nfc_sec",
	"spi_msm_geni",
	"msm_sharedmem",
	"qfprom_sys",
	"qcom_iommu_debug",
	"msm_performance",
	"stub_dlkm",
	"hung_task_enh",
	"msm_geni_serial",
	"gh_tlmm_vm_mem_access",
	"qti_ocp_notifier",
	"smsc75xx",
	"wsa884x_dlkm",
	"hall_ic",
	"hdcp_qseecom_dlkm",
	"qcom_ipc_lite",
	"qcom_spmi_adc5_gen3",
	"msm_ext_display",
	"mhi_dev_net",
	"msm_show_epoch",
	"ep_pcie_drv",
	"sec_qc_hw_param",
	"qseecom_proxy",
	"sec_abc_detect_conn",
	"qrtr_gunyah",
	"snvm",
	"qpnp_amoled_regulator",
	"phy_qcom_emu",
	"sec_arm64_fsimd_debug",
	"qcom_pil_info",
	"snd_soc_wm_adsp",
	"cfg80211",
	"slim_qcom_ngd_ctrl",
	"wsa883x_dlkm",
	"sdpm_clk",
	"hwmon",
	"sec_arm64_debug",
	"fingerprint_sysfs",
	"sec_qc_soc_id",
	"msm_lmh_dcvs",
	"reboot_mode",
	"industrialio_buffer_cb",
	"ddr_cdev",
	"dev_ril_bridge",
	"qti_qmi_cdev",
	"nb7vpq904m",
	"fsa4480_i2c",
	"sec_qc_smem",
	"memlat_vendor",
	"icc_test",
	"sec_pon_alarm",
	"i3c_master_msm_geni",
	"qcom_esoc",
	"leds_qpnp_vibrator_ldo",
	"max31760_fan",
	"sec_reboot_cmd",
	"pm8941_pwrkey",
	"vibrator_vib_info",
	"cpu_voltage_cooling",
	"qcom_q6v5",
	"qcom_spmi_temp_alarm",
	"kperfmon",
	"policy_engine",
	"sec_qc_rdx_bootdev",
	"usb_f_ss_acm",
	"leds_qti_tri_led",
	"usb_f_ccid",
	"swr_haptics_dlkm",
	"usbmon",
	"nvmem_qfprom",
	"qcom_lpm",
	"phy_generic",
	"repeater_i2c_eusb2",
	"lvstest",
	"leds_s2mpb02",
	"sec_pd",
	"msm_mmrm",
	"qcom_spss",
	"mhi",
	"phy_msm_snps_eusb2",
	"sec_audio_sysfs",
	"wcd_usbss_i2c",
	"usb_f_cdev",
	"qti_battery_charger",
	"sync_fence",
	"msm_hw_fence",
	"qcom_vadc_common",
	"switch_gpio",
	"cpufreq_stats_vendor",
	"memlat",
	"audpkt_ion_dlkm",
	"gh_mem_notifier",
	"smsc",
	"redriver",
	"usb_bam",
	"rdbg",
	"qti_fixed_regulator",
	"qcedev_mod_dlkm",
	"qce50_dlkm",
	"adsp_sleepmon",
	"hall_ic_notifier",
	"phy_msm_ssusb_qmp",
	"usb_f_ss_mon_gadget",
	"sps_drv",
	"sec_input_notifier",
	"ucsi_glink",
	"rmnet_perf_tether",
	"sec_tclm_v2",
	"sec_cmd",
	"if_cb_manager",
	"sec_secure_touch",
	"sec_tsp_log",
	"core_hang_detect",
	"ehset",
	"stm_ftrace",
	"hvc_gunyah",
	"qti_battery_debug",
	"charger_ulog_glink",
	"pmic_glink_debug",
	"altmode_glink",
	"phy_msm_m31_eusb2",
	"repeater",
	"adsp_factory_module",
	"sensors_core",
	"pmic_pon_log",
	"coresight_tgu",
	"rmnet_core",
	"boot_stats",
	"swr_dlkm",
	"rmnet_ctl",
	"ipa_fmwk",
	"plh_scmi",
	"plh_vendor",
	"sys_pm_vx",
	"subsystem_sleep_stats",
	"glink_probe",
	"dmesg_dumper",
	"soc_sleep_stats",
	"cdsp_loader",
	"q6_dlkm",
	"panel_event_notifier",
	"usb_typec_manager",
	"common_muic",
	"input_cs40l26_i2c",
	"vbus_notifier",
	"rimps_log",
	"sec_panel_notifier",
	"tz_log_dlkm",
	"snd_event_dlkm",
	"stm_core",
	"wcd_core_dlkm",
	"slimbus",
	"sec_vibrator_inputff_module",
	"eud",
	"microdump_collector",
	"pdic_notifier_module",
	"cl_dsp",
	"usb_notify_layer",
	"qcom_sysmon",
	"snd_debug_proc",
	"cdsprm",
	"smp2p",
	"q6_pdr_dlkm",
	"coresight",
	"glink_pkt",
	"gpucc_crow",
	"qcom_glink_spss",
	"smp2p_sleepstate",
	"msm_memshare",
	"heap_mem_ext_v01",
	"qsee_ipc_irq_bridge",
	"switch_class",
	"qti_devfreq_cdev",
	"qcom_cpuss_sleep_stats",
	"qti_cpufreq_cdev",
	"bam_dma",
	"debugcc_crow",
	"debugcc_kalama",
	"qcom_edac",
	"phy_qcom_ufs_qmp_v4",
	"pinctrl_spmi_mpp",
	"camcc_crow",
	"phy_qcom_ufs_qmp_v4_lahaina",
	"pwm_qti_lpg",
	"videocc_crow",
	"gpucc_kalama",
	"qcom_ramdump",
	"sysmon_subsystem_stats",
	"phy_qcom_ufs_qmp_v4_waipio",
	"pmic_glink",
	"pci_msm_drv",
	"pdr_interface",
	"rproc_qcom_common",
	"qcom_smd",
	"phy_qcom_ufs_qmp_v4_khaje",
	"qcom_glink_smem",
	"qcom_glink",
	"qmi_helpers",
	"twofish_generic",
	"sec_qc_user_reset",
	"lcd",
	"msm_show_resume_irq",
	"msm_gpi",
	"msm_sysstats",
	"ssg",
	"phy_qcom_ufs_qmp_v4_kona",
	"blk_sec_stats",
	"blk_sec_common",
	"twofish_common",
	"zram",
	"spmi_pmic_arb_debug",
	"zsmalloc",
	"qrng_dlkm",
	"sec_qc_param",
	"sec_qc_summary",
	"sec_qc_debug",
	"sec_qc_dbg_partition",
	"nvme",
	"nvme_core",
	"bcl_pmic5",
	"c1dcvs_scmi",
	"c1dcvs_vendor",
	"qcom_rimps",
	"msm_qmp",
	"qcom_aoss",
	"stub_regulator",
	"softdog",
	"s2mpb03",
	"s2mpb02_regulator",
	"mfd_s2mpb02",
	"s2dos05_regulator",
	"pmic_class",
	"i2c_gpio",
	"sec_qc_upload_cause",
	"sec_upload_cause",
	"sec_pmsg",
	"sec_param",
	"sec_crashkey_long",
	"sec_crashkey",
	"sec_key_notifier",
	"rtc_pm8xxx",
	"qrtr",
	"qcom_reboot_reason",
	"pinctrl_spmi_gpio",
	"spmi_pmic_arb",
	"qcom_spmi_pmic",
	"regmap_spmi",
	"qti_regmap_debugfs",
	"pmu_scmi",
	"pmu_vendor",
	"qcom_pmu_lib",
	"qcom_llcc_pmu",
	"debug_symbol",
	"qcom_dload_mode",
	"arm_smmu",
	"qcom_iommu_util",
	"phy_qcom_ufs_qrbtc_sdm845",
	"phy_qcom_ufs_qmp_v4_crow",
	"phy_qcom_ufs_qmp_v4_kalama",
	"phy_qcom_ufs",
	"nvmem_qcom_spmi_sdam",
	"ns",
	"qnoc_crow",
	"qnoc_kalama",
	"qnoc_qos",
	"pinctrl_kalama",
	"pinctrl_crow",
	"pinctrl_msm",
	"memory_dump_v2",
	"mem_buf",
	"qcom_dma_heaps",
	"msm_dma_iommu_mapping",
	"mem_buf_msgq",
	"mem_buf_dev",
	"mem_hooks",
	"llcc_qcom",
	"iommu_logger",
	"gunyah",
	"mdt_loader",
	"secure_buffer",
	"gh_ctrl",
	"videocc_kalama",
	"tcsrcc_kalama",
	"dispcc_kalama",
	"dispcc_crow",
	"dcc_v2",
	"crypto_qti_common",
	"crypto_qti_hwkm",
	"hwkm",
	"tmecom_intf",
	"cqhci",
	"clk_dummy",
	"cpu_hotplug",
	"thermal_pause",
	"sched_walt",
	"qcom_cpufreq_hw",
	"sec_pm_log",
	"bwmon",
	"qcom_dcvs",
	"dcvs_fp",
	"rpmh_regulator",
	"qcom_tsens",
	"thermal_minidump",
	"qcom_pdc",
	"qcom_ipcc",
	"camcc_kalama",
	"icc_rpmh",
	"icc_debug",
	"icc_bcm_voter",
	"socinfo",
	"gcc_kalama",
	"gcc_crow",
	"clk_qcom",
	"gdsc_regulator",
	"proxy_consumer",
	"debug_regulator",
	"clk_rpmh",
	"qcom_rpmh",
	"cmd_db",
	"qcom_ipc_logging",
	"sec_debug",
	"qcom_cpu_vendor_hooks",
	"gh_virt_wdt",
	"qcom_wdt_core",
	"qcom_scm",
	"minidump",
	"gh_rm_drv",
	"gh_dbl",
	"gh_msgq",
	"gh_arm_drv",
	"smem",
	"qcom_hwspinlock",
	"abc",
	"sec_qc_logger",
	"sec_arm64_ap_context",
	"sec_debug_region",
	"sec_log_buf",
	"sec_boot_stat",
	"sec_class",

	NULL
};

/*
 * Some drivers don't have module_init(), which will lead to lookup failure
 * from lazy_initcall when a module with the same name is asked to be loaded
 * from the userspace.
 *
 * Lazy initcall can optionally maintain a list of kernel drivers built into
 * the kernel that matches the name from the firmware so that those aren't
 * treated as errors.
 *
 * Ew, is this the best approach?
 *
 * Detecting the presense of .init.text section or initcall_t function is
 * unreliable as .init.text might not exist when a driver doesn't use __init
 * and modpost adds an empty .init stub even if a driver doesn't declare a
 * function for module_init().
 *
 * This list is generatable by putting .ko modules from vendor, vendor_boot and
 * vendor_dlkm to a directory, cd'ing to kernel's O directory and running the
 * following:
 *
 * MODDIR=/path/to/modules
 * find -name '*.o' | tr '-' '_' > list
 * find "$MODDIR" -name '*.ko' -exec modinfo {} + | grep '^name:' | awk '{print $2}' | sort | uniq | while read f; do if grep -q /"$f".o list; then printf '\t"'$f'",\n'; fi; done
 */
static const __initconst char * const builtin_list[] = {
	"camera",
	"kiwi_v2",
	"ipa_clientsm",
	"ipanetm",
	"rndisipam",
	"stm_ts_spi",
	"msm_kgsl",
	"mhi_dev_uci",
	"snd_soc_cs35l45_i2c",
	"sec_qc_qcom_wdt_core",
	"ipam",
	"sec_thermistor",
	"machine_dlkm",
	"cpufreq_stats_scmi",
	"usb_notifier_qcom",
	"coresight_hwevent",
	"coresight_tmc",
	"sec_direct_charger",
	"msm_drm",
	"qdss_bridge",
	"usb_f_qdss",
	"spcom",
	"lpass_cdc_wsa_macro_dlkm",
	"cnss2",
	"lpass_cdc_tx_macro_dlkm",
	"max77705_fuelgauge",
	"lpass_cdc_rx_macro_dlkm",
	"spss_utils",
	"abc_hub",
	"usb_f_gsi",
	"flicker_sensor",
	"qcom_pon",
	"bt_fm_slim",
	"gsim",
	"lpass_cdc_wsa2_macro_dlkm",
	"lpass_cdc_va_macro_dlkm",
	"mac80211",
	"sec_tsp_dumpkey",
	"mhi_dev_dtr",
	"swr_dmic_dlkm",
	"sec_ap_pmic",
	"mhi_dev_drv",
	"snd_soc_cs35l43_i2c",
	"wlan_firmware_service",
	"isg6320",
	"uwb",
	"msm_eva",
	"mhi_dev_satellite",
	"flicker_test",
	"adsp_loader_dlkm",
	"swr_ctrl_dlkm",
	"max77705_charger",
	"coresight_tpda",
	"audio_pkt_dlkm",
	"mhi_dev_netdev",
	"qrtr_mhi",
	"cps4038_charger",
	"coresight_cti",
	"memlat_scmi",
	"pdic_max77705",
	"qcrypto_msm_dlkm",
	"sdhci_msm_sec",
	"rmnet_wlan",
	"btpower",
	"stmvl53l8",
	"rmnet_perf",
	"wcd938x_dlkm",
	"pinctrl_lpi_dlkm",
	"coresight_dummy",
	"stm_p_basic",
	"snd_soc_hdmi_codec",
	"coresight_funnel",
	"lpass_cdc_dlkm",
	"wcd9xxx_dlkm",
	"mhi_cntrl_qcom",
	"audio_prm_dlkm",
	"spf_core_dlkm",
	"gpr_dlkm",
	"snd_soc_cirrus_amp",
	"q6_notifier_dlkm",
	"sec_qc_qcom_reboot_reason",
	"mbhc_dlkm",
	"qcom_q6v5_pas",
	"qbt2000_spidev",
	"fingerprint",
	"f_fs_ipc_log",
	"rmnet_shs",
	"bcl_soc",
	"pca9481_charger",
	"cnss_utils",
	"coresight_csr",
	"wez02",
	"hdm",
	"sec_common_fn",
	"sec_qc_rbcmd",
	"rmnet_sch",
	"rmnet_aps",
	"snd_soc_cs40l26",
	"qrtr_smd",
	"input_booster_lkm",
	"coresight_remote_etm",
	"radio_i2c_rtc6226_qca",
	"usb_f_diag",
	"cnss_nl",
	"wcd938x_slave_dlkm",
	"smcinvoke_dlkm",
	"stm_console",
	"mfd_max77705",
	"coresight_tpdm",
	"sec_battery",
	"sec_qc_rst_exinfo",
	"rmnet_offload",
	"cnss_prealloc",
	"stm_p_ost",
	"hdmi_dlkm",
	"dropdump",
	"usb_f_conn_gadget",
	"dwc3_msm",
	"qti_qmi_sensor_v2",
	"synx_driver",
	"cnss_plat_ipc_qmi_svc",
	"cpufreq_limit",
	"snd_usb_audio_qmi",
	"coresight_replicator",
	"coresight_stm",
	"repeater_qti_pmic_eusb2",
	"qti_userspace_cdev",
	"qcom_cpufreq_hw_debug",
	"i2c_msm_geni",
	"sb_core",
	"smsc95xx",
	"lt9611uxc",
	"sg",
	"qcom_hv_haptics",
	"frpc_adsprpc",
	"msm_video",
	"qcom_va_minidump",
	"leds_qti_flash",
	"gh_irq_lend",
	"nfc_sec",
	"spi_msm_geni",
	"msm_sharedmem",
	"qfprom_sys",
	"qcom_iommu_debug",
	"msm_performance",
	"stub_dlkm",
	"hung_task_enh",
	"msm_geni_serial",
	"gh_tlmm_vm_mem_access",
	"qti_ocp_notifier",
	"smsc75xx",
	"wsa884x_dlkm",
	"hall_ic",
	"hdcp_qseecom_dlkm",
	"qcom_ipc_lite",
	"qcom_spmi_adc5_gen3",
	"msm_ext_display",
	"mhi_dev_net",
	"msm_show_epoch",
	"ep_pcie_drv",
	"sec_qc_hw_param",
	"qseecom_proxy",
	"sec_abc_detect_conn",
	"qrtr_gunyah",
	"snvm",
	"qpnp_amoled_regulator",
	"phy_qcom_emu",
	"sec_arm64_fsimd_debug",
	"qcom_pil_info",
	"snd_soc_wm_adsp",
	"cfg80211",
	"slim_qcom_ngd_ctrl",
	"wsa883x_dlkm",
	"sdpm_clk",
	"hwmon",
	"sec_arm64_debug",
	"fingerprint_sysfs",
	"sec_qc_soc_id",
	"msm_lmh_dcvs",
	"reboot_mode",
	"industrialio_buffer_cb",
	"ddr_cdev",
	"dev_ril_bridge",
	"qti_qmi_cdev",
	"nb7vpq904m",
	"fsa4480_i2c",
	"sec_qc_smem",
	"memlat_vendor",
	"icc_test",
	"sec_pon_alarm",
	"i3c_master_msm_geni",
	"qcom_esoc",
	"leds_qpnp_vibrator_ldo",
	"max31760_fan",
	"sec_reboot_cmd",
	"pm8941_pwrkey",
	"vibrator_vib_info",
	"cpu_voltage_cooling",
	"qcom_q6v5",
	"qcom_spmi_temp_alarm",
	"kperfmon",
	"policy_engine",
	"sec_qc_rdx_bootdev",
	"usb_f_ss_acm",
	"leds_qti_tri_led",
	"usb_f_ccid",
	"swr_haptics_dlkm",
	"usbmon",
	"nvmem_qfprom",
	"qcom_lpm",
	"phy_generic",
	"repeater_i2c_eusb2",
	"lvstest",
	"leds_s2mpb02",
	"sec_pd",
	"msm_mmrm",
	"qcom_spss",
	"mhi",
	"phy_msm_snps_eusb2",
	"sec_audio_sysfs",
	"wcd_usbss_i2c",
	"usb_f_cdev",
	"qti_battery_charger",
	"sync_fence",
	"msm_hw_fence",
	"qcom_vadc_common",
	"switch_gpio",
	"cpufreq_stats_vendor",
	"memlat",
	"audpkt_ion_dlkm",
	"gh_mem_notifier",
	"smsc",
	"redriver",
	"usb_bam",
	"rdbg",
	"qti_fixed_regulator",
	"qcedev_mod_dlkm",
	"qce50_dlkm",
	"adsp_sleepmon",
	"hall_ic_notifier",
	"phy_msm_ssusb_qmp",
	"usb_f_ss_mon_gadget",
	"sps_drv",
	"sec_input_notifier",
	"ucsi_glink",
	"rmnet_perf_tether",
	"sec_tclm_v2",
	"sec_cmd",
	"if_cb_manager",
	"sec_secure_touch",
	"sec_tsp_log",
	"core_hang_detect",
	"ehset",
	"stm_ftrace",
	"hvc_gunyah",
	"qti_battery_debug",
	"charger_ulog_glink",
	"pmic_glink_debug",
	"altmode_glink",
	"phy_msm_m31_eusb2",
	"repeater",
	"adsp_factory_module",
	"sensors_core",
	"pmic_pon_log",
	"coresight_tgu",
	"rmnet_core",
	"boot_stats",
	"swr_dlkm",
	"rmnet_ctl",
	"ipa_fmwk",
	"plh_scmi",
	"plh_vendor",
	"sys_pm_vx",
	"subsystem_sleep_stats",
	"glink_probe",
	"dmesg_dumper",
	"soc_sleep_stats",
	"cdsp_loader",
	"q6_dlkm",
	"panel_event_notifier",
	"usb_typec_manager",
	"common_muic",
	"input_cs40l26_i2c",
	"vbus_notifier",
	"rimps_log",
	"sec_panel_notifier",
	"tz_log_dlkm",
	"snd_event_dlkm",
	"stm_core",
	"wcd_core_dlkm",
	"slimbus",
	"sec_vibrator_inputff_module",
	"eud",
	"microdump_collector",
	"pdic_notifier_module",
	"cl_dsp",
	"usb_notify_layer",
	"qcom_sysmon",
	"snd_debug_proc",
	"cdsprm",
	"smp2p",
	"q6_pdr_dlkm",
	"coresight",
	"glink_pkt",
	"gpucc_crow",
	"qcom_glink_spss",
	"smp2p_sleepstate",
	"msm_memshare",
	"heap_mem_ext_v01",
	"qsee_ipc_irq_bridge",
	"switch_class",
	"qti_devfreq_cdev",
	"qcom_cpuss_sleep_stats",
	"qti_cpufreq_cdev",
	"bam_dma",
	"debugcc_crow",
	"debugcc_kalama",
	"qcom_edac",
	"phy_qcom_ufs_qmp_v4",
	"pinctrl_spmi_mpp",
	"camcc_crow",
	"phy_qcom_ufs_qmp_v4_lahaina",
	"pwm_qti_lpg",
	"videocc_crow",
	"gpucc_kalama",
	"qcom_ramdump",
	"sysmon_subsystem_stats",
	"phy_qcom_ufs_qmp_v4_waipio",
	"pmic_glink",
	"pci_msm_drv",
	"pdr_interface",
	"rproc_qcom_common",
	"qcom_smd",
	"phy_qcom_ufs_qmp_v4_khaje",
	"qcom_glink_smem",
	"qcom_glink",
	"qmi_helpers",
	"twofish_generic",
	"sec_qc_user_reset",
	"lcd",
	"msm_show_resume_irq",
	"msm_gpi",
	"msm_sysstats",
	"ssg",
	"phy_qcom_ufs_qmp_v4_kona",
	"blk_sec_stats",
	"blk_sec_common",
	"twofish_common",
	"zram",
	"spmi_pmic_arb_debug",
	"zsmalloc",
	"qrng_dlkm",
	"sec_qc_param",
	"sec_qc_summary",
	"sec_qc_debug",
	"sec_qc_dbg_partition",
	"nvme",
	"nvme_core",
	"bcl_pmic5",
	"c1dcvs_scmi",
	"c1dcvs_vendor",
	"qcom_rimps",
	"ufs_qcom",
	"ufshcd_crypto_qti",
	"msm_qmp",
	"qcom_aoss",
	"stub_regulator",
	"softdog",
	"s2mpb03",
	"s2mpb02_regulator",
	"mfd_s2mpb02",
	"s2dos05_regulator",
	"pmic_class",
	"i2c_gpio",
	"sec_qc_upload_cause",
	"sec_upload_cause",
	"sec_pmsg",
	"sec_param",
	"sec_crashkey_long",
	"sec_crashkey",
	"sec_key_notifier",
	"rtc_pm8xxx",
	"qrtr",
	"qcom_reboot_reason",
	"pinctrl_spmi_gpio",
	"spmi_pmic_arb",
	"qcom_spmi_pmic",
	"regmap_spmi",
	"qti_regmap_debugfs",
	"pmu_scmi",
	"pmu_vendor",
	"qcom_pmu_lib",
	"qcom_llcc_pmu",
	"debug_symbol",
	"qcom_dload_mode",
	"arm_smmu",
	"qcom_iommu_util",
	"phy_qcom_ufs_qrbtc_sdm845",
	"phy_qcom_ufs_qmp_v4_crow",
	"phy_qcom_ufs_qmp_v4_kalama",
	"phy_qcom_ufs",
	"nvmem_qcom_spmi_sdam",
	"ns",
	"qnoc_crow",
	"qnoc_kalama",
	"qnoc_qos",
	"pinctrl_kalama",
	"pinctrl_crow",
	"pinctrl_msm",
	"memory_dump_v2",
	"mem_buf",
	"qcom_dma_heaps",
	"msm_dma_iommu_mapping",
	"mem_buf_msgq",
	"mem_buf_dev",
	"mem_hooks",
	"llcc_qcom",
	"iommu_logger",
	"gunyah",
	"mdt_loader",
	"secure_buffer",
	"gh_ctrl",
	"videocc_kalama",
	"tcsrcc_kalama",
	"dispcc_kalama",
	"dispcc_crow",
	"dcc_v2",
	"crypto_qti_common",
	"crypto_qti_hwkm",
	"hwkm",
	"tmecom_intf",
	"cqhci",
	"clk_dummy",
	"cpu_hotplug",
	"thermal_pause",
	"sched_walt",
	"qcom_cpufreq_hw",
	"sec_pm_log",
	"bwmon",
	"qcom_dcvs",
	"dcvs_fp",
	"rpmh_regulator",
	"qcom_tsens",
	"thermal_minidump",
	"qcom_pdc",
	"qcom_ipcc",
	"camcc_kalama",
	"icc_rpmh",
	"icc_debug",
	"icc_bcm_voter",
	"socinfo",
	"gcc_kalama",
	"gcc_crow",
	"clk_qcom",
	"gdsc_regulator",
	"proxy_consumer",
	"debug_regulator",
	"clk_rpmh",
	"qcom_rpmh",
	"cmd_db",
	"qcom_ipc_logging",
	"sec_debug",
	"qcom_cpu_vendor_hooks",
	"gh_virt_wdt",
	"qcom_wdt_core",
	"qcom_scm",
	"minidump",
	"gh_rm_drv",
	"gh_dbl",
	"gh_msgq",
	"gh_arm_drv",
	"smem",
	"qcom_hwspinlock",
	"abc",
	"sec_qc_logger",
	"sec_arm64_ap_context",
	"sec_debug_region",
	"sec_log_buf",
	"sec_boot_stat",
	"sec_class",

	NULL,
};

/*
 * Some drivers behave differently when it's built-in, so deferring its
 * initialization causes issues.
 *
 * Put those to this blacklist so that it is ignored from lazy_initcall.
 *
 * You can also use this as an ignorelist.
 */
static const __initconst char * const blacklist[] = {
	NULL
};

/*
 * You may want some specific drivers to load after all lazy modules have been
 * loaded.
 *
 * Add them here.
 */
static const __initconst char * const deferred_list[] = {
	"ufshcd_crypto_qti",
	"ufs_qcom",

	NULL
};

static struct lazy_initcall __initdata lazy_initcalls[ARRAY_SIZE(targets_list) - ARRAY_SIZE(blacklist) + ARRAY_SIZE(deferred_list)];
static int __initdata counter;

bool __init add_lazy_initcall(initcall_t fn, char modname[], char filename[])
{
	int i;
	bool match = false;
	enum lazy_initcall_type type = NORMAL;

	for (i = 0; blacklist[i]; i++) {
		if (!strcmp(blacklist[i], modname))
			return false;
	}

	for (i = 0; targets_list[i]; i++) {
		if (!strcmp(targets_list[i], modname)) {
			match = true;
			break;
		}
	}

	for (i = 0; deferred_list[i]; i++) {
		if (!strcmp(deferred_list[i], modname)) {
			match = true;
			type = DEFERRED;
			break;
		}
	}

	if (!match)
		return false;

	mutex_lock(&lazy_initcall_mutex);

	pr_debug("adding lazy_initcalls[%d] from %s - %s\n",
				counter, modname, filename);

	lazy_initcalls[counter].fn = fn;
	lazy_initcalls[counter].modname = modname;
	lazy_initcalls[counter].filename = filename;
	lazy_initcalls[counter].type = type;
	counter++;

	mutex_unlock(&lazy_initcall_mutex);

	return true;
}

static char __initdata errors_str[16 * 1024];

#define __err(...) do { \
	size_t len = strlen(errors_str); \
	char *ptr = errors_str + len; \
	snprintf(ptr, sizeof(errors_str) - len, __VA_ARGS__); \
	smp_mb(); \
	pr_err("%s", ptr); \
} while (0)

static bool __init show_errors_str(void)
{
	char *s, *p, *tok;

	if (strlen(errors_str) == 0)
		return false;

	s = kstrdup(errors_str, GFP_KERNEL);
	if (!s)
		return true;

	for (p = s; (tok = strsep(&p, "\n")) != NULL; )
		if (tok[0] != '\0')
			pr_err("%s\n", tok);

	kfree(s);

	return true;
}

static void __init show_errors(struct work_struct *unused)
{
	int i;

	// Start printing only after 30s of uptime
	if (ktime_to_us(ktime_get_boottime()) < 30 * USEC_PER_SEC)
		goto out;

	show_errors_str();

	for (i = 0; i < counter; i++) {
		if (!lazy_initcalls[i].loaded) {
			pr_err("lazy_initcalls[%d]: %s not loaded yet\n", i, lazy_initcalls[i].modname);
		}
	}

out:
	queue_delayed_work(system_freezable_power_efficient_wq,
			&show_errors_work, 5 * HZ);
}

static int __init unknown_integrated_module_param_cb(char *param, char *val,
					      const char *modname, void *arg)
{
	__err("%s: unknown parameter '%s' ignored\n", modname, param);
	return 0;
}

static int __init integrated_module_param_cb(char *param, char *val,
				      const char *modname, void *arg)
{
	size_t nlen, plen, vlen;
	char *modparam;

	nlen = strlen(modname);
	plen = strlen(param);
	vlen = val ? strlen(val) : 0;
	if (vlen)
		/* Parameter formatted as "modname.param=val" */
		modparam = kmalloc(nlen + plen + vlen + 3, GFP_KERNEL);
	else
		/* Parameter formatted as "modname.param" */
		modparam = kmalloc(nlen + plen + 2, GFP_KERNEL);
	if (!modparam) {
		pr_err("%s: allocation failed for module '%s' parameter '%s'\n",
		       __func__, modname, param);
		return 0;
	}

	/* Construct the correct parameter name for the built-in module */
	memcpy(modparam, modname, nlen);
	modparam[nlen] = '.';
	memcpy(&modparam[nlen + 1], param, plen);
	if (vlen) {
		modparam[nlen + 1 + plen] = '=';
		memcpy(&modparam[nlen + 1 + plen + 1], val, vlen);
		modparam[nlen + 1 + plen + 1 + vlen] = '\0';
	} else {
		modparam[nlen + 1 + plen] = '\0';
	}

	/* Now have parse_args() look for the correct parameter name */
	parse_args(modname, modparam, __start___param,
		   __stop___param - __start___param,
		   -32768, 32767, NULL,
		   unknown_integrated_module_param_cb);
	kfree(modparam);
	return 0;
}

static noinline void __init load_modname(const char * const modname, const char __user *uargs)
{
	int i, ret;
	bool match = false;
	char *args;
	initcall_t fn;

	pr_debug("trying to load \"%s\"\n", modname);

	// Check if the driver is blacklisted (built-in, but not lazy-loaded)
	for (i = 0; blacklist[i]; i++) {
		if (!strcmp(blacklist[i], modname)) {
			pr_debug("\"%s\" is blacklisted (not lazy-loaded)\n", modname);
			return;
		}
	}

	// Find the function pointer from lazy_initcalls[]
	for (i = 0; i < counter; i++) {
		if (!strcmp(lazy_initcalls[i].modname, modname)) {
			fn = lazy_initcalls[i].fn;
			if (lazy_initcalls[i].loaded) {
				pr_debug("lazy_initcalls[%d]: %s already loaded\n", i, modname);
				return;
			}
			lazy_initcalls[i].loaded = true;
			match = true;
			break;
		}
	}

	// Unable to find the driver that the userspace requested
	if (!match) {
		// Check if this module is built-in without module_init()
		for (i = 0; builtin_list[i]; i++) {
			if (!strcmp(builtin_list[i], modname))
				return;
		}
		__fatal("failed to find a built-in module with the name \"%s\"\n", modname);
		return;
	}

	// Setup args
	if (uargs != NULL) {
		args = strndup_user(uargs, ~0UL >> 1);
		if (IS_ERR(args)) {
			pr_err("failed to parse args: %d\n", PTR_ERR(args));
		} else {
			/*
			 * Parameter parsing is done in two steps for integrated modules
			 * because built-in modules have their parameter names set as
			 * "modname.param", which means that each parameter name in the
			 * arguments must have "modname." prepended to it, or it won't
			 * be found.
			 *
			 * Since parse_args() has a lot of complex logic for actually
			 * parsing out arguments, do parsing in two parse_args() steps.
			 * The first step just makes parse_args() parse out each
			 * parameter/value pair and then pass it to
			 * integrated_module_param_cb(), which builds the correct
			 * parameter name for the built-in module and runs parse_args()
			 * for real. This means that parse_args() recurses, but the
			 * recursion is fixed because integrated_module_param_cb()
			 * passes a different unknown handler,
			 * unknown_integrated_module_param_cb().
			 */
			if (*args)
				parse_args(modname, args, NULL, 0, 0, 0, NULL,
					   integrated_module_param_cb);
		}
	}

	ret = fn();
	if (ret != 0)
		__err("lazy_initcalls[%d]: %s's init function returned %d\n", i, modname, ret);

	// Check if all modules are loaded so that __init memory can be released
	match = false;
	for (i = 0; i < counter; i++) {
		if (lazy_initcalls[i].type == NORMAL && !lazy_initcalls[i].loaded)
			match = true;
	}

	if (!match)
		cancel_delayed_work_sync(&show_errors_work);
	else
		queue_delayed_work(system_freezable_power_efficient_wq,
				&show_errors_work, 5 * HZ);

	if (!match)
		WRITE_ONCE(completed, true);

	return;
}

static noinline int __init __load_module(struct load_info *info, const char __user *uargs,
		       int flags)
{
	long err;

	/*
	 * Do basic sanity checks against the ELF header and
	 * sections.
	 */
	err = elf_validity_check(info);
	if (err) {
		pr_err("Module has invalid ELF structures\n");
		goto err;
	}

	/*
	 * Everything checks out, so set up the section info
	 * in the info structure.
	 */
	err = setup_load_info(info, flags);
	if (err)
		goto err;

	load_modname(info->name, uargs);

err:
	free_copy(info);
	return err;
}

static int __ref load_module(struct load_info *info, const char __user *uargs,
		       int flags)
{
	int i, ret = 0;

	mutex_lock(&lazy_initcall_mutex);

	if (completed) {
		// Userspace may ask even after all modules have been loaded
		goto out;
	}

	ret = __load_module(info, uargs, flags);
	smp_wmb();

	if (completed) {
		if (deferred_list[0] != NULL) {
			pr_info("all userspace modules loaded, now loading built-in deferred drivers\n");

			for (i = 0; deferred_list[i]; i++)
				load_modname(deferred_list[i], NULL);
		}
		pr_info("all modules loaded, calling free_initmem()\n");
		if (show_errors_str())
			WARN(1, "all modules loaded with errors, review if necessary");
		free_initmem();
		mark_readonly();
	}

out:
	mutex_unlock(&lazy_initcall_mutex);
	return ret;
}

static int may_init_module(void)
{
	if (!capable(CAP_SYS_MODULE))
		return -EPERM;

	return 0;
}

SYSCALL_DEFINE3(init_module, void __user *, umod,
		unsigned long, len, const char __user *, uargs)
{
	int err;
	struct load_info info = { };

	err = may_init_module();
	if (err)
		return err;

	err = copy_module_from_user(umod, len, &info);
	if (err)
		return err;

	return load_module(&info, uargs, 0);
}

SYSCALL_DEFINE3(finit_module, int, fd, const char __user *, uargs, int, flags)
{
	struct load_info info = { };
	void *hdr = NULL;
	int err;

	err = may_init_module();
	if (err)
		return err;

	err = kernel_read_file_from_fd(fd, 0, &hdr, INT_MAX, NULL,
				       READING_MODULE);
	if (err < 0)
		return err;
	info.hdr = hdr;
	info.len = err;

	return load_module(&info, uargs, flags);
}
