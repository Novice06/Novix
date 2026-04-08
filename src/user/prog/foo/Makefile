
CFLAGS  := -ffreestanding -nostdlib -g -I $(LIBC)/include

LDFLAGS := -nostdlib -static -T linker.ld

OUT := $(BUILD_DIR)/user/prog/foo.bin

all: clean $(OUT)

$(OUT): main.o $(LIBC)/build/crt0.o $(LIBC)/build/libc.a
	mkdir -p $(@D)
	$(LD) $(LDFLAGS) $(LIBC)/build/crt0.o main.o $(LIBC)/build/libc.a -o $@ -Map foo.map

main.o: main.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(OUT)

.PHONY: all clean