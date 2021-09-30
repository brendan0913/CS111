# NAME: Brendan Rossmango
# EMAIL: brendan0913@ucla.edu
# ID: 505370692

default:
	@rm -f lab3b
	@ln -s lab3b.py lab3b
	@chmod +x lab3b
	@echo "lab3b link successfully created"

clean:
	rm -f lab3b *.gz

dist: default
	tar -czvf lab3b-505370692.tar.gz Makefile README lab3b.py
