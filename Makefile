PROGRAM = esp8266-homekit-ir-tv-remote

EXTRA_COMPONENTS = extras/http-parser $(abspath components/wolfssl) $(abspath components/cjson) $(abspath components/homekit) $(abspath components/ir) extras/dhcpserver $(abspath components/wifi-config)

FLASH_SIZE ?= 32

EXTRA_CFLAGS += -DHOMEKIT_SHORT_APPLE_UUIDS

LIBS += m

include $(SDK_PATH)/common.mk

monitor:
	$(FILTEROUTPUT) --port $(ESPPORT) --baud 115200 --elf $(PROGRAM_OUT)
