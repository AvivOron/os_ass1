#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "defs.h"
#include "x86.h"
#include "elf.h"

typedef struct {
  char path[200];
  int len;
} path_directory;
 
path_directory path_directories[50];
char dir_and_file_str[400];

struct inode* checkInPathDirs(char* filename){
    struct inode *ip, *pathContent;
    char content[1000];

    memset(content, 0, 1000);
    pathContent = namei("/path");
    ilock(pathContent);
    readi(pathContent, content, 0, 1000);
    iunlockput(pathContent);

    int i,n,pos,count,length, currIndex;
    path_directory *dir;
    pos = 0;
    count = 0;

    memset(path_directories, 0, sizeof(path_directory) * 50);

    length = strlen(content);
    for(i = 0; i < length; i++){
        if(content[i] == ':'){
            path_directories[count].len = pos;
            path_directories[count].path[pos] = '\0';
            count++;
            pos = 0;
        }else{
            if(pos < 199){
                path_directories[count].path[pos] = content[i];
                pos++;
            }
        }
    }
 
    length = strlen(filename);

    for(i = 0; i < count; i++){
        currIndex = 0;
        dir = path_directories + i;

        memset(dir_and_file_str, 0, sizeof(char) * 400);

        for(n = 0; n < dir->len; n++){
            dir_and_file_str[n] = dir->path[n];
        }

        currIndex = dir->len;
        for(n=0; n < length; n++){
            dir_and_file_str[currIndex++] = filename[n];
        }

        dir_and_file_str[currIndex] = '\0';

        if((ip = namei(dir_and_file_str)) != 0){
            return ip;
        }
    }

    return 0;
}


void 
pseudo_main(int (*entry)(int, char**), int argc, char **argv) 
{
  int status = entry(argc, argv);

  asm("movl   %0,(%%esp)\n\t" // i dont know why we put status into esp
      "push $0x0\n\t" // i dont know why we push 0 into stack (doesnt work without it)
      "mov    $0x2,%%eax\n\t" // call sys_exit
      "int    $0x40\n\t" 
      "ret\n\t" 
             : /* no output operands */
             : "r" (status) /* input operands */
             : "%eax", "%esp" /* clobbered registers */
  );
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

  ustack[0] = 0xffffffff;  // fake return PC
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
