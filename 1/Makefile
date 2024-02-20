GCC_FLAGS = -Wextra -Werror -Wall -Wno-gnu-folding-constant -g

all: libcoro.c solution.c heap_help.c
	gcc $(GCC_FLAGS) -ldl -rdynamic libcoro.c solution.c heap_help.c

clean:
	rm a.out