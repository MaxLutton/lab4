build:
	gcc -lmraa -lm -o lab4a lab4a.c
	gcc -lmraa -lm -o lab4b lab4b.c

clean:
	rm -f lab4a
	rm -f lab4b
	rm -f log1
	rm -f log2
dist:
	tar -czf lab4-604493477.tar.gz lab4a.c lab4b.c Makefile log1 log2 README
