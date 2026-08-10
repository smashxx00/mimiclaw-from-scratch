CC      ?= cc
CFLAGS  ?= -O2 -std=c11 -Wall -Wextra
CPPFLAGS += -Isrc -Ithird_party/cJSON -D_POSIX_C_SOURCE=200809L
LDLIBS  += -lcurl

SRCS = src/main.c src/bus.c src/llm.c src/tools.c src/memory.c src/agent.c \
       third_party/cJSON/cJSON.c
HDRS = $(wildcard src/*.h) third_party/cJSON/cJSON.h

TARGET = nano-mimiclaw

all: $(TARGET)

$(TARGET): $(SRCS) $(HDRS)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $(SRCS) $(LDLIBS)

test/unit_tests: test/unit_tests.c src/bus.c src/memory.c src/tools.c \
                 third_party/cJSON/cJSON.c $(HDRS)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ test/unit_tests.c src/bus.c \
	    src/memory.c src/tools.c third_party/cJSON/cJSON.c $(LDLIBS)

test: $(TARGET) test/unit_tests
	sh test/run_tests.sh

trace: $(TARGET)
	sh scripts/generate-trace.sh

clean:
	rm -f $(TARGET) test/unit_tests

.PHONY: all test trace clean
