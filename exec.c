#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "defs.h"
#include "x86.h"
#include "elf.h"

#define SIZE_OF_PATH 200  
#define NUM_OF_DIRS_IN_PATH 50    

typedef struct _path_dir{
  char path[SIZE_OF_PATH];
  int len;
} path_dir;
 
path_dir path_dirs[NUM_OF_DIRS_IN_PATH];

char file_path[SIZE_OF_PATH + 100];

struct inode* checkInPathDirs(char* path){
    struct inode *ip, *pathContent;
    char content[SIZE_OF_PATH];
    
    memset(content, 0, SIZE_OF_PATH);
    pathContent = namei("/path");
    ilock(pathContent);
    readi(pathContent, content, 0, SIZE_OF_PATH);
    iunlockput(pathContent);
    //cprintf(content);
    
    int pos = 0;
    int i = 0;
    int length = 0;
    int count = 0;
    path_dir *p;
    int n;
    
  memset(path_dirs, 0, sizeof(path_dir) * NUM_OF_DIRS_IN_PATH);
  
  length = strlen(content);
  pos = 0;
  count = 0;
  for(i = 0; i < length; i++){
    if(content[i] == ':'){
      path_dirs[count].path[pos] = '\0';
      path_dirs[count].len = pos;
      count ++;
      pos = 0;
    }else{
      if(pos < SIZE_OF_PATH - 1){
        path_dirs[count].path[pos] = content[i];
        pos++;
      }
    }
  }
 


  length = strlen(path);

    for(i = 0; i < count; i++){
      p = path_dirs + i;
 
      memset(file_path, 0, sizeof(char) * SIZE_OF_PATH + 100);

      n = 0;

      for(n=0; n < p->len; n++){
        file_path[n] = p->path[n];
      }

      for(n=0; n < length; n++){
        file_path[p->len + n] = path[n];
      }

      file_path[p->len + length] = '\0';
      
      if((ip = namei(file_path)) != 0){
        return ip;
      }
    }
  
    return 0;
}


void 
pseudo_main(int (*entry)(int, char**), int argc, char **argv) 
{
  //int status = entry(argc, argv);
  entry(argc, argv);
  asm("mov    $0x2,%eax\n\t"
      //"mov    " (status) ",%ebx\n\t"
      "int    $0x40\n\t"
      "ret\n\t");
}

int
exec(char *path, char **argv)
{
  char *s, *last;
  int i, off;
  uint argc, sz, sp, ustack[3+MAXARG+1];
  uint pointer_pseudo_main;
  struct elfhdr elf;
  struct inode *ip;
  struct proghdr ph;
  pde_t *pgdir, *oldpgdir;

  begin_op();

  if((ip = namei(path)) == 0 && (ip = checkInPathDirs(path)) == 0){
    end_op();
    return -1;
  }
  ilock(ip);
  pgdir = 0;

  // Check ELF header
  if(readi(ip, (char*)&elf, 0, sizeof(elf)) != sizeof(elf))
    goto bad;
  if(elf.magic != ELF_MAGIC)
    goto bad;

  if((pgdir = setupkvm()) == 0)
    goto bad;

  // Load program into memory.
  sz = 0;
  for(i=0, off=elf.phoff; i<elf.phnum; i++, off+=sizeof(ph)){
    if(readi(ip, (char*)&ph, off, sizeof(ph)) != sizeof(ph))
      goto bad;
    if(ph.type != ELF_PROG_LOAD)
      continue;
    if(ph.memsz < ph.filesz)
      goto bad;
    if(ph.vaddr + ph.memsz < ph.vaddr)
      goto bad;
    if((sz = allocuvm(pgdir, sz, ph.vaddr + ph.memsz)) == 0)
      goto bad;
    if(ph.vaddr % PGSIZE != 0)
      goto bad;
    if(loaduvm(pgdir, (char*)ph.vaddr, ip, ph.off, ph.filesz) < 0)
      goto bad;
  }
  iunlockput(ip);
  end_op();
  ip = 0;

  pointer_pseudo_main = sz;  


  // Allocate two pages at the next page boundary.
  // Make the first inaccessible.  Use the second as the user stack.
  sz = PGROUNDUP(sz);
  if((sz = allocuvm(pgdir, sz, sz + 3*PGSIZE)) == 0)
    goto bad;
  clearpteu(pgdir, (char*)(sz - 2*PGSIZE));
  
  if (copyout(pgdir, pointer_pseudo_main, pseudo_main, (uint)exec - (uint)pseudo_main) < 0)
    goto bad;

  sp = sz;

  // Push argument strings, prepare rest of stack in ustack.
  for(argc = 0; argv[argc]; argc++) {
    if(argc >= MAXARG)
      goto bad;
    sp = (sp - (strlen(argv[argc]) + 1)) & ~3;
    if(copyout(pgdir, sp, argv[argc], strlen(argv[argc]) + 1) < 0)
      goto bad;
    ustack[4+argc] = sp;
  }
  ustack[4+argc] = 0;

  ustack[0] = 8;  // fake return PC
  ustack[1] = elf.entry;
  ustack[2] = argc;
  ustack[3] = sp - (argc+1)*4;  // argv pointer

  sp -= (4+argc+1) * 4;
  if(copyout(pgdir, sp, ustack, (4+argc+1)*4) < 0)
    goto bad;

  // Save program name for debugging.
  for(last=s=path; *s; s++)
    if(*s == '/')
      last = s+1;
  safestrcpy(proc->name, last, sizeof(proc->name));

  // Commit to the user image.
  oldpgdir = proc->pgdir;
  proc->pgdir = pgdir;
  proc->sz = sz;
  proc->tf->eip = pointer_pseudo_main;  // main
  proc->tf->esp = sp;
  switchuvm(proc);
  freevm(oldpgdir);
  return 0;

 bad:
  if(pgdir)
    freevm(pgdir);
  if(ip){
    iunlockput(ip);
    end_op();
  }
  return -1;

}
