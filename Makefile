
.DEFAULT: all
.ONESHELL:

.phony: clean all constructor

OUTPUT:= Build

#SRC:= item.cpp main.cpp variable.cpp
SRC:= main.cpp \
	LuaEngine.cpp \
	StrUtil.cpp \
	OSUtil.cpp \
	FileUtil.cpp \
	PackageConfig.cpp

SRC:=$(addprefix $(OUTPUT)/,$(SRC))

COMPILER := g++
CXXFLAGS := --std=c++11 -Wall -Wextra -g
LDFLAGS :=
OS := $(shell uname -s)
ifeq ($(OS),Darwin)
COMPILER := clang++
CXXFLAGS := --std=c++11 --stdlib=libc++ -Wall -Wextra -g
endif

LUA_VER:=lua-5.2.3
LUA_DIR:=src/$(LUA_VER)/src
LUA_SRC:=lapi.c lcode.c lctype.c ldebug.c ldo.c ldump.c lfunc.c lgc.c llex.c \
	lmem.c lobject.c lopcodes.c lparser.c lstate.c lstring.c ltable.c ltm.c \
	lundump.c lvm.c lzio.c \
	lauxlib.c lbaselib.c lbitlib.c lcorolib.c ldblib.c liolib.c \
	lmathlib.c loslib.c lstrlib.c ltablib.c loadlib.c linit.c
LUA_OUT:=$(addprefix $(OUTPUT)/,$(LUA_SRC))

all: constructor

constructor: $(OUTPUT)/constructor

$(OUTPUT)/%.o: src/%.cpp | $(OUTPUT)
	@$(COMPILER) $(CXXFLAGS) -I $(LUA_DIR) -c -MMD -MF $(OUTPUT)/$*.dep -o $@ $<

$(OUTPUT)/%.o: $(LUA_DIR)/%.c | $(OUTPUT)
	@$(COMPILER) -x c++ $(CXXFLAGS) -c -MMD -MF $(OUTPUT)/$*.dep -o $@ $<

$(OUTPUT)/constructor: $(SRC:.cpp=.o) $(LUA_OUT:.c=.o)
	@$(COMPILER) $(CXXFLAGS) -o $(OUTPUT)/constructor $^ $(LDFLAGS)

$(OUTPUT):
	mkdir $(OUTPUT)

clean:
	rm -rf $(OUTPUT)

-include $(SRC:.cpp=.dep)
