# NAME: Brendan Rossmango
# EMAIL: brendan0913@ucla.edu
# ID: 505370692

default: client server

client:
	gcc -Wall -Wextra lab1b-client.c -lz -o lab1b-client

server:
	gcc -Wall -Wextra lab1b-server.c -lz -o lab1b-server

clean:
	rm -f lab1b-client lab1b-server *.tar.gz

dist: default
	tar -czvf lab1b-505370692.tar.gz lab1b-client.c lab1b-server.c Makefile README
