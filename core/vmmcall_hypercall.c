#include "vmmcall.h"
#include "initfunc.h"
#include "printf.h"
#include "panic.h"
#include "current.h"
#include "vt_ept.h"
#include "string.h"
#include "process.h"
#include "mm.h"
#include "cpu_mmu.h"
#include "vt_regs.h"
#include "vmmcall_hypercall.h"

static struct kernel_addr{
  u64 kpgd_addr;
}kernel_addr;

struct memdump_data {
  u64 physaddr;
  u64 cr0, cr3, cr4, efer;
  ulong virtaddr;
  int sendlen;
};

struct memdump_hvirt_data {
  u64 *virtsrc, *virtdes;
  int sendlen;
};


static void 
printhex(unsigned char * data, int length, u64 addr){
  int i, j;
  unsigned char * tmp = data;
  
  for(j = 1; j <= length/8; ++j){
    //print address
    printf("0x%08llx ", addr);
    //print data in that address
    for(i=0; i < 8; ++i){
      printf(" %02X", tmp[i]);
    }
    
    for (i = 0; i < 8; ++i)
      { 
	if (isprint(tmp[i]))
	{
	 printf(" %c", tmp[i]);
	}
	else
	printf(" .");
      }
    
    printf("\n");
    addr+=0x8;
    tmp = data+8*j;
  }
}

void
memdump_hphys(u64 hphys){
  u8 * phphys;
  int length = 128;
  phphys = mapmem_hphys (hphys, length, 0);
  static char * hpdes; 
  hpdes = alloc(length);
  memset(hpdes, 0, length);
  memcpy(hpdes, phphys, length);
  printhex(hpdes, length, (u64)hpdes);
}

static void 
get_control_regs(ulong * cr0, ulong * cr3, ulong * cr4, u64 * efer){
  current->vmctl.read_control_reg (CONTROL_REG_CR0, cr0);
  current->vmctl.read_control_reg (CONTROL_REG_CR3, cr3);
  current->vmctl.read_control_reg (CONTROL_REG_CR4, cr4);
  current->vmctl.read_msr (MSR_IA32_EFER, efer);
}

void memdump_gphys(long gphys, int type, void * value){
  char data[16];
  memset(data, 0, sizeof(data));
  
  if (type == 0) {
    read_gphys_b(gphys, (u32 *)value, 0);
  } else if (type == 1) {
    read_gphys_b(gphys, (u64 *)value, 0);
  }
  //printhex(data, 16, gphys);
  
}

int 
memdump_gvirt(struct memdump_data * dumpdata, void * value){
  long gvirt = 0xffffffff81c16000ULL - (0xffffffff81000000ULL - 0x0000000001000000ULL);
  struct memdump_data * data = dumpdata;
  int i, r;
  u64 ent[5];
  int levels;
  u64 physaddr;
  int width = 8;
  char str[width];
  memset(ent, 0, sizeof(ent));
  get_control_regs((ulong *)&data->cr0, (ulong *)&data->cr3, (ulong *)&data->cr4, &data->efer);
  
  for (i = 0; i < data->sendlen; i += width)
    {
      r = cpu_mmu_get_pte(data->virtaddr+i, (ulong)data->cr0, (ulong)data->cr3, (ulong)data->cr4, 
		      data->efer, true, false, false, ent, 
		      &levels);
      if (r == VMMERR_SUCCESS)
	{
	  physaddr = (ent[0] & PTE_ADDR_MASK64) | ((data->virtaddr+i) & 0xFFF);
	  read_hphys_q(physaddr, str, 0);
	  memcpy(value+i, str, width);
	  //printhex(str, width, physaddr);
	  memset(str, 0, width);
	}
      else{
	printf("error: r = %d\n", r);
	break;
      }
    }
  return r;
  
}

u64 vmi_translate_ksym2v(char * ksym)
{
  u64 addr = 0;
  if ( !strcmp(ksym, "init_task") ) {
    addr  = 0xffffffff81c1d4e0;
  }
  return addr;
}

u64 vmi_get_offset(char *offset_name)
{
  if (strcmp(offset_name, "linux_tasks") == 0) {
    return 0x338;
  } else  if (strcmp(offset_name, "linux_pid") == 0) {
    return 0x3f8;
  } else  if (strcmp(offset_name, "linux_name") == 0) {
    return 0x5a0;
  }
  return 0x0;
}

int
virt_memcpy(ulong virtaddr, int nr_bytes, void * value){
  struct memdump_data data;
  u64 ent[5];
  int r, levels;
  memset(ent, 0, sizeof(ent));
  memset(&data, 0, sizeof(struct memdump_data));
  data.virtaddr = virtaddr;
  data.cr3 = 0xffffffff81c16000ULL - (0xffffffff81000000ULL - 0x0000000001000000ULL);
  get_control_regs((ulong *)&data.cr0, (ulong *)&data.cr3, (ulong *)&data.cr4, &data.efer);
  r = cpu_mmu_get_pte(data.virtaddr, (ulong)data.cr0, (ulong)data.cr3, (ulong)data.cr4, data.efer, false, false, false, ent, &levels);
  /* printf("[%s r=%d 0x%016llx]\n", __func__, r, virtaddr); */
  if (r == VMMERR_SUCCESS)
    {
      
      data.physaddr = (ent[0] & PTE_ADDR_MASK64) | ((data.virtaddr) & 0xFFF);
      if(nr_bytes == 4)
	read_hphys_l(data.physaddr, value, 0);
      if(nr_bytes == 8)
	read_hphys_q(data.physaddr, value, 0);
      return VMI_SUCCESS;
    }
  return VMI_FAILURE;
}

char *
virt_strcpy(ulong vaddr){
  char * str;
  str = alloc(PAGESIZE);
  if(!str){
    printf("can't alloc a page of memory\n");
    return NULL;
  }
  int index = 0,i;
  u64 dtbaddr = kernel_addr.kpgd_addr;
  struct memdump_data data;
  memset(&data, 0, sizeof(data));
  data.sendlen = PAGESIZE;
  data.virtaddr = vaddr;
  memdump_gvirt(&data, str);
  return str;
}

static void
get_process_list (void){
  kernel_addr.kpgd_addr = 0xffffffff81c16000ULL - (0xffffffff81000000ULL - 0x0000000001000000ULL);
  u64 current_process;
  unsigned long tasks_offset, pid_offset, name_offset;
  u64 list_head = 0, current_list_entry = 0, next_list_entry = 0;
  u32 pid = 0;
  char *procname = NULL;
  int i, status;

  tasks_offset = vmi_get_offset("linux_tasks"); 
  pid_offset = vmi_get_offset("linux_pid");
  name_offset = vmi_get_offset("linux_name");

  list_head = vmi_translate_ksym2v("init_task") + tasks_offset;
  printf("list_head: %016llx\n", list_head);
  next_list_entry = list_head;
  do
    {
      current_process = next_list_entry - tasks_offset;
      virt_memcpy(current_process + pid_offset, 4, &pid);
      char * tmp;
      if(!tmp){
	printf("error! can't get process name!\n");
	return;
      }
      tmp = virt_strcpy(current_process + name_offset);
      printf("[%5d] %s (struct addr:%lx)\n", pid, tmp, current_process);
      status = virt_memcpy(next_list_entry, 8, &next_list_entry);
      if (status != VMMERR_SUCCESS)
      	{
      	  printf("failed to read next pointer in loop at %x\n", next_list_entry);
      	  return;
      	}
    }while(next_list_entry != list_head);
}


static void
vmmcall_processlist_init (void){
  vmmcall_register ("vmmcall_processlist", get_process_list);
}

INITFUNC ("vmmcal0", vmmcall_processlist_init);
