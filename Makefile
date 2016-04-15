.SUFFIXES:
.DEFAULT: all
.ONESHELL:
.SILENT:
.PHONY: clean all install constructor constructor.debug

PREFIX:=${HOME}
OUTPUT:= .build

SRC:= main.cpp \
	Item.cpp \
	BuildItem.cpp \
	Scope.cpp \
	Rule.cpp \
	TransformSet.cpp \
	Compile.cpp \
	OptionalSource.cpp \
	Executable.cpp \
	InternalExecutable.cpp \
	Library.cpp \
	ExternLibrary.cpp \
	CodeFilter.cpp \
	CodeGenerator.cpp \
	CreateFile.cpp \
	Tool.cpp \
	Toolset.cpp \
	Pool.cpp \
	DefaultTools.cpp \
	Configuration.cpp \
	Variable.cpp \
	Generator.cpp \
	NinjaGenerator.cpp \
	MakeGenerator.cpp \
	LuaEngine.cpp \
	LuaValue.cpp \
	LuaExtensions.cpp \
	LuaCodeGenExt.cpp \
	LuaCompileExt.cpp \
	LuaConfigExt.cpp \
	LuaFileExt.cpp \
	LuaItemExt.cpp \
	LuaScopeExt.cpp \
	LuaSysExt.cpp \
	LuaToolExt.cpp \
	Debug.cpp \
	StrUtil.cpp \
	OSUtil.cpp \
	FileUtil.cpp \
	Directory.cpp \
	PackageSet.cpp \
	PackageConfig.cpp

SRC:=$(addprefix $(OUTPUT)/,$(SRC))

COMPILER := g++
CXXFLAGS := --std=c++11 -Wall -Wextra -Og -g
LDFLAGS :=
OS := $(shell uname -s)
ifeq ($(OS),Linux)
CXXFLAGS := $(CXXFLAGS) -DLUA_USE_LINUX
LDFLAGS := -static-libgcc -static-libstdc++ -ldl
else
ifeq ($(OS),Darwin)
COMPILER := clang++
CXXFLAGS := --std=c++11 --stdlib=libc++ -Wall -Wextra -O2 -g -DLUA_USE_MACOSX
endif
endif

LUA_VER:=lua-5.3.2
LUA_DIR:=src/$(LUA_VER)/src
LUA_SRC:=lapi.c lcode.c lctype.c ldebug.c ldo.c ldump.c lfunc.c lgc.c llex.c \
	lmem.c lobject.c lopcodes.c lparser.c lstate.c lstring.c ltable.c ltm.c \
	lundump.c lvm.c lzio.c \
	lauxlib.c lbaselib.c lbitlib.c lcorolib.c ldblib.c liolib.c \
	lmathlib.c loslib.c lstrlib.c ltablib.c lutf8lib.c loadlib.c linit.c
LUA_OUT:=$(addprefix $(OUTPUT)/,$(LUA_SRC))

all: constructor

constructor: $(OUTPUT)/bin/constructor
	@echo "Self-compiling constructor..."
	@cd $(OUTPUT) && ninja && cd ..
	@rm -f ./constructor
	@ln $(OUTPUT)/bin/constructor constructor
	@echo "Constructor is built and in current directory"

constructor.debug: $(OUTPUT)/constructor

$(OUTPUT)/build.ninja: $(OUTPUT)/constructor
	@$(OUTPUT)/constructor

$(OUTPUT)/bin/constructor: $(OUTPUT)/build.ninja

$(OUTPUT)/%.o: src/%.cpp | $(OUTPUT)
	@echo "[CXX] $<"
	@$(COMPILER) $(CXXFLAGS) -c -MMD -MF $(OUTPUT)/$*.dep -o $@ $<

$(OUTPUT)/%.o: $(LUA_DIR)/%.c | $(OUTPUT)
	@echo "[CXX] $<"
	@$(COMPILER) -x c++ $(CXXFLAGS) -c -MMD -MF $(OUTPUT)/$*.dep -o $@ $<

$(OUTPUT)/constructor: $(SRC:.cpp=.o) $(LUA_OUT:.c=.o)
	@echo "[LD] constructor"
	@$(COMPILER) $(CXXFLAGS) -o $(OUTPUT)/constructor $^ $(LDFLAGS)

$(OUTPUT):
	@echo "Bootstrapping constructor..."
	mkdir $(OUTPUT)

install: constructor
	@echo "Installing to ${PREFIX}..."
	@mkdir -p ${PREFIX}/bin
	@cp ./constructor ${PREFIX}/bin

clean:
	rm -rf $(OUTPUT)

-include $(SRC:.cpp=.dep)
-include $(LUA_OUT:.cpp=.dep)
