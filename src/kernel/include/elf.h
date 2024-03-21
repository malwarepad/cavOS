#include "types.h"

typedef uint16_t Elf64_Half;   // Unsigned medium integer
typedef uint64_t Elf64_Off;    // Unsigned file offset
typedef uint64_t Elf64_Addr;   // Unsigned program address
typedef uint32_t Elf64_Word;   // Unsigned integer
typedef int32_t  Elf64_Sword;  // Signed integer
typedef uint64_t Elf64_Xword;  // Unsigned long integer
typedef int64_t  Elf64_Sxword; // Signed long integer

#define ELF_NIDENT 16

typedef struct {
  unsigned char e_ident[16]; /* ELF identification */
  Elf64_Half    e_type;      /* Object file type */
  Elf64_Half    e_machine;   /* Machine type */
  Elf64_Word    e_version;   /* Object file version */
  Elf64_Addr    e_entry;     /* Entry point address */
  Elf64_Off     e_phoff;     /* Program header offset */
  Elf64_Off     e_shoff;     /* Section header offset */
  Elf64_Word    e_flags;     /* Processor-specific flags */
  Elf64_Half    e_ehsize;    /* ELF header size */
  Elf64_Half    e_phentsize; /* Size of program header entry */
  Elf64_Half    e_phnum;     /* Number of program header entries */
  Elf64_Half    e_shentsize; /* Size of section header entry */
  Elf64_Half    e_shnum;     /* Number of section header entries */
  Elf64_Half    e_shstrndx;  /* Section name string table index */
} Elf64_Ehdr;

typedef struct {
  Elf64_Word  p_type;   /* Type of segment */
  Elf64_Word  p_flags;  /* Segment attributes */
  Elf64_Off   p_offset; /* Offset in file */
  Elf64_Addr  p_vaddr;  /* Virtual address in memory */
  Elf64_Addr  p_paddr;  /* Reserved */
  Elf64_Xword p_filesz; /* Size of segment in file */
  Elf64_Xword p_memsz;  /* Size of segment in memory */
  Elf64_Xword p_align;  /* Alignment of segment */
} Elf64_Phdr;

typedef struct {
  Elf64_Word  sh_name;      /* Section name */
  Elf64_Word  sh_type;      /* Section type */
  Elf64_Xword sh_flags;     /* Section attributes */
  Elf64_Addr  sh_addr;      /* Virtual address in memory */
  Elf64_Off   sh_offset;    /* Offset in file */
  Elf64_Xword sh_size;      /* Size of section */
  Elf64_Word  sh_link;      /* Link to other section */
  Elf64_Word  sh_info;      /* Miscellaneous information */
  Elf64_Xword sh_addralign; /* Address alignment boundary */
  Elf64_Xword sh_entsize;   /* Size of entries, if section has table */
} Elf64_Shdr;

#define PT_NULL 0
#define PT_LOAD 1
#define PT_DYNAMIC 2
#define PT_INTERP 3
#define PT_NOTE 4
#define PT_SHLIB 5
#define PT_PHDR 6
#define PT_LOPROC 0x70000000
#define PT_HIPROC 0x7fffffff

enum Elf_Ident {
  EI_MAG0 = 0,       // 0x7F
  EI_MAG1 = 1,       // 'E'
  EI_MAG2 = 2,       // 'L'
  EI_MAG3 = 3,       // 'F'
  EI_CLASS = 4,      // Architecture (32/64)
  EI_DATA = 5,       // Byte Order
  EI_VERSION = 6,    // ELF Version
  EI_OSABI = 7,      // OS Specific
  EI_ABIVERSION = 8, // OS Specific
  EI_PAD = 9         // Padding
};

#define ELFMAG0 0x7F // e_ident[EI_MAG0]
#define ELFMAG1 'E'  // e_ident[EI_MAG1]
#define ELFMAG2 'L'  // e_ident[EI_MAG2]
#define ELFMAG3 'F'  // e_ident[EI_MAG3]

#define ELFDATA2LSB (1) // Little Endian

#define ELFCLASS32 (1)    // 32-bit Architecture
#define ELF_x86_MACHINE 3 // 32-bit Architecture (later)

#define ELFCLASS64 (2)          // 64-bit Architecture
#define ELF_x86_64_MACHINE 0x3E // 64-bit Architecture (later)

enum Elf_Type {
  ET_NONE = 0, // Unkown Type
  ET_REL = 1,  // Relocatable File
  ET_EXEC = 2  // Executable File
};

#ifndef ELF_H
#define ELF_H

bool     elf_check_file(Elf64_Ehdr *hdr);
int16_t  create_taskid();
uint32_t elf_execute(char *filepath, uint32_t argc, char **argv);

#endif