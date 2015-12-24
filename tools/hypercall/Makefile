.PHONY : all
all : processlist

.PHONY : clean
clean :
	$(RM) processlist

processlist : processlist.c ../common/call_vmm.c ../common/call_vmm.h
	$(CC) -g -s -o processlist processlist.c ../common/call_vmm.c
