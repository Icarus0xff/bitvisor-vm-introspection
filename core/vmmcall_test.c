#include "vmmcall.h"
#include "initfunc.h"
#include "printf.h"
#include "panic.h"
#include "current.h"
#include "vt_ept.h"

static void
test (void)
{
  ulong rbx;
  printf("test vmmcall_test!\n");
  current->vmctl.read_general_reg (GENERAL_REG_RBX, &rbx);
  printf("int a is: %ul\n", rbx);
  
  u64 gphys = 0x0ULL;
  vt_ept_set_mem_region(gphys, (int)rbx);
  //  panic(__func__);
}

static void
vmmcall_test_init (void)
{
	vmmcall_register ("vmmcall_test", test);
}

INITFUNC ("vmmcal0", vmmcall_test_init);
