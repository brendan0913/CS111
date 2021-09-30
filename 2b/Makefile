# NAME: Brendan Rossmango
# EMAIL: brendan0913@ucla.edu
# ID: 505370692

default:
	gcc -Wall -Wextra -pthread -lprofiler -g lab2_list.c SortedList.c -o lab2_list

clean:
	rm -f lab2_list *.gz

dist: graphs profile
	tar -czvf lab2b-505370692.tar.gz Makefile README lab2_list.c SortedList.c SortedList.h lab2b_list.csv profile.out \
	lab2b_1.png lab2b_2.png lab2b_3.png lab2b_4.png lab2b_5.png lab2b_list.gp
 
tests: default
	@rm -f *.csv
	# lab2b_1.png: throughput (ns) vs number of threads
	./lab2_list --threads=1 --iterations=1000 --sync=m >> lab2b_list.csv
	./lab2_list --threads=2 --iterations=1000 --sync=m >> lab2b_list.csv
	./lab2_list --threads=4 --iterations=1000 --sync=m >> lab2b_list.csv
	./lab2_list --threads=8 --iterations=1000 --sync=m >> lab2b_list.csv
	./lab2_list --threads=12 --iterations=1000 --sync=m >> lab2b_list.csv
	./lab2_list --threads=16 --iterations=1000 --sync=m >> lab2b_list.csv
	./lab2_list --threads=24 --iterations=1000 --sync=m >> lab2b_list.csv
	./lab2_list --threads=1 --iterations=1000 --sync=s >> lab2b_list.csv
	./lab2_list --threads=2 --iterations=1000 --sync=s >> lab2b_list.csv
	./lab2_list --threads=4 --iterations=1000 --sync=s >> lab2b_list.csv
	./lab2_list --threads=8 --iterations=1000 --sync=s >> lab2b_list.csv
	./lab2_list --threads=12 --iterations=1000 --sync=s >> lab2b_list.csv
	./lab2_list --threads=16 --iterations=1000 --sync=s >> lab2b_list.csv
	./lab2_list --threads=24 --iterations=1000 --sync=s >> lab2b_list.csv

	# lab2b_2.png: wait-for-lock time, average time per operation (ns) vs number of threads
	# use same mutex tests as for lab2b_1.png

	# lab2b_3.png: insert+delete yield, 4 lists, 1-16 threads and iterations, no sync options
	# vs insert+delete yield, 4 lists, 1-16 threads, 10-80 iterations, sync options 
	# expect no-sync options to reliably fail
	-./lab2_list --threads=1 --iterations=1 --yield=id --lists=4 >> lab2b_list.csv
	-./lab2_list --threads=1 --iterations=2 --yield=id --lists=4 >> lab2b_list.csv
	-./lab2_list --threads=1 --iterations=4 --yield=id --lists=4 >> lab2b_list.csv
	-./lab2_list --threads=1 --iterations=8 --yield=id --lists=4 >> lab2b_list.csv
	-./lab2_list --threads=1 --iterations=16 --yield=id --lists=4 >> lab2b_list.csv
	-./lab2_list --threads=4 --iterations=1 --yield=id --lists=4 >> lab2b_list.csv
	-./lab2_list --threads=4 --iterations=2 --yield=id --lists=4 >> lab2b_list.csv
	-./lab2_list --threads=4 --iterations=4 --yield=id --lists=4 >> lab2b_list.csv
	-./lab2_list --threads=4 --iterations=8 --yield=id --lists=4 >> lab2b_list.csv
	-./lab2_list --threads=4 --iterations=16 --yield=id --lists=4 >> lab2b_list.csv
	-./lab2_list --threads=8 --iterations=1 --yield=id --lists=4 >> lab2b_list.csv
	-./lab2_list --threads=8 --iterations=2 --yield=id --lists=4 >> lab2b_list.csv
	-./lab2_list --threads=8 --iterations=4 --yield=id --lists=4 >> lab2b_list.csv
	-./lab2_list --threads=8 --iterations=8 --yield=id --lists=4 >> lab2b_list.csv
	-./lab2_list --threads=8 --iterations=16 --yield=id --lists=4 >> lab2b_list.csv
	-./lab2_list --threads=12 --iterations=1 --yield=id --lists=4 >> lab2b_list.csv
	-./lab2_list --threads=12 --iterations=2 --yield=id --lists=4 >> lab2b_list.csv
	-./lab2_list --threads=12 --iterations=4 --yield=id --lists=4 >> lab2b_list.csv
	-./lab2_list --threads=12 --iterations=8 --yield=id --lists=4 >> lab2b_list.csv
	-./lab2_list --threads=12 --iterations=16 --yield=id --lists=4 >> lab2b_list.csv
	-./lab2_list --threads=16 --iterations=1 --yield=id --lists=4 >> lab2b_list.csv
	-./lab2_list --threads=16 --iterations=2 --yield=id --lists=4 >> lab2b_list.csv
	-./lab2_list --threads=16 --iterations=4 --yield=id --lists=4 >> lab2b_list.csv
	-./lab2_list --threads=16 --iterations=8 --yield=id --lists=4 >> lab2b_list.csv
	-./lab2_list --threads=16 --iterations=16 --yield=id --lists=4 >> lab2b_list.csv
	./lab2_list --threads=1 --iterations=10 --yield=id --lists=4 --sync=m >> lab2b_list.csv
	./lab2_list --threads=1 --iterations=20 --yield=id --lists=4 --sync=m >> lab2b_list.csv
	./lab2_list --threads=1 --iterations=40 --yield=id --lists=4 --sync=m >> lab2b_list.csv
	./lab2_list --threads=1 --iterations=80 --yield=id --lists=4 --sync=m >> lab2b_list.csv
	./lab2_list --threads=4 --iterations=10 --yield=id --lists=4 --sync=m >> lab2b_list.csv
	./lab2_list --threads=4 --iterations=20 --yield=id --lists=4 --sync=m >> lab2b_list.csv
	./lab2_list --threads=4 --iterations=40 --yield=id --lists=4 --sync=m >> lab2b_list.csv
	./lab2_list --threads=4 --iterations=80 --yield=id --lists=4 --sync=m >> lab2b_list.csv
	./lab2_list --threads=8 --iterations=10 --yield=id --lists=4 --sync=m >> lab2b_list.csv
	./lab2_list --threads=8 --iterations=20 --yield=id --lists=4 --sync=m >> lab2b_list.csv
	./lab2_list --threads=8 --iterations=40 --yield=id --lists=4 --sync=m >> lab2b_list.csv
	./lab2_list --threads=8 --iterations=80 --yield=id --lists=4 --sync=m >> lab2b_list.csv
	./lab2_list --threads=12 --iterations=10 --yield=id --lists=4 --sync=m >> lab2b_list.csv
	./lab2_list --threads=12 --iterations=20 --yield=id --lists=4 --sync=m >> lab2b_list.csv
	./lab2_list --threads=12 --iterations=40 --yield=id --lists=4 --sync=m >> lab2b_list.csv
	./lab2_list --threads=12 --iterations=80 --yield=id --lists=4 --sync=m >> lab2b_list.csv
	./lab2_list --threads=16 --iterations=10 --yield=id --lists=4 --sync=m >> lab2b_list.csv
	./lab2_list --threads=16 --iterations=20 --yield=id --lists=4 --sync=m >> lab2b_list.csv
	./lab2_list --threads=16 --iterations=40 --yield=id --lists=4 --sync=m >> lab2b_list.csv
	./lab2_list --threads=16 --iterations=80 --yield=id --lists=4 --sync=m >> lab2b_list.csv
	./lab2_list --threads=1 --iterations=10 --yield=id --lists=4 --sync=s >> lab2b_list.csv
	./lab2_list --threads=1 --iterations=20 --yield=id --lists=4 --sync=s >> lab2b_list.csv
	./lab2_list --threads=1 --iterations=40 --yield=id --lists=4 --sync=s >> lab2b_list.csv
	./lab2_list --threads=1 --iterations=80 --yield=id --lists=4 --sync=s >> lab2b_list.csv
	./lab2_list --threads=4 --iterations=10 --yield=id --lists=4 --sync=s >> lab2b_list.csv
	./lab2_list --threads=4 --iterations=20 --yield=id --lists=4 --sync=s >> lab2b_list.csv
	./lab2_list --threads=4 --iterations=40 --yield=id --lists=4 --sync=s >> lab2b_list.csv
	./lab2_list --threads=4 --iterations=80 --yield=id --lists=4 --sync=s >> lab2b_list.csv
	./lab2_list --threads=8 --iterations=10 --yield=id --lists=4 --sync=s >> lab2b_list.csv
	./lab2_list --threads=8 --iterations=20 --yield=id --lists=4 --sync=s >> lab2b_list.csv
	./lab2_list --threads=8 --iterations=40 --yield=id --lists=4 --sync=s >> lab2b_list.csv
	./lab2_list --threads=8 --iterations=80 --yield=id --lists=4 --sync=s >> lab2b_list.csv
	./lab2_list --threads=12 --iterations=10 --yield=id --lists=4 --sync=s >> lab2b_list.csv
	./lab2_list --threads=12 --iterations=20 --yield=id --lists=4 --sync=s >> lab2b_list.csv
	./lab2_list --threads=12 --iterations=40 --yield=id --lists=4 --sync=s >> lab2b_list.csv
	./lab2_list --threads=12 --iterations=80 --yield=id --lists=4 --sync=s >> lab2b_list.csv
	./lab2_list --threads=16 --iterations=10 --yield=id --lists=4 --sync=s >> lab2b_list.csv
	./lab2_list --threads=16 --iterations=20 --yield=id --lists=4 --sync=s >> lab2b_list.csv
	./lab2_list --threads=16 --iterations=40 --yield=id --lists=4 --sync=s >> lab2b_list.csv
	./lab2_list --threads=16 --iterations=80 --yield=id --lists=4 --sync=s >> lab2b_list.csv

	# lab2b_4.png: no yield, 1-12 threads, 1000 iterations, 1-16 lists, mutex sync
	./lab2_list --threads=1 --iterations=1000 --lists=4 --sync=m >> lab2b_list.csv
	./lab2_list --threads=1 --iterations=1000 --lists=8 --sync=m >> lab2b_list.csv
	./lab2_list --threads=1 --iterations=1000 --lists=16 --sync=m >> lab2b_list.csv
	./lab2_list --threads=2 --iterations=1000 --lists=4 --sync=m >> lab2b_list.csv
	./lab2_list --threads=2 --iterations=1000 --lists=8 --sync=m >> lab2b_list.csv
	./lab2_list --threads=2 --iterations=1000 --lists=16 --sync=m >> lab2b_list.csv
	./lab2_list --threads=4 --iterations=1000 --lists=4 --sync=m >> lab2b_list.csv
	./lab2_list --threads=4 --iterations=1000 --lists=8 --sync=m >> lab2b_list.csv
	./lab2_list --threads=4 --iterations=1000 --lists=16 --sync=m >> lab2b_list.csv
	./lab2_list --threads=8 --iterations=1000 --lists=4 --sync=m >> lab2b_list.csv
	./lab2_list --threads=8 --iterations=1000 --lists=8 --sync=m >> lab2b_list.csv
	./lab2_list --threads=8 --iterations=1000 --lists=16 --sync=m >> lab2b_list.csv
	./lab2_list --threads=12 --iterations=1000 --lists=4 --sync=m >> lab2b_list.csv
	./lab2_list --threads=12 --iterations=1000 --lists=8 --sync=m >> lab2b_list.csv
	./lab2_list --threads=12 --iterations=1000 --lists=16 --sync=m >> lab2b_list.csv

	# lab2b_5.png: no yield, 1-12 threads, 1000 iterations, 1-16 lists, spin-lock sync
	./lab2_list --threads=1 --iterations=1000 --lists=4 --sync=s >> lab2b_list.csv
	./lab2_list --threads=1 --iterations=1000 --lists=8 --sync=s >> lab2b_list.csv
	./lab2_list --threads=1 --iterations=1000 --lists=16 --sync=s >> lab2b_list.csv
	./lab2_list --threads=2 --iterations=1000 --lists=4 --sync=s >> lab2b_list.csv
	./lab2_list --threads=2 --iterations=1000 --lists=8 --sync=s >> lab2b_list.csv
	./lab2_list --threads=2 --iterations=1000 --lists=16 --sync=s >> lab2b_list.csv
	./lab2_list --threads=4 --iterations=1000 --lists=4 --sync=s >> lab2b_list.csv
	./lab2_list --threads=4 --iterations=1000 --lists=8 --sync=s >> lab2b_list.csv
	./lab2_list --threads=4 --iterations=1000 --lists=16 --sync=s >> lab2b_list.csv
	./lab2_list --threads=8 --iterations=1000 --lists=4 --sync=s >> lab2b_list.csv
	./lab2_list --threads=8 --iterations=1000 --lists=8 --sync=s >> lab2b_list.csv
	./lab2_list --threads=8 --iterations=1000 --lists=16 --sync=s >> lab2b_list.csv
	./lab2_list --threads=12 --iterations=1000 --lists=4 --sync=s >> lab2b_list.csv
	./lab2_list --threads=12 --iterations=1000 --lists=8 --sync=s >> lab2b_list.csv
	./lab2_list --threads=12 --iterations=1000 --lists=16 --sync=s >> lab2b_list.csv

profile: default
	rm -f ./raw.gperf profile.out
	LD_PRELOAD=/usr/lib/libprofiler.so
	CPUPROFILE=./raw.gperf ./lab2_list --threads=12 --iterations=1000 --sync=s
	/usr/bin/pprof --text ./lab2_list ./raw.gperf > profile.out
	/usr/bin/pprof --list=run_threads ./lab2_list ./raw.gperf >> profile.out
	rm -f ./raw.gperf

graphs: tests
	gnuplot ./lab2b_list.gp

