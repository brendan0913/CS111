# NAME: Brendan Rossmango
# EMAIL: brendan0913@ucla.edu
# ID: 505370692

default:
	gcc -g -Wall -Wextra lab0.c -o lab0

clean:
	rm -f lab0 *.tar.gz *.txt

dist: default
	tar -czvf lab0-505370692.tar.gz lab0.c Makefile README *.png

check: default
	@echo "===================================" \
	# check --input is handled correctly (0) \
	echo "Testing 123" > in.txt; \
	./lab0 --input in.txt > result.txt; \
	if [[ $$? -ne 0 ]]; then \
	echo "Incorrect exit code (not 0) for --input"; \
	else \
	echo "Correct exit code (0) for --input"; \
	fi

	@# check that text is copied correctly from input to result \
	cmp in.txt result.txt; \
	if [[ $$? -eq 0 ]]; then \
	echo "Correct copying from input to output for --input"; \
	else \
	echo "Incorrect copying from input to output for --input"; \
	fi \

	@rm result.txt

	@# check --output is handled correctly (0) \
	./lab0 --output out.txt < in.txt; \
	if [[ $$? -ne 0 ]]; then \
	echo "Incorrect exit code (not 0) for --output"; \
	else \
	echo "Correct exit code (0) for --output"; \
	fi

	@# check that text is copied correctly to output from input \
	cmp in.txt out.txt; \
	if [[ $$? -eq 0 ]]; then \
	echo "Correct copying from input to output for --output"; \
	else \
	echo "Incorrect copying from input to output for --output"; \
	fi \

	@rm out.txt

	@# check --input + --output are handled correctly (0) \
	./lab0 --input in.txt --output out.txt &> /dev/null; \
	if [[ $$? -ne 0 ]]; then \
	echo "Incorrect exit code (not 0) for --input + --output"; \
	else \
	echo "Correct exit code (0) for --input + --output"; \
	fi

	@# check that text is copied correctly from input to output \
	cmp in.txt out.txt; \
	if [[ $$? -eq 0 ]]; then \
	echo "Correct copying from input to output for --input + --output"; \
	else \
	echo "Incorrect copying from input to output for --input + --output"; \
	fi \

	@rm out.txt

	@# check that invalid argument is handled correctly (1) \
	echo | ./lab0 --input in.txt --output out.txt --dummy &> /dev/null; \
	if [[ $$? -ne 1 ]]; then \
	echo "Incorrect exit code (not 1) for invalid argument"; \
	else \
	echo "Correct exit code (1) for invalid argument"; \
	fi \

	@# check that nonexistent input file is handled correctly (2) \
	./lab0 --input NONEXISTENT.txt &> /dev/null; \
	if [[ $$? -ne 2 ]]; then \
	echo "Incorrect exit code (not 2) for nonexistent file"; \
	else \
	echo "Correct exit code (2) for nonexistent file"; \
	fi

	@# check that unopenable input file is handled correctly (2) \
	# user does not have read permissions for file \
	echo "unopenable" > UNOPENABLE.txt; \
	chmod u-r UNOPENABLE.txt; \
	./lab0 --input UNOPENABLE.txt --output out.txt &> /dev/null; \
	if [[ $$? -ne 2 ]]; then \
	echo "Incorrect exit code (not 2) for unopenable file"; \
	else \
	echo "Correct exit code (2) for unopenable file"; \
	fi \

	@chmod u=rwx UNOPENABLE.txt; \
	rm UNOPENABLE.txt

	@# check that uncreatable output file is handled correctly (3) \
	# user does not have write permissions for file \
	echo "uncreatable" > UNCREATABLE.txt; \
	chmod u-w UNCREATABLE.txt; \
	./lab0 --input in.txt --output UNCREATABLE.txt &> /dev/null; \
	if [[ $$? -ne 3 ]]; then \
	echo "Incorrect exit code (not 3) for uncreatable file"; \
	else \
	echo "Correct exit code (3) for uncreatable file"; \
	fi \

	@chmod u=rwx UNCREATABLE.txt; \
	rm UNCREATABLE.txt

	@# check that segfault is handled correctly (exit code 139) \
	echo | ./lab0 --segfault &> /dev/null; \
	if [[ $$? -ne 139 ]]; then \
	echo "Incorrect exit code (not 139) for segfault"; \
	else \
	echo "Correct exit code (139) for segfault"; \
	fi

	@# check that segfault and catch are handled correctly (4) \
	echo | ./lab0 --segfault --catch &> /dev/null; \
	if [[ $$? -ne 4 ]]; then \
	echo "Incorrect exit code (not 4) for catching segfault"; \
	else \
	echo "Correct exit code (4) for catching segfault"; \
	fi

	@# check that input is correctly overwritten to output \
	echo "THIS TEXT SHOULD NOT BE HERE AFTER COPY" >> out.txt; \
	echo | ./lab0 --output out.txt --input in.txt &> /dev/null; \
	cmp in.txt out.txt; \
	if [[ $$? -eq 0 ]]; then \
	echo "Correct overwriting of input to output for nonempty output file"; \
	else \
	echo "Incorrect overwriting of input to output for nonempty output file"; \
	fi \

	@rm in.txt out.txt
