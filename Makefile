CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -Werror -std=c99
LDFLAGS = -lmodbus -lpthread

SRC_DIR = src
OBJ_DIR = obj

# Common sources and objects
COMMON_SRCS = $(SRC_DIR)/ComAndDataProcessor.c \
              $(SRC_DIR)/Scheduler.c \
              $(SRC_DIR)/simpleConfigParser/simpleConfigParser.c

COMMON_OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(COMMON_SRCS))

# piModbusMaster
MASTER_SRCS = $(SRC_DIR)/piModbusMaster.c \
              $(SRC_DIR)/ModbusMasterThread.c \
              $(COMMON_SRCS)
MASTER_OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(MASTER_SRCS))
MASTER_BIN = piModbusMaster

# piModbusSlave
SLAVE_SRCS = $(SRC_DIR)/piModbusSlave.c \
             $(SRC_DIR)/ModbusSlaveThread.c \
             $(COMMON_SRCS)
SLAVE_OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SLAVE_SRCS))
SLAVE_BIN = piModbusSlave

.PHONY: all clean

all: $(MASTER_BIN) $(SLAVE_BIN)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(MASTER_BIN): $(MASTER_OBJS)
	$(CC) $^ $(LDFLAGS) -o $@

$(SLAVE_BIN): $(SLAVE_OBJS)
	$(CC) $^ $(LDFLAGS) -o $@

clean:
	rm -rf $(OBJ_DIR) $(MASTER_BIN) $(SLAVE_BIN)
