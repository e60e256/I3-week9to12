CC = gcc
CFLAGS = -Wall
LDLIBS = -lm -lpthread
OBJS = week9-2.o fft2.o
TARGET = serv_sendX4

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDLIBS)

.PHONY: tmpclean clean
tmpclean:
	rm -f *~
clean: tmpclean
	rm -f $(OBJS) $(TARGET)
