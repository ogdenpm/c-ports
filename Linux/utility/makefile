TARGET = utility.a
OBJS = file.o getch.o memory.o message.o option.o _utility.o exit.o
OWNTARGET=Y
include ../common.mk

$(TARGET): $(OBJS)
	ar rv $@ $(OBJS)
	mkdir -p ../Install && cp -p $(TARGET) ../Install/

$(OBJS): utility.h
option.o: verInfo.h
