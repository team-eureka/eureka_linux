
# __LINUX_KERNEL__ is used in OS-independent header files
MV_DEFINE := -DBERLIN -DSoC_Galois -D__LINUX_KERNEL__ -D__CODE_LINK__=0 -DCPUINDEX=0
MV_DEFINE += -DBERLIN_B_0=0x1000 -DBERLIN_C_0=0x2000 -DBERLIN_BG2=0x3000 -DBERLIN_BG2_Z2=0x3010 -DBERLIN_BG2_A0=0x3020 -DBERLIN_BG2_CD=0x3030 -DBERLIN_BG2CDP=0x3040 -DBERLIN_BG2_CT=0x3050 -DBERLIN_BG2_Q=0x3060 -DBERLIN_BG2_DTV=0x3070
MV_DEFINE += -DBERLIN_BG2CDP_A0_EXT=0xA0

# define BERLIN_CHIP_VERSION
ifeq ($(CONFIG_BERLIN2CT),y)
        MV_DEFINE += -DBERLIN_CHIP_VERSION=BERLIN_BG2_CT
endif
ifeq ($(CONFIG_ARCH_BERLIN2),y)
        MV_DEFINE += -DBERLIN_CHIP_VERSION=BERLIN_BG2_A0
endif
ifeq ($(CONFIG_BERLIN2Q),y)
        MV_DEFINE += -DBERLIN_CHIP_VERSION=BERLIN_BG2_Q
endif
ifeq ($(CONFIG_BERLIN2DTV),y)
        MV_DEFINE += -DBERLIN_CHIP_VERSION=BERLIN_BG2_DTV
endif
ifeq ($(CONFIG_BERLIN2CD),y)
        MV_DEFINE += -DBERLIN_CHIP_VERSION=BERLIN_BG2_CD
endif
ifeq ($(CONFIG_BERLIN2CDP),y)
        MV_DEFINE += -DBERLIN_CHIP_VERSION=BERLIN_BG2CDP
endif

HEADER_PATH += -I$(srctree)/arch/arm/mach-berlin/modules/cc

# separate shm for PE/AMP
ifeq ($(CONFIG_BERLIN_PE), y)
HEADER_PATH += -I$(srctree)/arch/arm/mach-berlin/modules/shm
else
HEADER_PATH += -I$(srctree)/arch/arm/mach-berlin/modules/shm_amp
endif

# use cdp header
ifeq ($(CONFIG_BERLIN2CDP),y)
HEADER_PATH +=	-I$(srctree)/arch/arm/mach-berlin/modules/amp/asinc/bg2cdp
endif

ifeq ($(CONFIG_BERLIN2CD),y)
HEADER_PATH +=	-I$(srctree)/arch/arm/mach-berlin/modules/amp/asinc/bg2cd
endif

ccflags-y	+= $(MV_DEFINE) $(HEADER_PATH)

ccflags-y	+= -DBERLIN_SINGLE_CPU

ifeq ($(CONFIG_BERLIN_FASTLOGO),y)
ccflags-y += -DBERLIN_BOOTLOGO
ccflags-y += -DCONFIG_BERLIN_FASTLOGO
endif

obj-$(CONFIG_BERLIN_AMP) += amp_core.o
amp_core-objs := amp_driver.o avio_dhub_drv.o vmeta_sched_driver.o amp_dev_snd.o
