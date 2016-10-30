CC = gcc
COMPILER_FLAGS = -O2 -g -ansi

OBJS =  ../../../../utils/matrix/csr_matrix/csr_load_matrix.o
OBJS +=	../../../../utils/matrix/csr_matrix/csr_matrix.o
OBJS +=	../../../../utils/matrix/csr_matrix/csr_assign.o
OBJS +=	../../../../utils/matrix/csr_matrix/csr_math.o
OBJS +=	../../../../utils/fcl_logging.o
OBJS +=	../../../../utils/clogging.o
OBJS +=	../../../../utils/vector/common/common_vector_math.o
OBJS +=	../../../../utils/vector/sparse/sparse_vector_math.o
OBJS +=	../../../../utils/fcl_file.o
OBJS +=	micro_assign.o

micro_assign : ${OBJS}
	${CC} ${COMPILER_FLAGS} ${OBJS} -o $@ -lm

%.o : %.c
	${CC} ${COMPILER_FLAGS} -c $< -o $@

clean :
	-rm *.o
	-rm micro_assign
