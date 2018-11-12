webproxy: webproxy.c hashTable.h hashTable.c
	gcc -W -o webproxy webproxy.c hashTable.c
clean:
	rm webproxy;
run:
	./webproxy 5001 60
compile:
	make clean; make
