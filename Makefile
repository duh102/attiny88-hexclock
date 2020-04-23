FLAGS = -mmcu=attiny88 -DF_CPU=8000000UL -Os -std=c99

test.hex: test.elf
	avr-objcopy -O ihex $< $@

clean:
	rm test.elf test.hex

test.elf: test.c hsv_rgb.c ws2812.h hsv_rgb.h dim_curve.h
	avr-gcc $(FLAGS) $^ -o $@
