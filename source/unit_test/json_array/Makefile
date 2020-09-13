TOOLS_DIR       =
NAME_MODULE     = ut_json_array
OPTIMIZE        = -g -Os
CXX             = g++
CC              = gcc
OBJ_DIR         = build_$(NAME_MODULE)

CXXFLAGS        += -I/usr/local/include
CXXFLAGS        += -I/usr/include
CXXFLAGS        += -I../..

-include Include.mk

# CXX compiler option
CXXFLAGS        +=      $(OPTIMIZE)     \
                -std=c++11              \
                -Wall                   \
                -Winline                \
                -pipe                   \
                -g                      \

# Library paths
LDFLAGS += -L/usr/local/lib
LDFLAGS += -L/usr/include
LDFLAGS += -Wl,-Map=$(OBJ_DIR)/$(NAME_MODULE).map

#Library libs
LDLIBS  +=      -lpthread               \

all: create $(OBJ_DIR)/$(NAME_MODULE)

create:
	@echo mkdir -p $(OBJ_DIR)
	@mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/%.o: %.cpp
	@echo CXX $<
	@$(CXX) -c -o $@ $< $(CXXFLAGS) $(LDFLAGS) $(LDLIBS)

$(OBJ_DIR)/%.o: %.c
	@echo CXX $<
	@$(CC) -c -o $@ $< $(CXXFLAGS) $(LDFLAGS) $(LDLIBS)

$(OBJ_DIR)/$(NAME_MODULE): $(OBJ)
	@echo ---------- START LINK PROJECT ----------
	@echo $(CXX) -o $@ $^ $(CXXFLAGS)
	@$(CXX) -o $@ $^ $(CXXFLAGS) $(LDFLAGS) $(LDLIBS)

flash:
	@$(OBJ_DIR)/$(NAME_MODULE)
clean:
	@echo rm -rf $(OBJ_DIR)
	@rm -rf $(OBJ_DIR)

