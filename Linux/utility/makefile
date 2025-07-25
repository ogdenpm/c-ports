TARGET = utility.a
OBJS = file.o getch.o memory.o message.o option.o _utility.o exit.o
OWNTARGET=Y
include ../common.mk

$(TARGET): $(OBJS)
	ar rv $@ $(OBJS)
	install -D -p -t ../Install $(TARGET)

$(OBJS): utility.h
option.o: verInfo.h
