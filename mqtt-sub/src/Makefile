GFLAGS := -Wall -Wextra -pedantic -std=gnu99 -O2
CFLAGS := $(GFLAGS)
SOURCES := $(wildcard *.c)
OBJS := $(SOURCES:.c=.o)
BIN := mqtt_sub
LDFLAGS := -lmosquitto -luci -lcurl -ljson-c -lsqlite3 -largp

.PHONY: all clean

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	$(RM) $(BIN) $(OBJS)
