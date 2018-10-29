webproxy: webproxy.c
	gcc -W -o webproxy webproxy.c
clean:
	rm webproxy; 
run:
	./webproxy 5001
compile:
	make clean; make
