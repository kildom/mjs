
WITH_PARSER=n
WITH_JSON=n

TARGET = test.exe

OBJ = test.o
#OBJ += common\cs_dbg.o
#OBJ += common\cs_dirent.o
#OBJ += common\cs_file.o
#OBJ += common\cs_time.o
OBJ += common\cs_varint.o
OBJ += common\mbuf.o
OBJ += common\mg_str.o
OBJ += common\str_util.o
#OBJ += common\test_util.o
#OBJ += common\platforms\arm\arm_exc.o
#OBJ += common\platforms\arm\arm_nsleep100.o
#OBJ += common\platforms\cc3200\cc3200_libc.o
#OBJ += common\platforms\esp\slip.o
#OBJ += common\platforms\esp\stub_flasher.o
#OBJ += common\platforms\esp31\rom\notes.o
#OBJ += common\platforms\esp32\stubs\led.o
#OBJ += common\platforms\esp32\stubs\stub_hello.o
#OBJ += common\platforms\esp32\stubs\uart.o
#OBJ += common\platforms\esp8266\esp_crypto.o
#OBJ += common\platforms\esp8266\esp_umm_malloc.o
#OBJ += common\platforms\esp8266\rboot\esptool2\esptool2.o
#OBJ += common\platforms\esp8266\rboot\esptool2\esptool2_elf.o
#OBJ += common\platforms\esp8266\rboot\rboot\rboot-stage2a.o
#OBJ += common\platforms\esp8266\rboot\rboot\rboot.o
#OBJ += common\platforms\esp8266\rboot\rboot\appcode\rboot-api.o
#OBJ += common\platforms\esp8266\rboot\rboot\appcode\rboot-bigflash.o
#OBJ += common\platforms\esp8266\stubs\stub_hello.o
#OBJ += common\platforms\esp8266\stubs\uart.o
#OBJ += common\platforms\lwip\mg_lwip_ev_mgr.o
#OBJ += common\platforms\lwip\mg_lwip_net_if.o
#OBJ += common\platforms\mbed\mbed_libc.o
#OBJ += common\platforms\msp432\msp432_libc.o
#OBJ += common\platforms\nrf5\nrf5_libc.o
#OBJ += common\platforms\pic32\pic32_net_if.o
#OBJ += common\platforms\simplelink\sl_fs.o
#OBJ += common\platforms\simplelink\sl_fs_slfs.o
#OBJ += common\platforms\simplelink\sl_mg_task.o
#OBJ += common\platforms\simplelink\sl_net_if.o
#OBJ += common\platforms\simplelink\sl_socket.o
#OBJ += common\platforms\simplelink\sl_ssl_if.o
#OBJ += common\platforms\wince\wince_libc.o
#OBJ += common\platforms\windows\windows_direct.o
OBJ += frozen\frozen.o
OBJ += mjs\src\mjs_array.o
OBJ += mjs\src\mjs_bcode.o
OBJ += mjs\src\mjs_builtin.o
OBJ += mjs\src\mjs_conversion.o
OBJ += mjs\src\mjs_core.o
#OBJ += mjs\src\mjs_dataview.o
OBJ += mjs\src\mjs_exec.o
OBJ += mjs\src\mjs_ffi.o
OBJ += mjs\src\mjs_gc.o
#OBJ += mjs\src\mjs_main.o
OBJ += mjs\src\mjs_object.o
OBJ += mjs\src\mjs_primitive.o
OBJ += mjs\src\mjs_string.o
OBJ += mjs\src\mjs_util.o
OBJ += mjs\src\ffi\ffi.o
#OBJ += mjs\src\ffi\ffi_test.o

INCLUDE = -I. -Imjs/src -Ifrozen
CFLAGS  = -fdata-sections -ffunction-sections -Wl,--gc-sections -Wall -g -O0 -Wno-unused-function -Wno-unused-but-set-variable
CFLAGS += -DCS_PLATFORM=0 -DMJS_EXPOSE_PRIVATE -DCS_ENABLE_STDIO=0 -DJSON_MINIMAL=1
# CFLAGS = -Wl,-Map,test.map -Os -g -D__CRT__NO_INLINE -D__MINGW32__ -D_MSC_VER=1900 -fdata-sections -ffunction-sections -Wl,--gc-sections

ifeq ($(WITH_PARSER),y)
  OBJ += mjs\src\mjs_parser.o 
  OBJ += mjs\src\mjs_tok.o
  CFLAGS += -DWITH_PARSER
endif

ifeq ($(WITH_JSON),y)
  OBJ += mjs\src\mjs_json.o
  CFLAGS += -DWITH_JSON
endif

all: $(TARGET)

clean:
	rm -f $(TARGET) $(OBJ)

.c.o:
	gcc -c $(CFLAGS) $(INCLUDE) -o $@ $<

$(TARGET): $(OBJ)
	gcc $(CFLAGS) -Wl,-Map,$(TARGET).map -o $@ $(OBJ)
