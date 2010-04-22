#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include "peresec_private.h"

// Maximum size of function name from input .MAP file.
// Used to verify the input file sanity.
#define MAX_EXPORT_NAMELEN 255
// Maximal amount of exported functions.
// Used to allocate static arrays.
#define MAX_EXPORTS 0x7fff
// Name of the export section to overwrite.
const char export_section_name[]=".edata";

// Text added at end of export section; just for fun
const char export_end_str[] ="Blessed are those who have not seen and yet believe";

// Sections
#define MAX_SECTIONS_NUM 64
#define MAX_SECTION_NAME_LEN 8
unsigned long sections_vaddr[MAX_SECTIONS_NUM];
unsigned long sections_raddr[MAX_SECTIONS_NUM];
unsigned long sections_rsize[MAX_SECTIONS_NUM];
unsigned char sections_names[MAX_SECTIONS_NUM][MAX_SECTION_NAME_LEN+1];

#define EXPORT_DIRECTORY_SIZE   0x0028
#define MZ_SIZEOF_HEADER        0x0040
#define MZ_NEWHEADER_OFS        0x003C
#define PE_SIZEOF_SIGNATURE     4
#define PE_SIZEOF_FILE_HEADER   20
#define PE_NUM_SECTIONS_OFS     2
#define PE_SIZEOF_OPTN_HEADER   96
#define PE_NUM_RVAS_AND_SIZES   92
#define PE_SIZEOF_DATADIR_ENTRY 8
#define PE_SIZEOF_SECTHDR_ENTRY 40

#ifndef false
#define false 0
#endif
#ifndef true
#define true 1
#endif

struct export_entry {
       unsigned short seg;
       unsigned long offs;
       unsigned long nmoffs;
       char srcname[MAX_EXPORT_NAMELEN+1];
       char dstname[MAX_EXPORT_NAMELEN+1];
       };

void export_sort(struct export_entry *exp[],unsigned int exp_size)
{
   int sorted=false;
   unsigned int i;
   struct export_entry *pTemp;
   // Sort the strings in ascending order
   while(!sorted)
   {
     sorted = true;
     for (i=0; i < exp_size-1; i++)
       if(strcmp(exp[i]->dstname, exp[i+1]->dstname) > 0)
       {
         sorted = false;     // We were out of order
         // Swap pointers exp[i] and exp[i+1]
         pTemp = exp[i];
         exp[i] = exp[i+1];
         exp[i+1] = pTemp;
       } 
   }
}

int find_dupename(struct export_entry *exp[],unsigned int exp_size)
{
   int i;
   for (i = 0 ; i < exp_size-1 ; i++)
       if(strcasecmp(exp[i]->dstname, exp[i+1]->dstname) == 0)
       {
         return i;
       } 
   return -1;
}

int find_dupeoffs(struct export_entry *exp[],unsigned int exp_size)
{
   int i,k;
   for (i = 0 ; i < exp_size ; i++)
     for (k = 0 ; k < exp_size ; k++)
       if ((i!=k) && (exp[i]->seg==exp[k]->seg) && (exp[i]->offs==exp[k]->offs))
       {
         return i;
       } 
   return -1;
}

char *get_name_with_prefix(char *dname,char *sname,char *prefix)
{
  int name_begin;
  if (strlen(sname)<2)
  {
    strcpy(dname,prefix);
    int dname_begin=strlen(dname);
    strcpy(dname+dname_begin,sname);
    return dname;
  }
  char name0=sname[0];
  char name1=sname[1];
  if ((name1=='@')||(name1=='?'))
  {
    dname[0]=name0;
    dname[1]=name1;
    name_begin=2;
  } else
  if ((name0=='@')||(name0=='?'))
  {
    dname[0]=name0;
    name_begin=1;
  } else
  {
    name_begin=0;
  }
  strcpy(dname+name_begin,prefix);
  int dname_begin=strlen(dname);
  strncpy(dname+dname_begin,sname+name_begin,MAX_EXPORT_NAMELEN-dname_begin-1);
  dname[MAX_EXPORT_NAMELEN]='\0';
  return dname;
}

/**
 * Reads 2-byte little-endian number from given FILE.
 */
inline unsigned short read_int16_le_file (FILE *fp)
{
    unsigned short l;
    l = fgetc(fp);
    l += fgetc(fp)<<8;
    return l;
}

/**
 * Reads 4-byte little-endian number from given FILE.
 */
inline long read_int32_le_file (FILE *fp)
{
    long l;
    l = fgetc(fp);
    l += fgetc(fp)<<8;
    l += fgetc(fp)<<16;
    l += fgetc(fp)<<24;
    return l;
}

/**
 * Writes 2-byte little-endian number to given FILE.
 */
inline void write_int16_le_file (FILE *fp, unsigned short x)
{
    fputc ((int) (x&255), fp);
    fputc ((int) ((x>>8)&255), fp);
}

/**
 * Writes 4-byte little-endian number to given FILE.
 */
inline void write_int32_le_file (FILE *fp, unsigned long x)
{
    fputc ((int) (x&255), fp);
    fputc ((int) ((x>>8)&255), fp);
    fputc ((int) ((x>>16)&255), fp);
    fputc ((int) ((x>>24)&255), fp);
}

/**
 * Returns length of opened file.
 * Value -1 means error.
 */
inline long file_length_opened (FILE *fp)
{
    long length;
    long lastpos;
    
    if (fp==NULL)
      return -1;
    lastpos = ftell (fp);
    if (fseek(fp, 0, SEEK_END) != 0)
      return -1;
    length = ftell(fp);
    fseek(fp, lastpos, SEEK_SET);
    return length;
}

short show_usage(char *fname)
{
    printf("usage:\n");
    printf("    %s <filename> [fstart]\n", fname);
    printf("where <filename> should be without extension,\n");
    printf("      [fstart] may be used to add prefix to function names.\n");
    return 1;
}

int main(int argc, char *argv[])
{
  printf("\nPE/DLL Rebuilder of Export Section (PeRESec) %s\n",VER_STRING);
  printf("-------------------------------\n");
  if (argc<2)
  {
      show_usage(argv[0]);
      return 11;
  }
  char *module_name=argv[1];
  char *funcname_prefix="";
  if (argc>2)
    funcname_prefix=argv[2];
  static struct export_entry *exports[MAX_EXPORTS];
  int idx;
  for (idx=0;idx<MAX_EXPORTS;idx++)
      exports[idx]=NULL;
  int module_name_size=strlen(module_name)+1;
  while ((module_name_size%8)!=0) module_name_size++;
  char *filename;
  filename=malloc(module_name_size+4);
  if (filename==NULL)
  {
      printf("Memory allocation error!\n");
      return 4;
  }
  // Reading functions
  sprintf(filename,"%s.map",module_name);
  FILE *fhndl=fopen(filename,"rb");
  if (fhndl==NULL)
  {
    printf("Can't open '%s' file!\n",filename);
    free(filename);
    return 1;
  }
  idx=0;
  while (!feof(fhndl))
  {
    exports[idx]=malloc(sizeof(struct export_entry));
    if (exports[idx]==NULL)
    {
      printf("Memory allocation error!\n");
      free(filename);
      return 4;
    }
    int nread=fscanf(fhndl," %hx:%lx %255s",&(exports[idx]->seg),&(exports[idx]->offs),exports[idx]->srcname);
    if ((nread<3)||(strlen(exports[idx]->srcname)<1))
    {
      if ((nread<=0) && feof(fhndl))
      {
        free(exports[idx]);
        exports[idx]=NULL;
        break;
      }
      printf("Error reading entry %d!\n",idx);
      printf("Fix the .MAP file and then retry.\n");
      free(filename);
      return 2;
    } else
    {
      get_name_with_prefix(exports[idx]->dstname,exports[idx]->srcname,funcname_prefix);
      exports[idx]->nmoffs=0;
      idx++;
      if (idx>=MAX_EXPORTS)
      {
        printf("Too many exports in .MAP file!\n");
        printf("Strip the file or increase MAX_EXPORTS to fix this.\n");
        free(filename);
        return 3;
      }
    }
  }
  fclose(fhndl);
  unsigned long num_of_exports=idx;
  printf("Got %d entries from .MAP file.\n",num_of_exports);
  //Sorting functions
  export_sort(exports,num_of_exports);
  printf("Entries are now sorted in memory.\n");
  // Checking
  int dupidx;
  dupidx=find_dupename(exports,num_of_exports);
  if (dupidx>=0)
  {
    printf("Duplicate entry name found!\n");
    printf("Entry \"%s\" duplicates. Aborting.\n",exports[dupidx]->dstname);
    printf("Remove duplicated entry from .MAP file to fix this.\n");
    free(filename);
    return 7;
  }
  dupidx=find_dupeoffs(exports,num_of_exports);
  if (dupidx>=0)
  {
    printf("Duplicate entry offset found!\n");
    printf("Offset 0x%08lX duplicates. Aborting.\n",exports[dupidx]->offs);
    printf("Remove duplicated entry from .MAP file to fix this.\n");
    free(filename);
    return 8;
  }
  //Saving the entries
  sprintf(filename,"%s.dll",module_name);
  fhndl=fopen(filename,"r+b");
  if (fhndl==NULL)
  {
    printf("Can't open '%s' file!\n",filename);
    printf("Check if it is in correct directory, and is not read-only.\n");
    free(filename);
    return 5;
  }
  // Reading headers
  unsigned long pe_header_raw_pos;
  unsigned long num_of_sections;
  unsigned long num_of_rva_and_sizes;
  unsigned long section_headers_raw_pos;
  unsigned long rvas_and_sizes_raw_pos;
  unsigned long filesize=file_length_opened(fhndl);
  // Locating PE header
  fseek(fhndl,MZ_NEWHEADER_OFS,SEEK_SET);
  pe_header_raw_pos=read_int32_le_file(fhndl);
  if ((pe_header_raw_pos < MZ_SIZEOF_HEADER) || 
  (pe_header_raw_pos+PE_SIZEOF_SIGNATURE+PE_SIZEOF_FILE_HEADER+PE_SIZEOF_OPTN_HEADER > filesize))
  {
      printf("The .DLL file has no valid 'new header'!\n");
      free(filename);
      return 6;
  }
  // Reading num. of sections
  fseek(fhndl,pe_header_raw_pos+PE_SIZEOF_SIGNATURE+PE_NUM_SECTIONS_OFS,SEEK_SET);
  num_of_sections=read_int16_le_file(fhndl);
  if (num_of_sections>=MAX_SECTIONS_NUM)
  {
      printf("The .DLL file has too many sections!\n");
      printf("Increaseing MAX_SECTIONS_NUM to above %d will fix this.\n",num_of_sections);
      free(filename);
      return 7;
  }
  // Reading num. of RVAs and sizes
  rvas_and_sizes_raw_pos = pe_header_raw_pos+PE_SIZEOF_SIGNATURE+PE_SIZEOF_FILE_HEADER;
  fseek(fhndl,rvas_and_sizes_raw_pos+PE_NUM_RVAS_AND_SIZES,SEEK_SET);
  num_of_rva_and_sizes=read_int32_le_file(fhndl);
  rvas_and_sizes_raw_pos += PE_SIZEOF_OPTN_HEADER;
  section_headers_raw_pos = rvas_and_sizes_raw_pos + num_of_rva_and_sizes*PE_SIZEOF_DATADIR_ENTRY;
  // Reading section headers
  for (idx=0;idx<=num_of_sections;idx++)
  {
      memset(sections_names[idx],0,MAX_SECTION_NAME_LEN+1);
      sections_vaddr[idx]=0x0f000000;
      sections_rsize[idx]=0;
      sections_raddr[idx]=0x0f000000;
  }
  for (idx=0;idx<num_of_sections;idx++)
  {
      unsigned long sect_pos = section_headers_raw_pos + idx*PE_SIZEOF_SECTHDR_ENTRY;
      fseek(fhndl,sect_pos,SEEK_SET);
      // Read section name
      fread (sections_names[idx+1], 1, MAX_SECTION_NAME_LEN, fhndl );
      sections_names[idx+1][MAX_SECTION_NAME_LEN]='\0';
      // Read addresses and sizes
      read_int32_le_file(fhndl);
      sections_vaddr[idx+1]=read_int32_le_file(fhndl);
      sections_rsize[idx+1]=read_int32_le_file(fhndl);
      sections_raddr[idx+1]=read_int32_le_file(fhndl);
  }
  int export_section_index=-1;
  for (idx=0;idx<=num_of_sections;idx++)
  {
      if (strcasecmp(sections_names[idx],export_section_name)==0)
      { export_section_index=idx; break; }
  }
  if (export_section_index<1)
  {
      printf("Cannot locate entry '%s' in section headers!\n",export_section_name);
      printf("Create the export section, or rename it to fix the problem.\n");
      free(filename);
      return 8;
  }
  printf("Export section '%s' located at RAW %08Xh.\n",export_section_name,sections_raddr[export_section_index]);
  printf("Section size is %d bytes (%08Xh).\n",sections_rsize[export_section_index],sections_rsize[export_section_index]);

  // Offsets relative to position of export section; these are computed using
  // the constants defined above
  const unsigned long adress_table_ofs   = EXPORT_DIRECTORY_SIZE;
  const unsigned long ordinal_table_ofs  = EXPORT_DIRECTORY_SIZE + (num_of_exports)*4;
  const unsigned long fnnames_table_ofs  = EXPORT_DIRECTORY_SIZE + (num_of_exports)*4 + (num_of_exports)*2;
  const unsigned long module_namestr_ofs = EXPORT_DIRECTORY_SIZE + (num_of_exports)*4 + (num_of_exports)*2 + (num_of_exports)*4;
  const unsigned long func_namestr_ofs   = EXPORT_DIRECTORY_SIZE + (num_of_exports)*4 + (num_of_exports)*2 + (num_of_exports)*4 + module_name_size;

  // Computing offsets inside the export section
  unsigned long address_table_raw_pos  = sections_raddr[export_section_index]+adress_table_ofs;
  unsigned long ordinal_table_raw_pos  = sections_raddr[export_section_index]+ordinal_table_ofs;
  unsigned long fnnames_table_raw_pos  = sections_raddr[export_section_index]+fnnames_table_ofs;
  unsigned long module_namestr_raw_pos = sections_raddr[export_section_index]+module_namestr_ofs;
  unsigned long func_namestr_raw_pos   = sections_raddr[export_section_index]+func_namestr_ofs;
  long          max_namestrs_size      = sections_rsize[export_section_index]-func_namestr_ofs;
  long          arr_raw_to_rva         = sections_vaddr[export_section_index]-sections_raddr[export_section_index];

  if (max_namestrs_size < MAX_EXPORT_NAMELEN)
  {
      printf("Cannot put %d entries in small export section!\n",MAX_EXPORT_NAMELEN);
      printf("Cut the .MAP file or make bigger section to fix this.\n");
      free(filename);
      return 9;
  }

  // Writing module name
  fseek(fhndl,module_namestr_raw_pos,SEEK_SET);
  for (idx=0;idx<strlen(module_name);idx++)
      fputc(module_name[idx],fhndl);
  for (;idx<module_name_size;idx++)
      fputc('\0',fhndl);
  // Writing function names
  fseek(fhndl,func_namestr_raw_pos,SEEK_SET);
  for (idx=0;idx<num_of_exports;idx++)
  {
      if (exports[idx]==NULL) break;
      unsigned long nmpos=ftell(fhndl);
      char *name=exports[idx]->dstname;
      exports[idx]->nmoffs=nmpos+arr_raw_to_rva;
      if (nmpos+strlen(name) >= func_namestr_raw_pos+max_namestrs_size)
      {
        printf("Function names space exceeded on func. %d!\n",idx);
        printf("Cut the .MAP file or make bigger section to fix this.\n");
        exports[idx]=NULL;
        break;
      }
      fputs(name,fhndl);
      fputc('\0',fhndl);
  }
  unsigned long end_of_used_space=ftell(fhndl);
  long remain_bts=func_namestr_raw_pos+max_namestrs_size-ftell(fhndl)-strlen(export_end_str);
  if (remain_bts>=0)
  {
    while (remain_bts>0)
    {
      fputc('\0',fhndl);
      remain_bts--;
    }
    fputs(export_end_str,fhndl);
  }
  printf("Written %d function export names into .DLL.\n",idx);
  // Updating section header
  fseek(fhndl,sections_raddr[export_section_index],SEEK_SET);
  {
    // export flags
        write_int32_le_file(fhndl,0);
    // export table creation date
        time_t dtime; time(&dtime);
        write_int32_le_file(fhndl,dtime);
    // export table version
        write_int32_le_file(fhndl,0);
    // module name address
        write_int32_le_file(fhndl,arr_raw_to_rva+module_namestr_raw_pos);
    // ordinal base
        write_int32_le_file(fhndl,1);
    // number of functions
        write_int32_le_file(fhndl,idx);
    // number of names
        write_int32_le_file(fhndl,idx);
    // address of functions
        write_int32_le_file(fhndl,arr_raw_to_rva+address_table_raw_pos);
    // address of names
        write_int32_le_file(fhndl,arr_raw_to_rva+fnnames_table_raw_pos);
    // address of ordinals
        write_int32_le_file(fhndl,arr_raw_to_rva+ordinal_table_raw_pos);
  }
  printf("Export section header updated.\n");
  fseek(fhndl,ordinal_table_raw_pos,SEEK_SET);
  for (idx=0;idx<num_of_exports;idx++)
  {
      if (exports[idx]==NULL)
      {
        write_int16_le_file(fhndl,0);
      } else
      {
        write_int16_le_file(fhndl,idx);
      }
  }
  printf("Written %d function export ordinals into .DLL.\n",idx);
  fseek(fhndl,address_table_raw_pos,SEEK_SET);
  for (idx=0;idx<num_of_exports;idx++)
  {
      if (exports[idx]==NULL)
      {
        write_int32_le_file(fhndl,0);
      } else
      {
        unsigned long val=exports[idx]->offs+sections_vaddr[(exports[idx]->seg)%MAX_SECTIONS_NUM];
        write_int32_le_file(fhndl,val);
      }
  }
  printf("Written %d function addresses into .DLL.\n",idx);
  fseek(fhndl,fnnames_table_raw_pos,SEEK_SET);
  for (idx=0;idx<num_of_exports;idx++)
  {
      if (exports[idx]==NULL)
      {
        write_int32_le_file(fhndl,0);
      } else
      {
        unsigned long val=exports[idx]->nmoffs;
        write_int32_le_file(fhndl,val);
      }
  }
  printf("Written %d function name offsets into .DLL.\n",idx);
  fclose(fhndl);
  long esection_used=end_of_used_space-sections_raddr[export_section_index];
  long esection_free=sections_rsize[export_section_index]-esection_used;
  printf("Used %d bytes in the export section; %d remain free.\n",esection_used,esection_free);
  sprintf(filename,"%s.def",module_name);
  fhndl=fopen(filename,"wb");
  if (fhndl==NULL)
  {
    printf("Can't open '%s' file!\n",filename);
    return 10;
  }
  fprintf(fhndl,"LIBRARY     %s.dll\n",module_name);
  fprintf(fhndl,"\nEXPORTS\n");
  for (idx=0;idx<num_of_exports;idx++)
  {
      if (exports[idx]==NULL)
        continue;
      char *name=exports[idx]->dstname;
      unsigned long val=exports[idx]->offs+sections_vaddr[(exports[idx]->seg)%MAX_SECTIONS_NUM];
      fprintf(fhndl,"    %-36s ; RVA=0x%08lX\n",name,val);
  }
  fclose(fhndl);
  printf("Written %d names into .DEF file.\n",idx);
  free(filename);
  return 0;
}
