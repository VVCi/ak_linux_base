CXXFLAGS	+= -I./source/common

VPATH += source/common

OBJ += $(OBJ_DIR)/cmd_line.o
OBJ += $(OBJ_DIR)/jsmn.o
OBJ += $(OBJ_DIR)/firmware.o
OBJ += $(OBJ_DIR)/base64.o
OBJ += $(OBJ_DIR)/fifo.o
