GCC_FLAGS = -Wextra -Werror -Wall -Wno-gnu-folding-constant -g

all: libcoro.c solution.c
	gcc $(GCC_FLAGS) libcoro.c solution.c

clean:
	rm a.out

# GCC_FLAGS = -Wextra -Werror -Wall -Wno-gnu-folding-constant

# hw-1: obj/solution.o obj/sortq.o
# 	gcc $(GCC_FLAGS) obj/solution.o obj/sortq.o -o hw-1

# obj/solution.o: src/solution.c include/sortq.h | obj
# 	gcc $(GCC_FLAGS) -I include/ -c src/solution.c -o obj/solution.o

# obj/sortq.o: src/sortq.c include/sortq.h | obj
# 	gcc $(GCC_FLAGS) -I include/ -c src/sortq.c -o obj/sortq.o
# # obj/clist.o: src/clist.c include/clist.h | obj
# # 	gcc $(GCC_FLAGS) -I include/ -c src/clist.c -o obj/clist.o

# # obj/point_list.o: src/point_list.c include/point_list.h include/clist.h | obj
# # 	gcc $(GCC_FLAGS) -I include/ -c src/point_list.c -o obj/point_list.o

# obj:
# 	mkdir obj

# .PHONY: clean

# clean:
# 	rm -r -f obj
# 	rm -r -f hw-1