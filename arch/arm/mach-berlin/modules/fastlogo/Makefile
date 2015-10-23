MWD=$(srctree)/arch/arm/mach-berlin
include $(MWD)/mv88de3100.mk

PWD=$(MWD)/modules/fastlogo
HEADER_PATH :=  \
	-I$(MWD)/include \
	-I$(MWD)/include/mach \
	-I$(MWD)/modules/shm_amp \
	-I$(PWD) \
	-I$(PWD)/inc_bg2cdp

ccflags-y	+= -DLOGO_ENABLE_MAIN=1 -DLOGO_ENABLE_PIP=0 -DLOGO_ENABLE_AUX=0
ccflags-y	+= -DLOGO_PROC_FS=1 -DLOGO_TIME_PROFILE=1
ifeq ($(CONFIG_BERLIN_SHM),y)
ccflags-y	+= -DLOGO_USE_SHM=1
endif
ccflags-y	+= $(MV_DEFINE) $(HEADER_PATH)

ccflags-y 	+= -I$(PWD)/THINVPP/include
ccflags-y 	+= -I$(PWD)/THINVPP/source/include
ccflags-y 	+= -I$(PWD)/THINVPP/source/MISC/include
asflags-y 	+= -I$(PWD)


FASTLOGO_OBJ += fastlogo_soc.o
FASTLOGO_OBJ += THINVPP/source/thinvpp_api.o
FASTLOGO_OBJ += THINVPP/source/thinvpp_apifuncs.o
FASTLOGO_OBJ += THINVPP/source/thinvpp_isr.o
FASTLOGO_OBJ += THINVPP/source/MISC/source/thinvpp_bcmbuf.o
#FASTLOGO_OBJ += DHUB/source/avio_dhub_drv.o

obj-$(CONFIG_BERLIN_FASTLOGO) += galois_fastlogo.o
galois_fastlogo-objs := fastlogo_driver.o $(FASTLOGO_OBJ)
