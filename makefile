ifeq ($(DEBUG), 1)
BUILD_TARGET := debug
else
BUILD_TARGET := release
endif

SOURCE_DIR   := src
RESOURCE_DIR := res
LIB_DIR      := lib
BUILD_DIR    := build

BUILD_TARGET_DIR := $(BUILD_DIR)/$(BUILD_TARGET)
BUILD_OBJ_DIR    := $(BUILD_TARGET_DIR)/obj
TARGET           := $(BUILD_TARGET_DIR)/bin/$(notdir $(CURDIR)).exe

################################################################################

CXX     := x86_64-w64-mingw32-g++
WINDRES := x86_64-w64-mingw32-windres

################################################################################

CXXFLAGS := -std=c++20 -fpermissive \
            -MMD -MP \
            -Wall -Wextra \
            -Wno-cast-function-type \
            -Wno-unused-parameter \
            -Wno-unused-variable \
            -Wno-unused-function

CXXDEFINES  := -DUNICODE -D_UNICODE

CXXINCLUDES := -I$(SOURCE_DIR) -Iinclude \
               -I$(LIB_DIR)/argparse/include

LDFLAGS     := -static -static-libgcc -static-libstdc++ -mwindows \
               -lmsvcrt -lcomdlg32 -lgdi32 -luser32 -lshell32 -lpowrprof

ifeq ($(DEBUG), 1)
CXXFLAGS     += -g -O0
CXXDEFINES   += -DDEBUG
else
CXXFLAGS     += -O3 -flto -ffunction-sections -fdata-sections
CXXDEFINES   += -DRELEASE
LDFLAGS      += -s -Wl,--gc-sections
endif

################################################################################

RES_STAMP     := $(BUILD_OBJ_DIR)/$(RESOURCE_DIR)/res.stamp
RES_HASH      := $(BUILD_OBJ_DIR)/$(RESOURCE_DIR)/current.md5
RES_HASH_TEMP := $(BUILD_OBJ_DIR)/$(RESOURCE_DIR)/temp.md5

SOURCES   := $(shell find $(SOURCE_DIR) -type f -name "*.cpp")
RC_FILE   := $(shell find $(RESOURCE_DIR) -type f -name "*.rc")
RESOURCES := $(shell find $(RESOURCE_DIR) -type f)

OBJECTS := $(SOURCES:%.cpp=$(BUILD_OBJ_DIR)/%.o)
OBJECTS += $(RC_FILE:%.rc=$(BUILD_OBJ_DIR)/%.rc.o)

DEPS := $(OBJECTS:.o=.d)

################################################################################

.PHONY: all clean _res $(DEPS)

all: _res $(TARGET)

_res: $(RESOURCES)
	@mkdir -p $(BUILD_TARGET_DIR)
	@mkdir -p $(dir $(RES_STAMP))
	@md5sum $(RESOURCES) | sort > $(RES_HASH_TEMP)
	@if [ ! -f $(RES_HASH) ] || [ "$$(md5sum $(RES_HASH) | awk '{print $$1}')" != "$$(md5sum $(RES_HASH_TEMP) | awk '{print $$1}')" ]; then \
		cp $(RES_HASH_TEMP) $(RES_HASH); \
		touch $(RES_STAMP); \
	fi

$(TARGET): $(OBJECTS) $(LIBS) $(DEPS)
	@mkdir -p $(shell dirname $@)
	$(CXX) -o $@ $(OBJECTS) $(LDFLAGS)

$(BUILD_OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(shell dirname $@)
	$(CXX) $(CXXFLAGS) $(CXXDEFINES) $(CXXINCLUDES) -c $< -o $@

$(BUILD_OBJ_DIR)/%.rc.o: $(RC_FILE) $(RES_STAMP)
	@mkdir -p $(shell dirname $@)
	$(WINDRES) $(CXXDEFINES) $(CXXINCLUDES) $< $@

clean:
	rm -rf $(BUILD_DIR)

-include $(DEPS)
