CFLAGS  = -std=c99 -Wall -Wextra -Os -g3 -D_POSIX_C_SOURCE=199309L

all: p01 p02 p03 p04

clean:
	$(RM) -f p01 p02 p03 p04
