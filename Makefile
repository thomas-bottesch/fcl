CC = gcc

COMPILER_FLAGS = -O2 -fopenmp

rwildcard=$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))
ALL_ALGO_SOURCES := $(call rwildcard,algorithms/,*.c)
ALL_UTIL_SOURCES := $(call rwildcard,utils/,*.c)

ALGO_OBJECTS := $(ALL_ALGO_SOURCES:%.c=%.o)
UTIL_OBJECTS := $(ALL_UTIL_SOURCES:%.c=%.o)

OBJS = ${UTIL_OBJECTS} ${ALGO_OBJECTS} main.o

fcl : ${OBJS}
	${CC} ${COMPILER_FLAGS} ${OBJS} -o $@ -lm

%.o : %.c
	${CC} ${COMPILER_FLAGS} -c $< -o $@

clean :
	-rm *.o
	-rm fcl
	-rm algorithms/*.o
	-rm algorithms/*/*.o
	-rm utils/*.o
	-rm utils/*/*.o
	-rm utils/*/*/*.o
