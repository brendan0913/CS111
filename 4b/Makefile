# NAME: Brendan Rossmango
# EMAIL: brendan0913@ucla.edu
# ID: 505370692

default:
	@if [ `uname -a | grep -c "armv7l"` -ne 0 ]; then \
		gcc -g -lmraa -Wextra -Wall -lm -o lab4b lab4b.c; \
		echo "gcc -g -lmraa -Wextra -Wall -lm -o lab4b lab4b.c"; \
	else \
		gcc -g -DDUMMY -Wextra -Wall -lm -o lab4b lab4b.c; \
		echo "gcc -g -DDUMMY -Wextra -Wall -lm -o lab4b lab4b.c"; \
	fi;

clean:
	rm -f lab4b *.gz

dist: default
	tar -czvf lab4b-505370692.tar.gz Makefile README lab4b.c

check: default
	@{ echo "STOP"; sleep 1; echo "START"; sleep 1; echo "SCALE=C"; sleep 1; \
	echo "LOG"; sleep 1; echo "invalid"; sleep 1; echo "PERIOD=2"; sleep 4; \
	echo "OFF"; } | ./lab4b --log=log.txt || echo "failed run"
	@echo "========================= logfile ========================="
	@cat log.txt
	@echo "========================= logfile ========================="
	@grep "STOP" log.txt > /dev/null || echo "failed STOP"
	@grep "START" log.txt > /dev/null || echo "failed START"
	@grep "SCALE=C" log.txt > /dev/null || echo "failed SCALE=C"
	@grep "LOG" log.txt > /dev/null || echo "failed LOG"
	@grep "invalid" log.txt > /dev/null || echo "failed invalid"
	@grep "PERIOD=2" log.txt > /dev/null || echo "failed PERIOD"
	@grep "OFF" log.txt > /dev/null || echo "failed OFF"
	@grep "SHUTDOWN" log.txt > /dev/null || echo "failed SHUTDOWN"
	@echo "Smoketest successful!"
	@rm -f log.txt