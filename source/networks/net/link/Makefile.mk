CXXFLAGS	+= -I./source/networks/net/link

VPATH += source/networks/net/link

OBJ += $(OBJ_DIR)/link.o
OBJ += $(OBJ_DIR)/link_mac.o
OBJ += $(OBJ_DIR)/link_phy.o
OBJ += $(OBJ_DIR)/link_data.o
