#include <stdio.h>
#include "hypercall.h"

static int
vmmcall_process_list (void)
{
  call_vmm_function_t f;
  call_vmm_arg_t a;
  call_vmm_ret_t r;
	
  char str[] = "hello, world!";
  CALL_VMM_GET_FUNCTION ("vmmcall_processlist", &f);
  if (!call_vmm_function_callable (&f)){
    printf("error!\n");
    return HYPERCALL_FAIL;
  }
  u64 addr = (u64)str;
  a.rbx = addr;
  a.rcx = sizeof(str)/sizeof(char);
  a.rcx = 1;
  call_vmm_call_function (&f, &a, &r);
  return HYPERCALL_SUCESS;
}

static void 
get_process_list()
{
  if(vmmcall_process_list() == HYPERCALL_SUCESS){
    printf("sucess!\n");
    return;
  }
  else{
    printf("fail to get process list, maybe u set a wrong kernel parameter!\n");
    return;
  }

}

int
main()
{
  printf("Start to get process list!\n");
  get_process_list();
}
