FLAGS = -mmcu=attiny88 -DF_CPU=8000000UL -Os -std=c99 -Werror

test.hex: test.elf
	avr-objcopy -O ihex $< $@
	avr-size -C --mcu=attiny88 $<

clean:
	rm test.elf test.hex

test.elf: test.c hsv_rgb.c twimaster/twimaster.c mcp7940_tiny.c
	avr-gcc $(FLAGS) $^ -o $@ 

hsv_rgb.c: hsv_rgb.h dim_curve.h

twimaster/twimaster.c: twimaster/i2cmaster.h

mcp7940_tiny.c: mcp7940_tiny.h
