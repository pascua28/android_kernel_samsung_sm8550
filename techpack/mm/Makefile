# SPDX-License-Identifier: GPL-2.0-only


MM_ROOT=$(KERNEL_SRC)/techpack/mm
KBUILD_OPTIONS := MM_ROOT=$(MM_ROOT)

ifeq ($(TARGET_SUPPORT),genericarmv8)
	KBUILD_OPTIONS += CONFIG_ARCH_KALAMA=y
endif


obj-y += hw_fence/
obj-y += msm_ext_display/
obj-y += sync_fence/


all:
	$(MAKE) -C $(KERNEL_SRC) M=$(shell pwd) modules $(KBUILD_OPTIONS)

modules_install:
	$(MAKE) INSTALL_MOD_STRIP=1 -C $(KERNEL_SRC) M=$(shell pwd) modules_install


clean:
	rm -f *.o *.ko *.mod.c *.mod.o *~ .*.cmd Module.symvers
	rm -rf .tmp_versions
