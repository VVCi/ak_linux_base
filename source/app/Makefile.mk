-include source/app/mqtt/Makefile.mk
-include source/app/interfaces/Makefile.mk

CXXFLAGS	+= -I./source/app

VPATH += source/app

OBJ += $(OBJ_DIR)/app.o
OBJ += $(OBJ_DIR)/app_data.o
OBJ += $(OBJ_DIR)/app_config.o
OBJ += $(OBJ_DIR)/shell.o

OBJ += $(OBJ_DIR)/task_list.o
OBJ += $(OBJ_DIR)/task_console.o
OBJ += $(OBJ_DIR)/task_if.o
OBJ += $(OBJ_DIR)/task_debug_msg.o
OBJ += $(OBJ_DIR)/task_sensor.o
OBJ += $(OBJ_DIR)/task_fw.o
OBJ += $(OBJ_DIR)/task_sys.o
