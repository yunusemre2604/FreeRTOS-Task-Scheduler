# Makefile - FreeRTOS PC Scheduler (WSL/Linux)

CC = gcc
BUILD_DIR = build
SRC_DIR = src

# Klasör Yolları (Senin yapına göre ayarlandı)
FREERTOS_DIR = FreeRTOS
FREERTOS_SRC = $(FREERTOS_DIR)/source
FREERTOS_INC = $(FREERTOS_DIR)/include
FREERTOS_PORT = $(FREERTOS_DIR)/portable/ThirdParty/GCC/Posix
FREERTOS_MEM  = $(FREERTOS_DIR)/portable/MemMang

# Derleyici Bayrakları
# -pthread: Thread desteği şart
# -I: Header dosyalarının yolları
CFLAGS = -Wall -Wextra -g -pthread -DprojCOVERAGE_TEST=0
CFLAGS += -I$(SRC_DIR) 
CFLAGS += -I$(FREERTOS_INC) 
CFLAGS += -I$(FREERTOS_PORT) 
CFLAGS += -I$(FREERTOS_PORT)/utils 
CFLAGS += -I.

# Kaynak Dosyalar (.c)
SRCS = $(SRC_DIR)/main.c \
       $(SRC_DIR)/scheduler.c \
       $(SRC_DIR)/tasks.c \
       $(FREERTOS_SRC)/tasks.c \
       $(FREERTOS_SRC)/list.c \
       $(FREERTOS_SRC)/queue.c \
       $(FREERTOS_SRC)/timers.c \
       $(FREERTOS_SRC)/event_groups.c \
       $(FREERTOS_SRC)/stream_buffer.c \
       $(FREERTOS_SRC)/croutine.c \
       $(FREERTOS_PORT)/port.c \
       $(FREERTOS_PORT)/utils/wait_for_event.c \
       $(FREERTOS_MEM)/heap_3.c

# Object Dosyaları
OBJS = $(SRCS:%.c=$(BUILD_DIR)/%.o)
TARGET = freertos_sim

# Derleme Kuralları
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

.PHONY: clean all