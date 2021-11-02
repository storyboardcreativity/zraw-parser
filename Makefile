CC := gcc
CFL := -ggdb -fPIC -std=c++11 -Wno-deprecated
TOOLCHAIN :=
OUT_EXECUTABLE_NAME := zraw-parser
ARCH :=
INCS := -Iinclude
LIBS := -lssl -lcrypto -lstdc++ -lm -L./lib -lnana -lpthread -lX11 -lXft -lfontconfig -lstdc++fs -lm
SOURCE_FILES := src/main.cpp

BUILDDIR := build/

all: check-and-reinit-submodules
	$(MAKE) -C ./zraw-decoder
	@mkdir -p $(BUILDDIR)
	cp zraw-decoder/build/* $(BUILDDIR)
	$(TOOLCHAIN)$(CC) $(CFL) $(ARCH) $(SOURCE_FILES) $(INCS) $(LIBS) -o $(BUILDDIR)$(OUT_EXECUTABLE_NAME)

check-and-reinit-submodules:
	@if git submodule status | egrep -q '^[-]|^[+]' ; then \
			echo "INFO: Need to reinitialize git submodules"; \
			git submodule update --init; \
	fi

clear:
	rm -f *.o
	rm -f $(OUT_EXECUTABLE_NAME)