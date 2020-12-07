NAME=EZCAM
CC=/home/abobco/tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian-x64/bin/arm-linux-gnueabihf-gcc
CFLAGS=-Wall -pthread 
ODIR=obj
SDIR=src

_DEPS=ArduCAM.h bcm283x_board_driver.h sccb_bus.h ov2640_regs.h ov5640_regs.h ov5642_regs.h 
DEPS=$(patsubst %,$(SDIR)/%,$(_DEPS))

_OBJ=EZCAM.o ArduCAM.o bcm283x_board_driver.o sccb_bus.o
OBJ=$(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: src/%.c $(DEPS)
	@mkdir -p $(@D)
	$(CC) -c -o $@ $< $(CFLAGS) -lrt -g -I/home/abobco/RaspberryPi/pigpio-master/install/include -L/home/abobco/RaspberryPi/pigpio-master/install/lib/ -lpigpio

$(NAME): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) -lrt -g -I/home/abobco/RaspberryPi/pigpio-master/install/include -L/home/abobco/RaspberryPi/pigpio-master/install/lib/ -lpigpio

.PHONY:	run

run:	$(NAME)
	sudo ./$(NAME)

.PHONY:	clean
clean:	
	rm $(NAME) $(ODIR)/*.o
