# SPDX-License-Identifier: GPL-2.0-only


MM_ROOT=$(ROOTDIR)display/vendor/qcom/opensource/mm-drivers
KBUILD_OPTIONS := MM_ROOT=$(MM_ROOT)

ifeq ($(TARGET_SUPPORT),genericarmv8)
	KBUILD_OPTIONS += CONFIG_ARCH_KALAMA=y
endif


obj-m += hw_fence/
obj-m += msm_ext_display/
obj-m += sync_fence/


all:
	$(MAKE) -C $(KERNEL_SRC) M=$(shell pwd) modules $(KBUILD_OPTIONS)

modules_install:
	$(MAKE) INSTALL_MOD_STRIP=1 -C $(KERNEL_SRC) M=$(shell pwd) modules_install


clean:
	rm -f *.o *.ko *.mod.c *.mod.o *~ .*.cmd Module.symvers
	rm -rf .tmp_versions
