CC := gcc
CFL := -ggdb -fPIC -std=c++11 -Wno-deprecated
TOOLCHAIN :=
OUT_EXECUTABLE_NAME := zraw-parser
ARCH :=

INCS := -Iinclude -Isrc -Inana/include -Izraw-decoder/tinydngloader/examples/dngwriter
INCS += -Izraw-decoder/zraw-decoder-lib/include

LIBS := -lssl -lstdc++ -lm -L./lib -lnana -lpthread -lX11 -lXft -lfontconfig -lstdc++fs -lm -lzraw -lcrypto
SOURCE_FILES := src/main.cpp

BUILDDIR := build/
LIBDIR := lib/

all: check-and-reinit-submodules
	$(MAKE) -C ./nana/build/makefile/
	$(MAKE) -C ./zraw-decoder
	@mkdir -p $(LIBDIR)
	cp ./nana/build/bin/libnana.a $(LIBDIR)
	cp ./zraw-decoder/zraw-decoder-lib/build/libzraw.a $(LIBDIR)
	@mkdir -p $(BUILDDIR)
	$(TOOLCHAIN)$(CC) $(CFL) $(ARCH) $(SOURCE_FILES) $(INCS) $(LIBS) -o $(BUILDDIR)$(OUT_EXECUTABLE_NAME)
	cp ./res/* $(BUILDDIR)

check-and-reinit-submodules:
	@if git submodule status | egrep -q '^[-]|^[+]' ; then \
			echo "INFO: Need to reinitialize git submodules"; \
			git submodule update --init; \
	fi

clear:
	rm -f *.o
	rm -f $(OUT_EXECUTABLE_NAME)
