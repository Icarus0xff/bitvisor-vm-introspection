#include "../common/call_vmm.h" 

typedef enum status{
  HYPERCALL_SUCESS,
  HYPERCALL_FAIL
} status_t;

typedef unsigned long long int u64;
typedef unsigned long int ulong;
