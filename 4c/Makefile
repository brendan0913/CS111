# NAME: Brendan Rossmango
# EMAIL: brendan0913@ucla.edu
# ID: 505370692

default:
	@if [ `uname -a | grep -c "armv7l"` -ne 0 ]; then \
		gcc -g -lmraa -Wextra -Wall -lm -o lab4c_tcp lab4c_tcp.c; \
		gcc -g -lssl -lcrypto -lmraa -Wall -Wextra -lm -o lab4c_tls lab4c_tls.c; \
		echo "gcc -g -lmraa -Wextra -Wall -lm -o lab4c_tcp lab4c_tcp.c"; \
		echo "gcc -g -lssl -lcrypto -lmraa -Wall -Wextra -lm -o lab4c_tls lab4c_tls.c"; \
	else \
 		gcc -g -DDUMMY -Wextra -Wall -lm -o lab4c_tcp lab4c_tcp.c; \
		gcc -g -lssl -lcrypto -DDUMMY -Wall -Wextra -lm -o lab4c_tls lab4c_tls.c; \
		echo "gcc -g -DDUMMY -Wextra -Wall -lm -o lab4c_tcp lab4c_tcp.c"; \
		echo "gcc -g -lssl -lcrypto -DDUMMY -Wall -Wextra -lm -o lab4c_tls lab4c_tls.c"; \
	fi;

clean:
	rm -f lab4c_tcp lab4c_tls *.gz

dist: default
	tar -czvf lab4c-505370692.tar.gz Makefile README lab4c_tcp.c lab4c_tls.c
