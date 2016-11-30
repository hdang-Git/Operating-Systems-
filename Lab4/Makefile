# Make file automatically compiles all files together
#
# Sample Commands: 
# 	make 
# 	make remove 
#
# Note: make defaults to target 'all'

all: FileSystem

FileSystem: FileSystem.o 
	cc FileSystem.o -o filesystem

FileSystem.o: FileSystem.c
	cc -c FileSystem.c 

remove:
	rm -rf *o filesystem
