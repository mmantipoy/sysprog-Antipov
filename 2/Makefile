GCC_FLAGS = -Wextra -Werror -Wall -Wno-gnu-folding-constant -g

all: parser.c solution.c heap_help.c
	gcc $(GCC_FLAGS) -ldl -rdynamic parser.c solution.c heap_help.c

clean:
	rm a.out