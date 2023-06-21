# Get OS name
UNAME := $(shell uname)

CC := gcc
# -ggdb
CFL := -O3 -fPIC -std=c++17 -Wno-deprecated
TOOLCHAIN :=
OUT_EXECUTABLE_NAME := zraw-parser
ARCH :=

INCS := -Iinclude -Isrc -Inana/include -Izraw-decoder/tinydngloader/examples/dngwriter
INCS += -Izraw-decoder/zraw-decoder-lib/include

SOURCE_FILES := src/main.cpp

BUILDDIR := build/
LIBDIR := lib/

ifeq ($(UNAME),Darwin)
LIBS := -L/opt/X11/lib -lstdc++ -lm -L./lib -lnana -lpthread -lX11 -lXft -lfontconfig -lm -lzraw
MACOS_VERSION := 10.15

ARCH := -arch x86_64 -arch arm64
else
LIBS := -lssl -lstdc++ -lm -L./lib -lnana -lpthread -lX11 -lXft -lfontconfig -lstdc++fs -lm -lzraw -lcrypto
endif

all: check-and-reinit-submodules
ifeq ($(UNAME),Darwin)
	@mkdir -p ./nana/build/cmake/build
	MACOSX_DEPLOYMENT_TARGET=$(MACOS_VERSION) CMAKE_OSX_ARCHITECTURES="x86_64;arm64" cmake -Bnana/build/cmake/build -Snana
	(cd ./nana/build/cmake/build && make)
	$(MAKE) -C ./zraw-decoder
# Create folders
	@mkdir -p $(LIBDIR)
	@mkdir -p $(BUILDDIR)
# Copy own static libs
	cp ./nana/build/cmake/build/libnana.a $(LIBDIR)
	cp ./zraw-decoder/zraw-decoder-lib/build/libzraw.a $(LIBDIR)
# Prepare bundle template
	cp -r ./mac_bundle_template $(BUILDDIR)$(OUT_EXECUTABLE_NAME).app
# Prepare resources
	cp ./res/* $(BUILDDIR)$(OUT_EXECUTABLE_NAME).app/Contents/MacOS
# Prepare executable
	$(TOOLCHAIN)$(CC) -mmacosx-version-min=$(MACOS_VERSION) $(CFL) $(ARCH) $(SOURCE_FILES) $(INCS) $(LIBS) -o $(BUILDDIR)$(OUT_EXECUTABLE_NAME)
	cp ./$(BUILDDIR)/$(OUT_EXECUTABLE_NAME) $(BUILDDIR)$(OUT_EXECUTABLE_NAME).app/Contents/MacOS
# Add dynamic libraries to the bundle
	dylibbundler -od -b -x $(BUILDDIR)$(OUT_EXECUTABLE_NAME).app/Contents/MacOS/$(OUT_EXECUTABLE_NAME) -d $(BUILDDIR)$(OUT_EXECUTABLE_NAME).app/Contents/Library/ -p @executable_path/../Library/

else
	$(MAKE) -C ./nana/build/makefile/
	$(MAKE) -C ./zraw-decoder
	@mkdir -p $(LIBDIR)
	cp ./nana/build/bin/libnana.a $(LIBDIR)
	cp ./zraw-decoder/zraw-decoder-lib/build/libzraw.a $(LIBDIR)
	@mkdir -p $(BUILDDIR)
	$(TOOLCHAIN)$(CC) $(CFL) $(ARCH) $(SOURCE_FILES) $(INCS) $(LIBS) -o $(BUILDDIR)$(OUT_EXECUTABLE_NAME)
	cp ./res/* $(BUILDDIR)
endif

check-and-reinit-submodules:
	@if git submodule status | egrep -q '^[-]|^[+]' ; then \
			echo "INFO: Need to reinitialize git submodules"; \
			git submodule update --init; \
	fi

clear:
	rm -f *.o
	rm -f $(OUT_EXECUTABLE_NAME)
