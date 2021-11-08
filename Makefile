CC = arm-linux-gnueabihf-gcc
CFLAGS = -g 
TARGET = opz/spi/spi
SSHHOST = tesla@192.168.1.138

.PHONY: all run clean rebuild kill debug
all: $(TARGET)
rebuild: clean $(TARGET)


$(TARGET): main.o
	arm-linux-gnueabihf-gcc -o $@ $^

clean:
	$(RM) main.o
	$(RM) $(TARGET)

run:
	ssh $(SSHHOST) 'exec PRJS/spi/spi -m6 /dev/spidev1.0'
debug:
	ssh $(SSHHOST) 'nohup gdbserver host:4444 PRJS/spi/spi -r 2 /dev/spidev1.0\
	 >/dev/null 2>&1 &'

kill:
	ssh $(SSHHOST) 'kill -9 $$(ps -ef| grep gdbserver | sed -n  \
	"s/[a-z]\+[\ t]*\([0-9]\+\) .*/\1/p"| head -1) '