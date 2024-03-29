SRC := src
VERSION := 1.0.0
OUT := out
LIB := lib

EXE := libblib.a

SPACE := $(eval) $(eval)

ifeq ($(OS),Windows_NT)
SHELL_REVERSE_COMMAND = tac
else
SHELL_REVERSE_COMMAND = tail -r
endif

# https://stackoverflow.com/questions/52674/simplest-way-to-reverse-the-order-of-strings-in-a-make-variable
REVERSE = $(shell printf "%s\n" $(strip $1) | $(SHELL_REVERSE_COMMAND))

CXX := g++
CC := gcc
FC := gfortran
ASMC := as

LD_FLAGS := -lcrypto
G_FLAGS := -Wall -O3 -I include
CXX_FLAGS :=
C_FLAGS :=
F_FLAGS :=
ASM_FLAGS :=

CXX_STD := 17
C_STD := 11

GET_SRCS = $(wildcard $(SRC)/*.$1) $(wildcard $(SRC)/*/*.$1)
GET_OBJS = $(foreach file,$(patsubst $(SRC)/%, %, $1),$(patsubst %,$(OUT)/%.o,$(subst $(SPACE),.,$(call REVERSE,$(subst /, ,$(word 1,$(subst ., ,$(file))))) $(word 2,$(subst ., ,$(file))))))

MAKE_OBJ = $($1) -c $($2) $($3) -o $($4)

CXX_SRCS := $(call GET_SRCS,cpp)
C_SRCS := $(call GET_SRCS,c)
F_SRCS := $(call GET_SRCS,f)
ASM_SRCS := $(call GET_SRCS,asm)

CXX_OBJS := $(call GET_OBJS,$(CXX_SRCS))
C_OBJS := $(call GET_OBJS,$(C_SRCS))
F_OBJS := $(call GET_OBJS,$(F_SRCS))
ASM_OBJS := $(call GET_OBJS,$(ASM_SRCS))

MODULES := $(patsubst $(SRC)/%/.,%,$(wildcard $(SRC)/*/.))

define COMPILE
$(1) -c -o $$@ $$< $(2) $(G_FLAGS)
endef

define FILE_RECIPE 
MODNAME = $(1)
FILEEXT = $(2)
$(OUT)/%.$$(MODNAME).$$(FILEEXT).o: $(SRC)/$$(MODNAME)/%.$$(FILEEXT) | $(OUT)
	$(call COMPILE,$(4),$(3))
endef

build: $(EXE)

$(OUT):
	mkdir $@

clean:
	rm -rf $(EXE) $(OUT)

$(OUT)/%.f.o: $(SRC)/%.f | $(OUT)
	$(FC) -c -o $@ $< $(F_FLAGS)
$(foreach mod,$(MODULES),$(eval $(call FILE_RECIPE,$(mod),f,$(F_FLAGS),$(FC))))

$(OUT)/%.cpp.o: $(SRC)/%.cpp | $(OUT)
	$(CXX) -c -o $@ $< $(CXX_FLAGS) -std=c++$(CXX_STD)
$(foreach mod,$(MODULES),$(eval $(call FILE_RECIPE,$(mod),cpp,$(CXX_FLAGS) -std=c++$(CXX_STD),$(CXX))))

$(OUT)/%.asm.o: $(SRC)/%.asm | $(OUT)
	$(ASM) -c -o $@ $< $(ASM_FLAGS)
$(foreach mod,$(MODULES),$(eval $(call FILE_RECIPE,$(mod),asm,$(ASM_FLAGS),$(ASM))))

$(OUT)/%.c.o: $(SRC)/%.c | $(OUT)
	$(CC) -c -o $@ $< $(C_FLAGS) -std=c$(C_STD)
$(foreach mod,$(MODULES),$(eval $(call FILE_RECIPE,$(mod),c,$(C_FLAGS) -std=c$(C_STD),$(CC))))

.PHONY: build clean test

$(EXE): $(CXX_OBJS) $(C_OBJS) $(F_OBJS) $(ASM_OBJS)
	ar rcs $(EXE) $(CXX_OBJS) $(C_OBJS) $(F_OBJS) $(ASM_OBJS)

test:
	make
	g++ test/main.cpp -L. -lblib -lcrypto -Llib -lblib -lssl -lcrypto -lcrypt32 -lWs2_32
	./a.exe