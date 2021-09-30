# NAME: Brendan Rossmango
# EMAIL: brendan0913@ucla.edu
# ID: 505370692

#! /usr/bin/gnuplot
#
# purpose:
#	 generate data reduction graphs for the multi-threaded list project
#
# input: lab2_list.csv
#	1. test name
#	2. # threads
#	3. # iterations per thread
#	4. # lists
#	5. # operations performed (threads x iterations x (ins + lookup + delete))
#	6. run time (ns)
#	7. run time per operation (ns)
#    8. average wait-for-lock (ns)
#
# output:
#	lab2b_1.png ... total number of operations per second for each synchronization method
#	lab2b_2.png ... threads and iterations that run (un-protected) w/o failure
#	lab2b_3.png ... threads and iterations that run (protected) w/o failure
#	lab2b_4.png ... cost per operation vs number of threads
#   lab2b_5
#
# Note:
#	Managing data is simplified by keeping all of the results in a single
#	file.  But this means that the individual graphing commands have to
#	grep to select only the data they want.
#
#	Early in your implementation, you will not have data for all of the
#	tests, and the later sections may generate errors for missing data.
#

# general plot parameters
set terminal png
set datafile separator ","

# total number of operations per second for each synchronization method
# lab2b_1.png
set title "List-1: Throughput (ns) vs number of threads for mutex/spin-lock"
set xlabel "Threads"
set logscale x 2
set ylabel "Throughput: operations per sec (ns)"
set logscale y 10
set output 'lab2b_1.png'

# grep out only list-none-m and list-none-s, 1-24 threads, 1000 iterations, 1 list
plot \
     "< grep 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'mutex' with linespoints lc rgb 'blue', \
     "< grep 'list-none-s,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'spin-lock' with linespoints lc rgb 'green', \

set title "List-2: Wait-for-lock time, avg time/op (ns) in mutex synced list"
set xlabel "Threads"
set logscale x 2
set ylabel "Wait-for-lock time, avg time/op (ns)"
set logscale y 10
set xrange [0.75:]
set output 'lab2b_2.png'
plot \
     "< grep 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):($8) \
	title 'wait-for-lock time' with linespoints lc rgb 'red', \
     "< grep 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):($7) \
	title 'average time per op' with linespoints lc rgb 'green'

set title "List-3: Protected and unprotected iterations that run without failure"
set xlabel "Threads"
set logscale x 2
set ylabel "Iterations without failure"
set logscale y 10
set xrange[0.5:20]
set output 'lab2b_3.png'
plot \
     "< grep 'list-id-none,[0-9]*,[0-9]*,4,' lab2b_list.csv" using ($2):($3) \
	with points lc rgb 'red' title 'unprotected', \
     "< grep 'list-id-m,[0-9]*,[0-9]*,4,' lab2b_list.csv" using ($2):($3) \
	with points lc rgb 'blue' title 'mutex', \
     "< grep 'list-id-s,[0-9]*,[0-9]*,4,' lab2b_list.csv" using ($2):($3) \
	with points lc rgb 'green' title 'spin-lock'

set title "List-4: Throughput of partitioned lists synced with mutex"
set xlabel "Threads"
set logscale x 2
set ylabel "Throughput (ns)"
set logscale y 10
set xrange[0.75:]
set output 'lab2b_4.png'
plot \
     "< grep 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title '1 list' with linespoints lc rgb 'red', \
     "< grep 'list-none-m,[0-9]*,1000,4,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title '4 lists' with linespoints lc rgb 'orange', \
     "< grep 'list-none-m,[0-9]*,1000,8,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title '8 lists' with linespoints lc rgb 'green', \
     "< grep 'list-none-m,[0-9]*,1000,16,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title '16 lists' with linespoints lc rgb 'blue'

set title "List-5: Throughput of partitioned lists synced with spin-lock"
set xlabel "Threads"
set logscale x 2
set ylabel "Throughput (ns)"
set logscale y 10
set xrange[0.75:]
set output 'lab2b_5.png'
plot \
     "< grep 'list-none-s,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title '1 list' with linespoints lc rgb 'red', \
     "< grep 'list-none-s,[0-9]*,1000,4,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title '4 lists' with linespoints lc rgb 'orange', \
     "< grep 'list-none-s,[0-9]*,1000,8,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title '8 lists' with linespoints lc rgb 'green', \
     "< grep 'list-none-s,[0-9]*,1000,16,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title '16 lists' with linespoints lc rgb 'blue'