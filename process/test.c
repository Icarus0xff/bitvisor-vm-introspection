#include <lib_lineinput.h>
#include <lib_printf.h>
#include <lib_string.h>
#include <lib_syscalls.h>

void test(){
  int d;

  d = msgopen("test");
  if(d >= 0) {
    msgsendint(d, 0);
    msgclose(d);
    printf("Test tested.\n");
  } else
    printf("test not found.\n");
  
}

_start (int a1, int a2)
{
        printf("Test how to build vmm process! %s\n", __func__);
	test();
	exitprocess (0);
	return 0;
}
