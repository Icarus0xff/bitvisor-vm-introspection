.PHONY : all
all : test

.PHONY : clean
clean :
	$(RM) test

test : test.c ../common/call_vmm.c ../common/call_vmm.h
	$(CC) -g -s -o test test.c ../common/call_vmm.c
