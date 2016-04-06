#include "printf.h"
#include "initfunc.h"
#include "process.h"
#include "vt_paging.h"
#include "mm.h"
//#include "mmio.h"
static int 
test_msghandler(int m, int c)
{
  printf("start set men privilege!\n");
  //vt_set_mem_test(0x0, 0x0);
  set_syscall_trigger();
  
  return 0;

}

static void 
test_init_msg(void){
  msgregister("test", test_msghandler);
}

void
set_syscall_trigger(void){
  u64 opcode = 0xffffffff811c6290ULL;
  //struct page * p = virt_to_page((virt_t)opcode);
  //virt_t phys = 0;//page_to_virt(page);
  //vt_set_mem_test(opcode, 1024*256);
  phys_t * phys;
  int r = virt_to_phys(0x0,phys);
  printf("%d\n", r);
  printf("llx\n",*phys);
}




INITFUNC ("msg0", test_init_msg);
