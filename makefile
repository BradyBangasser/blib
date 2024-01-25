# Set standards to gnu23 and gnu++23

SRC :=src
LIB :=LIB
INCLUDE :=include
EXE :=libblib.a
TESTEXE:=test
OUT :=out
RM ?=rm
CXX_STD := c++20
C_STD := c17

GFLAGS := -I include -Wall
CFLAGS := 
CXXFLAGS := 
TESTFLAGS := -L . -lblib -L lib -lssl -lcrypto

ifeq ($(OS),Windows_NT)
	TESTFLAGS := $(TESTFLAGS) -lcrypt32 -lWs2_32
	TESTEXE := $(TESTEXE).exe
#	RM := del \r \S
else
	TESTEXE := $(TESTEXE).out
endif

CC:=gcc
CXX:=g++

CXX_SRCS :=$(wildcard $(SRC)/*.cpp)
C_SRCS =$(wildcard $(SRC)/*.c)

OBJS :=$(patsubst $(SRC)/%, $(OUT)/%.o, $(wildcard $(SRC)/*.c $(SRC)/*.cpp))

.PHONY: clean test rebuild build

build: $(EXE)

$(EXE): $(OBJS)
	ar rcs $(EXE) $(OBJS)

$(wildcard $(SRC)/*.c $(SRC)/*.cpp): $(OUT)

$(OUT)/%.cpp.o: $(SRC)/%.cpp $(SRC)/%.hpp
	$(CXX) $(GFLAGS) -std=$(CXX_STD) $(CXXFLAGS) -c -o $@ $<

$(OUT)/%.c.o: $(SRC)/%.c $(SRC)/%.h
	$(CC) $(GFLAGS) -std=$(C_STD) $(CFLAGS) -c -o $@ $<

$(OUT):
	mkdir out

rebuild: 
	make clean
	make build

clean:
# fix windows issues 
	$(RM) out/* $(EXE) *.out *.exe *.o

test:
	make $(EXE)
	g++ test/main.cpp -o $(TESTEXE) $(GFLAGS) $(TESTFLAGS)
	- ./$(TESTEXE)
	rm $(TESTEXE)