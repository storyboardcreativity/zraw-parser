CC := gcc
CFL := -ggdb -fPIC -std=c++11
TOOLCHAIN :=
OUT_EXECUTABLE_NAME := zraw_processor
ARCH :=
LIBS := -lssl -lcrypto -lstdc++ -lm
SOURCE_FILES := main.cpp ZRawFrameContainerParserSingletone.cpp ZRawFrameDecompressorSingletone.cpp ZRawFramePreProcessorSingletone.cpp Tools.cpp

all:
	$(TOOLCHAIN)$(CC) $(CFL) $(ARCH) $(SOURCE_FILES) $(LIBS) -o $(OUT_EXECUTABLE_NAME)

clear:
	rm -f *.o
	rm -f $(OUT_EXECUTABLE_NAME)