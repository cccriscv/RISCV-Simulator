#include"Read_Elf.h"
#include <iostream>
#include <string.h>
using namespace std;

FILE *elf=NULL;
FILE * ftmp;
Elf64_Ehdr elf64_hdr;

//Program headers
unsigned int padr=0;
unsigned int psize=0;
unsigned int pnum=0;

//Section Headers
unsigned int sadr=0;
unsigned int ssize=0;
unsigned int snum=0;

//Symbol table
unsigned int symnum=0;
unsigned int symadr=0;
unsigned int symsize=0;

//Section header name string table index
unsigned int sind=0;
unsigned int shstroff=0; // offset

//.symtab: symbols' name
unsigned int stradr=0;

// relocate elf and file
bool open_file(const char* file_name)
{
    /* file: the executable file */
    file = fopen(file_name, "rb");
    if (file == NULL){
        cerr << "Failed to open "<< file_name << endl;
        return false;
    }
    /* ftmp: points to the executable file */
    ftmp = fopen(file_name, "rb");
    if (ftmp == NULL){
        cerr << "Failed to open "<< file_name << endl;
        return false;
    }
    /* elf: the file where the elf will be written into
         can be used in later comparison */
    elf = fopen("elf.txt", "w");
    if (elf == NULL){
        cerr << "Failed to write to 'elf.txt'" << endl;
        return false;
    }
    else
        cout << "Elf-related info will be written to 'elf.txt'"<< endl;
    return true;
}

void read_elf(const char* file_name)
{
	if(!open_file(file_name))
		return ;

	fprintf(elf,"ELF Header:\n");
	read_Elf_header();

	fprintf(elf,"\n\nSection Headers:\n");
	read_elf_sections();

	fprintf(elf,"\n\nProgram Headers:\n");
	read_Phdr();

	fprintf(elf,"\n\nSymbol table:\n");
	read_symtable();

	fclose(elf);
}

void read_Elf_header()
{
	//file should be relocated
	fread(&elf64_hdr,1,sizeof(elf64_hdr),file);
		
	fprintf(elf," magic number:  ");
    for(int i = 0; i < 16; ++i)
        fprintf(elf, "%x ", elf64_hdr.e_ident[i]);
    fprintf(elf, "\n");
    
	fprintf(elf," Class:  ELFCLASS32\n");
	
	fprintf(elf," Data:  little-endian\n");
		
	fprintf(elf," Version:   1 (current)\n");

	fprintf(elf," OS/ABI:	 UNIX - System V\n");
	
	fprintf(elf," ABI Version:   0\n");
	
	fprintf(elf," Type:  \n");
	
	fprintf(elf," Machine:   \n");

	fprintf(elf," Version:  0x%x\n", elf64_hdr.e_version);

	fprintf(elf," Entry point address:  0x%llx\n", elf64_hdr.e_entry);
    //entry = elf64_hdr.e_entry;

	fprintf(elf," Start of program headers:    %lld (bytes into  file)\n", elf64_hdr.e_phoff);
    padr = elf64_hdr.e_phoff;
    //cout << "size of int64: "<< sizeof(elf64_hdr.e_phoff)<< endl;
    
    fprintf(elf," Start of section headers:    %lld (bytes into  file)\n", elf64_hdr.e_shoff);
    sadr = elf64_hdr.e_shoff;
    
	fprintf(elf," Flags:  0x%x\n", elf64_hdr.e_flags);

	fprintf(elf," Size of this header:   %d (bytes)\n", elf64_hdr.e_ehsize);

	fprintf(elf," Size of program headers:   %d (bytes)\n", elf64_hdr.e_phentsize);
    psize = elf64_hdr.e_phentsize;
    
	fprintf(elf," Number of program headers:   %d \n", elf64_hdr.e_phnum);
    pnum = elf64_hdr.e_phnum;
    
	fprintf(elf," Size of section headers:    %d (bytes)\n", elf64_hdr.e_shentsize);
    ssize = elf64_hdr.e_shentsize;
    
	fprintf(elf," Number of section headers:  %d\n", elf64_hdr.e_shnum);
    snum = elf64_hdr.e_shnum;
    
	fprintf(elf," Section header string table index:   %d\n", elf64_hdr.e_shstrndx);
    sind = elf64_hdr.e_shstrndx;
}

void read_elf_sections()
{
    char section_name[30];
	Elf64_Shdr elf64_shdr;
    
    /* find section header string table offset */
    fseek(file, (int)(sadr + ssize * sind), SEEK_SET); // shstradr temporarily points to the section header of .shstrtab
    fread(&elf64_shdr, 1, sizeof(elf64_shdr), file);
    shstroff = elf64_shdr.sh_offset;
    // cout << "offset of .shstrtab" << shstroff<< endl;
    
	for(int c=0;c<snum;c++){
		fprintf(elf," [%3d]\n",c);
		
        /* relocate file to next section header */
        fseek(file, sadr + c * ssize, SEEK_SET);
		fread(&elf64_shdr,1,sizeof(elf64_shdr),file);
        
        fseek(ftmp, shstroff + elf64_shdr.sh_name, SEEK_SET); // elf64_shdr.sh_name is the offset
        fread(section_name, 1, 30, ftmp);
		fprintf(elf," Name: %-15s", section_name);

        fprintf(elf," Type: %-15s", section_types[(int)(elf64_shdr.sh_type)]);
        
        fprintf(elf," Address:  %5llx", elf64_shdr.sh_addr);
        
        fprintf(elf," Offest:  %5llx\n", elf64_shdr.sh_offset);
        
        fprintf(elf," Size:  %llx", elf64_shdr.sh_size);
        
        fprintf(elf," Entsize:  %llx", elf64_shdr.sh_entsize);
        
        fprintf(elf," Flags:   %llx", elf64_shdr.sh_flags);
        
        fprintf(elf," Link:  %x", elf64_shdr.sh_link);
        
        fprintf(elf," Info:  %x", elf64_shdr.sh_info);
        
        fprintf(elf," Align: %llx\n", elf64_shdr.sh_addralign);
        
        if (strcmp(section_name, ".text") == 0){
            cadr = elf64_shdr.sh_offset;
            csize = elf64_shdr.sh_size;
            vadr = elf64_shdr.sh_addr;
        }
        else if(strcmp(section_name, ".data") == 0){
            // gp = elf64_shdr.sh_addr;
            vdadr = elf64_shdr.sh_addr;
            dsize = elf64_shdr.sh_size;
            dadr = elf64_shdr.sh_offset;
        }
        else if(strcmp(section_name, ".symtab") == 0){ // symbol table
            symadr = elf64_shdr.sh_offset;
            symsize = elf64_shdr.sh_size;
            symnum = symsize / sizeof(Elf64_Sym);
        }
        else if(strcmp(section_name, ".strtab") == 0){ // symbols' name table
            stradr = elf64_shdr.sh_offset;
        }
        else if(strcmp(section_name, ".sdata") == 0){
            dsize += elf64_shdr.sh_size;  // also load this in.
        }
 	}
}

void read_Phdr()
{
	Elf64_Phdr elf64_phdr;
	for(int c=0;c<pnum;c++){
		fprintf(elf," [%3d]\n",c);
			
		//file should be relocated
        fseek(file, padr + c * psize, SEEK_SET);
		fread(&elf64_phdr,1,sizeof(elf64_phdr),file);

        fprintf(elf," Type:   %x", elf64_phdr.p_type);
        
        fprintf(elf," Flags:   %x", elf64_phdr.p_flags);
        
        fprintf(elf," Offset:   %llx", elf64_phdr.p_offset);
        
        fprintf(elf," VirtAddr:  %llx", elf64_phdr.p_vaddr);
        
        fprintf(elf," PhysAddr:   %llx", elf64_phdr.p_paddr);
        
        fprintf(elf," FileSiz:   %llx", elf64_phdr.p_filesz);
        
        fprintf(elf," MemSiz:   %llx", elf64_phdr.p_memsz);
        
        fprintf(elf," Align:   %llx", elf64_phdr.p_align);
	}
}


void read_symtable()
{
    char symbol_name[40];
	Elf64_Sym elf64_sym;
	for(int c=0;c<symnum;c++){
		fprintf(elf," [%3d]   ",c);
		
		//file should be relocated
        fseek(file, symadr + c * sizeof(elf64_sym), SEEK_SET);
		fread(&elf64_sym,1,sizeof(elf64_sym),file);

        fseek(ftmp, stradr + elf64_sym.st_name, SEEK_SET); // elf64_shdr.sh_name is the offset
        fread(symbol_name, 1, 40, ftmp);
		fprintf(elf," Name:  %40s   ", symbol_name);

        fprintf(elf," Bind:   %d", elf64_sym.st_info);
        
        fprintf(elf," Type:   %d", elf64_sym.st_info);
        
        fprintf(elf," NDX:   %x", elf64_sym.st_shndx);
        
        fprintf(elf," Size:   %llx", elf64_sym.st_size);
        
        fprintf(elf," Value:   %llx\n", elf64_sym.st_value);
        
        if (strcmp(symbol_name, "main") == 0){
            madr = elf64_sym.st_value;
            entry = madr;
        }
        else if (strcmp(symbol_name, "atexit") == 0){
            endPC = elf64_sym.st_value;
        }
        // get 'gp' from symbol table.
        else if(strcmp(symbol_name, "__global_pointer$") == 0){
            gp = elf64_sym.st_value;
        }
        else if(strcmp(symbol_name, "_gp") == 0){
            gp = elf64_sym.st_value;
        }
	}
}

/* return where a global variable is stored */
unsigned int find_addr(string var_name){
    char symbol_name[40];
    Elf64_Sym elf64_sym;
    for(int c=0;c<symnum;c++){
        //file should be relocated
        fseek(file, symadr + c * sizeof(elf64_sym), SEEK_SET);
        fread(&elf64_sym,1,sizeof(elf64_sym),file);
        
        fseek(ftmp, stradr + elf64_sym.st_name, SEEK_SET); // elf64_shdr.sh_name is the offset
        fread(symbol_name, 1, 40, ftmp);
        
        if (string(symbol_name) == var_name)
            return elf64_sym.st_value;
    }
    return -1; // not found
}
