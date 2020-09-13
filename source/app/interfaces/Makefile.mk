CXXFLAGS	+= -I./source/app/interfaces

VPATH += source/app/interfaces

OBJ += $(OBJ_DIR)/if_console.o
OBJ += $(OBJ_DIR)/if_rf433.o
OBJ += $(OBJ_DIR)/if_rf24.o
OBJ += $(OBJ_DIR)/if_app.o
OBJ += $(OBJ_DIR)/if_cpu_serial.o
OBJ += $(OBJ_DIR)/if_usb_stick_rf24.o
