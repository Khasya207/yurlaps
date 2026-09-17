CC ?= cc
CPPFLAGS += -I../OneLoopDecoder.cydsn
CFLAGS ?= -std=c99 -O2 -Wall -Wextra -Wpedantic -Werror

SOURCES = test_decoder.c \
          ../OneLoopDecoder.cydsn/rch_protocol.c \
          ../OneLoopDecoder.cydsn/rch_demod.c \
          ../OneLoopDecoder.cydsn/passage_tracker.c

PSOC_SOURCES = ../OneLoopDecoder.cydsn/main.c \
               ../OneLoopDecoder.cydsn/capture_dma.c \
               ../OneLoopDecoder.cydsn/timebase.c \
               ../OneLoopDecoder.cydsn/rch_output.c \
               ../OneLoopDecoder.cydsn/rch_protocol.c \
               ../OneLoopDecoder.cydsn/rch_demod.c \
               ../OneLoopDecoder.cydsn/passage_tracker.c

OUTPUT_SOURCES = test_output.c ../OneLoopDecoder.cydsn/rch_output.c

CAPTURE_SOURCES = test_capture.c ../OneLoopDecoder.cydsn/capture_dma.c
TIMEBASE_SOURCES = test_timebase.c ../OneLoopDecoder.cydsn/timebase.c

.PHONY: all test syntax clean
all: test_decoder test_output test_capture test_timebase

test_decoder: $(SOURCES)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SOURCES) -o $@

test_output: $(OUTPUT_SOURCES)
	$(CC) -Ipsoc-stubs $(CPPFLAGS) $(CFLAGS) $(OUTPUT_SOURCES) -o $@

test_capture: $(CAPTURE_SOURCES)
	$(CC) -Ipsoc-stubs $(CPPFLAGS) $(CFLAGS) $(CAPTURE_SOURCES) -o $@

test_timebase: $(TIMEBASE_SOURCES)
	$(CC) -Ipsoc-stubs $(CPPFLAGS) $(CFLAGS) $(TIMEBASE_SOURCES) -o $@

test: test_decoder test_output test_capture test_timebase syntax
	./test_decoder
	./test_output
	./test_capture
	./test_timebase

syntax:
	$(CC) -Ipsoc-stubs -I../OneLoopDecoder.cydsn -std=c99 -Wall -Wextra \
		-Wpedantic -Werror -fsyntax-only $(PSOC_SOURCES)

clean:
	rm -f test_decoder test_output test_capture test_timebase
