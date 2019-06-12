CFLAGS  = -std=c99 -Wall -Wextra -Os -g3 -D_POSIX_C_SOURCE=199309L

all: p01 p02 p03 p04 p05 p06 p07 p08 p09

clean:
	$(RM) p01 p02 p03 p04 p05 p06 p07 p08 p09
