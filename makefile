SRC :=src
LIB :=LIB
INCLUDE :=include
EXE :=libblib.a
OUT :=out
RM ?=rm

CC:=gcc
CXX:=g++

ifeq ($(OS),Windows_NT)
	RM=cmd //C del
endif

CXX_SRCS :=$(wildcard $(SRC)/*.cpp)
C_SRCS =$(wildcard $(SRC)/*.c)

OBJS :=$(patsubst $(SRC)/%, $(OUT)/%.o, $(wildcard $(SRC)/*.c $(SRC)/*.cpp))

$(EXE): $(OBJS)
	ar rcs $(EXE) $(OBJS)

$(wildcard $(SRC)/*.c $(SRC)/*.cpp): $(OUT)

$(OUT)/%.cpp.o: $(SRC)/%.cpp
	$(CXX) -c -o $@ $^

$(OUT)/%.c.o: $(SRC)/%.c
	$(CC) -c -o $@ $^

$(OUT):
	mkdir out

clean:
	$(foreach file, $(wildcard $(OUT)/*), $(RM) $(file); )