#include <stdio.h>
#include <stdlib.h>
#include <time.h>	
#include <string.h>
#include "fat.h"
#define SECTOR_SIZE 512
#define CLUSTER_SIZE 1024;
#define VBR_OFFSET 0
#define FAT_OFFSET 1
#define RD_OFFSET 241
#define DATA_OFFSET 273


//function prototypes
void fillReservedSec(FILE*);
void fillRD();
void myCreate(FILE*);
void myWrite();
void myRead();

int findEmptySector(FILE*, int);
int findEmptyEntry(FILE*, int, int);
int findFreeDirFileEntry(FILE*, int, int);
void getDateTime(struct RD*);



unsigned short BPS; 
unsigned short totalSectors;
int currently_written_sector;

/*
	CALCULATION NOTES:
 	2MB = 2,097,152 bytes  = 4096 sectors   
 */

int main(){
	FILE* fp = fopen("Drive2MB", "r+");
	//fillReservedSec();
	//fillRD();
	myCreate(fp);
	fclose(fp);
	return 0;
}

/*
 * This function writes to the boot sector and sets the global variables.
 */
void fillReservedSec(FILE* fp){
	struct VBR* v = malloc(sizeof(struct VBR));
	v->BPS = SECTOR_SIZE;
	v->SPC = 4;
	v->RSC = 0;
	v->NFT = 1;
	v->MNRE = 1;
	v->totalSectors = 4096;
	v->ptrSize = 2;
	v->sysType = 12;
	fwrite(v, sizeof(struct VBR), 1, fp);
	printf("wrote to disk\n");
}

void fillRD(){

}

/*
 * Can create empty files and directories
 */
void myCreate(FILE* fp){
	printf("myCreate(): \n");
	//find free entry in root directory and return location
	int location = findFreeDirFileEntry(fp, RD_OFFSET, DATA_OFFSET-1);
	printf("Location of RD: %d bytes in\n", location);
	int fatLocation = findEmptyEntry(fp, FAT_OFFSET, RD_OFFSET-1);
	printf("Free fat location: %d\n", fatLocation);
	
			
	//TODO: update this in the case of subdirectories
	//TODO: consider whether or not for empty file/directory to even allocate space for it
	//find first empty data sector
	int sectorLocation = findEmptySector(fp, DATA_OFFSET-1);
	//update currently used sector in data region
	currently_written_sector = sectorLocation;
	printf("Free data sector @ %d %d\n", sectorLocation, currently_written_sector);
	
	//if entry space exists and aren't being used
	if(location != -1 && fatLocation != -1 && sectorLocation != -1){
		printf("Root directory is found + %d\n", location);
		//Create file/dir struct
		struct RD* r = malloc(sizeof(struct RD));
		strcpy(r->filename, "Testing");
		strcpy(r->ext, "txt");
		r->attr = 0;		//set as a file
		getDateTime(r);		//set datetime
		printf("date time %s\n",r->datetime);
		r->occupied=1;		//set as occupied 
		r->startCluster=sectorLocation;
		r->fileSize = 0;
		
		//write the file struct into the root directory 
		fseek(fp, location, SEEK_SET);
		fwrite(r, sizeof(struct RD), 1, fp);
		//write 0xFFFF into first free fat entry
		signed short endOfFile= 0xFFFF; // or -1
		fseek(fp, fatLocation, SEEK_SET);
		fwrite(&endOfFile, sizeof(signed short), 1, fp);
	} else if(location == -1){
		printf("Root Directory has no space or error. %d\n", location);
	} else if(fatLocation == -1){
		printf("No more space in FAT!!! %d\n", fatLocation);
	} else if(sectorLocation == -1){
		printf("Ran out of space in Data Section. Byte location %d\n", sectorLocation);
	} else {
		printf("Error\n");	
	}
}

/*
 * This function finds the empty sector in a data region. Used to write to data region. 
 */
int findEmptySector(FILE* fp, int offset){
	printf("findEmptySector() called\n");
	unsigned short temp;
	int i = 1;				//1 to compensate for boot sector
	int byteNumber;
	int byteSeek = -1;
	do {
		//start at first sector/cluster of offset
		int byteNumber = SECTOR_SIZE * offset + i * SECTOR_SIZE; //or TODO: i * CLUSTER_SIZE;
		fseek(fp, byteNumber, SEEK_SET);
		printf("ftell(): %ld\n", ftell(fp));
		printf("Byte Number: %d\n", byteNumber);
		//read in first two bytes, if not 0 move to next block.
		fread(&temp, sizeof(unsigned short),1, fp); 
		printf("temp value: %hu\n", temp);
		//if first val isn't 0, then move on to next sector/cluster
		if(temp == 0){
			byteSeek = byteNumber;
			printf("free space at sector %d\n", byteSeek);
		} else {
			printf("Sorry, not a free sector %d\n", byteSeek);
		}
		i++;
	} while(temp != 0 && byteSeek != -1);
	return byteSeek;
}

/*
 * This function finds the first empty byte entry in the region and returns the location.
 * The start and end offsets define the region boundaries.
 * start is defined as the start of one region, end is the start of the next region
 */
int findEmptyEntry(FILE* fp, int start, int end){
	printf("findEmptyEntry() called\n");
	unsigned short temp;
	int i;
	int byteLocation = -1;
	//scan through region until it hits an empty location
	for(i = start*SECTOR_SIZE; i < end*SECTOR_SIZE; i+=2){
		fseek(fp, i, SEEK_SET);
		printf("ftell(): %ld\n", ftell(fp));
		fread(&temp, sizeof(unsigned short), 1, fp);
		if(temp == 0){
			byteLocation = i;
			printf("read in entry: %hu\n", temp);
			printf("ftell(); %ld\n", ftell(fp));
			printf("bytelocation in findEmptyEntry(): %d\n", byteLocation);
			break;
		}
	}
	return byteLocation;
}

/*
 * This function finds an empty location in the root directory or directory structure by 
 * checking the occupied field in the virtual drive s.
 */
int findFreeDirFileEntry(FILE* fp, int s_offset, int e_offset){
	printf("findFreeDirFileEntry() called\n");
	int i;
	int entrySize = 32;			//each file or directory entry is 32 bytes big
	unsigned char bytes_read[entrySize];	//read in first 32 bytes into this array
	int byteLocation = -1;
	//scan for 'occupied' field and compare to 0
	for(i = s_offset*SECTOR_SIZE; i <= e_offset*SECTOR_SIZE; i+=32){
		printf("dir/file entry location in bytes = %d\n", i);
		fseek(fp, i, SEEK_SET);
		printf("ftell(); %ld\n", ftell(fp));
		fread(bytes_read, sizeof(unsigned char), entrySize, fp);	//read into array
		//if 'occupied' field bit  is 0, then it is free, else move on 
		if(bytes_read[25] == 0){
			printf("SUCCESS!!!\n");
			byteLocation = i;
			break;
		} else{
			printf("No. not here. Such fail. %c\n", bytes_read[25]);
		}
		
	}
	return byteLocation;
}

/*
 * This function puts in the datetime into the struct element in the form HHMMSSMMDDYY.
 * The order is time first followed by date.
 */
void getDateTime(struct RD* rd){
	time_t rawtime;
	struct tm *info;
	char buffer[13];
    time(&rawtime);
    info = localtime(&rawtime);
    strftime(buffer, 13, "%H%M%S%m%d%y", info);
    printf("Formatted date & time : |%s|\n", buffer ); 
	strcpy(rd->datetime, buffer);
}
	

