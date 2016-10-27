# Make file automatically compiles all files together
#
# Sample Commands: 
# 	make 
# 	make remove 
#
# Note: make defaults to target 'all'

all: thread

thread: threadAnalysis.o
	cc threadAnalysis.c -o threadA -lpthread

threadAnalysis.o: threadAnalysis.c
	cc -c threadAnalysis.c -lpthread

remove:
	rm -rf *o threadA
