/*   
    IOS ELF stripper, converts traditional ELF files into the format IOS wants.
    Copyright (C) 2008 neimod.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>

#define ELF_NIDENT 16

typedef struct 
{
        unsigned long		ident0;
	unsigned long		ident1;
	unsigned long		ident2;
	unsigned long		ident3;
	unsigned long		machinetype;
        unsigned long		version;
        unsigned long		entry;
        unsigned long       phoff;
        unsigned long       shoff;
        unsigned long		flags;
        unsigned short      ehsize;
        unsigned short      phentsize;
        unsigned short      phnum;
        unsigned short      shentsize;
        unsigned short      shnum;
        unsigned short      shtrndx;
} elfheader;

typedef struct
{
  unsigned long  shname;                /* Section name (string tbl index) */
  unsigned long  shtype;                /* Section type */
  unsigned long  shflags;               /* Section flags */
  unsigned long  shaddr;                /* Section virtual addr at execution */
  unsigned long  shoffset;              /* Section file offset */
  unsigned long  shsize;                /* Section size in bytes */
  unsigned long  shlink;                /* Link to another section */
  unsigned long  shinfo;                /* Additional section information */
  unsigned long  shaddralign;           /* Section alignment */
  unsigned long  shentsize;             /* Entry size if section holds table */
} elfshentry;

typedef struct
{
	unsigned long offset;
	unsigned long size;
} offset_size_pair;

unsigned short getbe16(void* pvoid)
{
	unsigned char* p = (unsigned char*)pvoid;

	return (p[0] << 8) | (p[1] << 0);
}

unsigned long getbe32(void* pvoid)
{
	unsigned char* p = (unsigned char*)pvoid;

	return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | (p[3] << 0);
}

int main(int argc, char* argv[])
{
	int result = 0;

	fprintf(stdout, "stripiosplugin - based on IOS ELF stripper - by neimod\n");
	if (argc < 3 )
	{
		fprintf(stderr,"Usage: %s <in.elf> <out.bin> [strip addr]\n", argv[0]);

		return -1;
	}

	FILE* fin = fopen(argv[1], "rb");
	FILE* fout = fopen(argv[2], "wb");

	if (fin == 0 || fout == 0)
	{
		if (fin == 0)
			fprintf(stderr,"ERROR opening file %s\n", argv[1]);
		if (fout == 0)
			fprintf(stderr,"ERROR opening file %s\n", argv[2]);
		return 1;
	}

	elfheader header;

	if (fread(&header, sizeof(elfheader), 1, fin) != 1)
	{
		fprintf(stderr,"ERROR reading ELF header\n");
		return 1;
	}

	unsigned long elfmagicword = getbe32(&header.ident0);

	if (elfmagicword != 0x7F454C46)
	{
		fprintf(stderr,"ERROR not a valid ELF\n");
		return 1;
	}

	unsigned long shoff = getbe32(&header.shoff);
	unsigned short shentsize = getbe16(&header.shentsize);
	unsigned short shnum = getbe16(&header.shnum);
	unsigned long memsz = 0, filesz = 0;
	unsigned long vaddr = 0, paddr = 0;

	//printf("shoff %lx\n", shoff);
	//printf("size %x\n", shentsize);
	//printf("entries %x\n", shnum);

	elfshentry* entry = new elfshentry[shnum];

	fseek(fin, shoff, SEEK_SET);
	if (fread(entry, sizeof(elfshentry), shnum, fin) != shnum)
	{
		fprintf(stderr,"ERROR reading sections header\n");
		return 1;
	}

	unsigned short i = 0;
	while (i < shnum)
	{
  		unsigned long addr = getbe32(&entry[i].shaddr);
  		unsigned long offset = getbe32(&entry[i].shoffset);
		unsigned long size = getbe32(&entry[i].shsize);
		if ((addr & ~0xfff) == 0x1377E000) {
  			printf("addr:%lx - ",addr);
			printf("offset:%lx - ",offset);  
			printf("size: %lx\n", size);
		fseek(fin, offset, SEEK_SET);
		char *buf = new char[size];
		if (!buf) {
			fprintf(stderr, "Error allocating memory\n");
			return 2;
		}
		fread(buf, sizeof(char), size, fin);
		if (fwrite(buf, sizeof(char), size, fout) != size) {
			fprintf(stderr, "Error writing output file\n");
			return 2;
		}
		delete [] buf;
		}
		i++;
	}

	if (entry)
		delete[] entry;
	if (fout)
		fclose(fout);

	if (fin)
		fclose(fin);

	return result;
}

