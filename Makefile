
CC = gcc

LIBS = /home/sandeep/unpv13e/libunp.a -lc

FLAGS = -g -O2

all: get_hw_addrs.o prhwaddrs.o odr.o odr_recv.o odr_lib.o
	${CC} -o prhwaddrs prhwaddrs.o get_hw_addrs.o ${LIBS}
	${CC} -o odr odr.o get_hw_addrs.o ${LIBS}
	${CC} -o odr_recv odr_recv.o get_hw_addrs.o ${LIBS} 

get_hw_addrs.o: get_hw_addrs.c
	${CC} ${FLAGS} -c get_hw_addrs.c

prhwaddrs.o: prhwaddrs.c
	${CC} ${FLAGS} -c prhwaddrs.c

odr.o:	odr.c
	${CC} ${FLAGS} -c odr.c

odr_recv.o: odr_recv.c
	${CC} ${FLAGS} -c odr_recv.c

odr_lib.o: odr_lib.c
	${CC} ${FLAGS} -c odr_lib.c

clean:
	rm -rf *.o prhwaddrs odr odr_recv
