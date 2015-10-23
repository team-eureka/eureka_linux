obj-y 	:= irq.o time.o gpio.o wdt_reset.o mv88de31xx_android.o hal_dhub.o
obj-$(CONFIG_BERLIN2CD) += berlin2cd-dt.o
obj-$(CONFIG_PM_OPP) += opp.o
obj-$(CONFIG_SMP) += platsmp.o headsmp.o

obj-$(CONFIG_HOTPLUG_CPU) += hotplug.o
obj-$(CONFIG_BERLIN_PXA3XX_NFC) += modules/nfc/
ifeq ($(CONFIG_BERLIN_PE)y, y)
obj-$(CONFIG_BERLIN_SHM) += modules/shm_amp/
obj-$(CONFIG_BERLIN_AMP) += modules/amp/
obj-$(CONFIG_BERLIN_TZDD) += modules/tzdd/
obj-$(CONFIG_BERLIN_TZDD_CB) += modules/tzdd_cb/
obj-$(CONFIG_BERLIN_FASTLOGO) += modules/fastlogo/
obj-$(CONFIG_BERLIN2CDP) += berlin.o
obj-$(CONFIG_BERLIN2CDP) += mv88de3006_bootinfo.o
else
obj-$(CONFIG_BERLIN2CD) += clock.o
obj-$(CONFIG_BERLIN2CD) += core.o
obj-$(CONFIG_BERLIN_SHM) += modules/shm/
obj-$(CONFIG_BERLIN_PE) += modules/pe/
obj-$(CONFIG_BERLIN_FASTLOGO) += modules/fastlogo_bg2cd/
endif
obj-$(CONFIG_BERLIN_CC) += modules/cc/
obj-$(CONFIG_BERLIN_GPIO) += modules/gpio/
obj-$(CONFIG_BERLIN_PWM) += modules/pwm/
obj-$(CONFIG_BERLIN_I2C) += modules/i2c/
obj-$(CONFIG_BERLIN_SDIO_WLAN_8787) += modules/wlan_sd8787/
obj-$(CONFIG_BERLIN_SDIO_WLAN_8797) += modules/wlan_sd8797/
obj-$(CONFIG_BERLIN_SDIO_WLAN_8801) += modules/wlan_sd8801/
obj-$(CONFIG_BERLIN_SDIO_WLAN_8887) += modules/wlan_sd8887/
obj-$(CONFIG_BERLIN_SDIO_BT_8887) += modules/bt_sd8887/
obj-$(CONFIG_BERLIN_SDIO_BT_8887_CHAR_DRV) += modules/bt_sd8887/
#obj-$(CONFIG_BERLIN_PE) += modules/amp/
#obj-$(CONFIG_BERLIN_FASTLOGO) += modules/fastlogo/
obj-$(CONFIG_BERLIN_GPU) += modules/gpu/
obj-$(CONFIG_BERLIN_GPU3D) += modules/gpu3D/



ccflags-y       += -I$(srctree)/arch/arm/mach-berlin/include/mach
