# Set standards to gnu23 and gnu++23

SRC :=src
LIB :=LIB
INCLUDE :=include
EXE :=libblib.a
OUT :=out
RM ?=rm

GFLAGS := -I include -Wall
CFLAGS := 
CXXFLAGS := 
TESTFLAGS := -L . -lblib -L lib -lssl -lcrypto -lcrypt32 -lws2_32

CC:=gcc
CXX:=g++

CXX_SRCS :=$(wildcard $(SRC)/*.cpp)
C_SRCS =$(wildcard $(SRC)/*.c)

OBJS :=$(patsubst $(SRC)/%, $(OUT)/%.o, $(wildcard $(SRC)/*.c $(SRC)/*.cpp))

.PHONY: clean test

$(EXE): $(OBJS)
	ar rcs $(EXE) $(OBJS)

$(wildcard $(SRC)/*.c $(SRC)/*.cpp): $(OUT)

$(OUT)/%.cpp.o: $(SRC)/%.cpp $(SRC)/%.hpp
	$(CXX) $(GFLAGS) $(CXXFLAGS) -c -o $@ $<

$(OUT)/%.c.o: $(SRC)/%.c $(SRC)/%.h
	$(CC) $(GFLAGS) $(CFLAGS) -c -o $@ $<

$(OUT):
	mkdir out

clean:
	$(RM) out/* $(EXE)

test:
	make $(EXE)
	g++ test/main.cpp $(GFLAGS) $(TESTFLAGS)
	./a.exe
	rm a.exe