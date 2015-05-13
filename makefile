# Make file

net367: host.o utilities.o link.o man.o switch.o net.o main.o
	gcc -o net367 host.o utilities.o link.o man.o switch.o net.o main.o

queue: switch.o testqueue.o
	gcc -o queue switch.o testqueue.o

table: switch.o testtable.o
	gcc -o table switch.o testtable.o

main.o: main.c
	gcc -c main.c

host.o: host.c 
	gcc -c host.c  

man.o:  man.c
	gcc -c man.c

net.o:  net.c
	gcc -c net.c

utilities.o: utilities.c
	gcc -c utilities.c

link.o:  link.c
	gcc -c link.c

switch.o: switch.c
	gcc -c switch.c
	
testqueue.o: testqueue.c
	gcc -c testqueue.c

testtable.o: testtable.c
	gcc -c testtable.c

clean:
	rm -f *.o

real_clean:
	rm -f *.o net367 queue table
