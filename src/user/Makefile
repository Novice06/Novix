LIBC=$(abspath libc)

export LIBC

all:
	$(MAKE) -C libc
	$(MAKE) -C prog
	$(MAKE) -C doom/doomgeneric -f Makefile.novix
	cp doom1.wad  $(BUILD_DIR)/user/


clean:
	$(MAKE) -C libc clean
	$(MAKE) -C doom/doomgeneric clean