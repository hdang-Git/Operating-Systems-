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
void fillReservedSec();
void fillRD();
void myCreate();
void myWrite();
void myRead();

int findEmptySector(FILE*, int);
int findEmptyEntry(FILE*, int, int);
void getDateTime(struct RD*);


typedef unsigned bits;
unsigned char byte; 		//uint8_t 
unsigned short twoBytes;  	//uint16_t
FILE* fp;
unsigned short BPS; 
unsigned short totalSectors;


/*
	CALCULATION NOTES:
 	2MB = 2,097,152 bytes  = 4096 sectors   
 */

int main(){
	fp = fopen("Drive2MB", "r+");
	//fillReservedSec();
	//fillRD();
	myCreate();
	return 0;
}

/*
 * This function writes to the boot sector and sets the global variables.
 */
void fillReservedSec(){
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

//adding a file to root directory
void myCreate(){
	FILE* fp = fopen("Drive2MB", "r+");
	unsigned short temp;
	//add to root directory
	fseek(fp, SECTOR_SIZE * RD_OFFSET, SEEK_SET);
	fscanf(fp, "%hu", &temp);	//TODO: change to fread()
	printf("temp: %hd\n", temp); 
	printf("ftell(): %ld\n", ftell(fp));
	//if nothing exists (first write) or check that no sectors are used
	if(temp == 0){
		//struct RD* r = malloc(sizeof(struct RD));
		printf("Root directory is found + %d\n", temp);
		//Create file/dir struct
		struct RD* r = malloc(sizeof(struct RD));
		strcpy(r->filename, "Testing");
		strcpy(r->ext, "txt");
		r->attr = 0;
		getDateTime(r);		
		printf("date time %s\n",r->datetime);
		/*
		r->reserved1 = 1;
		r->createdTime = 0;
		r->createdDate = 0;
		r->accessDate = 0;
		r->reserved2 = 0;
		r->modifiedTime = 0;
		r->modifiedDate = 0;
		*/
		r->occupied=1;

		//find first empty fat sector
		findEmptySector(fp, FAT_OFFSET);
			
		//call a method to return offset
		r->startCluster=FAT_OFFSET;

		r->fileSize = 532;
		//if file size is greater than cluster size (2 sectors)
		if(r->fileSize > 1024){
			//loop and decrement by 1024
		}
		//write the file struct into the root directory 
		
	} else {
		printf("Root Directory isn't empty or error. %d\n", temp);
	} 
	
	fclose(fp);
	//else scan through reserved1 sections
	/*
	int i = 0;
	//512 bytes 32 bytes per file/directory -> 16 entries * 32 records = 512 total records
	
	for(i = 0; i < 512; i *= 32){
		fseek(fp, SECTOR_SIZE*RD_OFFSET + i, SEEK_SET); //change to bps
	}
	*/
	
	
	//if greater than cluster size add to fat else end with -1 for 2 bytes
	//add offset and go to data section, write data
}

/*
 * This function finds the empty sector in a data region
 */
int findEmptySector(FILE* fp, int offset){
	unsigned short temp;
	int i = 1;				//@ 1 to compensate for boot sector
	int byteNumber;
	int byteSeek = -1;
	do {
		//start at first fat cluster;
		int byteNumber = SECTOR_SIZE * offset + i * SECTOR_SIZE; //or i * CLUSTER_SIZE;
		fseek(fp, byteNumber, SEEK_SET);
		printf("ftell(): %ld\n", ftell(fp));
		printf("Byte Number: %d\n", byteNumber);
		//read in first two bytes, if not 0 move to next block.
		fread(&temp, sizeof(unsigned short),1, fp); 
		printf("temp value: %hu\n", temp);
		//if first pointer val isn't 0, then move on to next cluster sector
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
int findEmptyEntry(File* fp, int start, int end){
	unsigned short temp;
	int i;
	int byteLocation = -1;
	//scan through region until it hits an empty location
	for(i = start*SECTOR_SIZE; i < end*SECTOR_SIZE; i++){
		byteLocation = SECTOR_SIZE * start + i*2;	//search through every two bytes
		fseek(fp, byteLocation, SEEK_SET);
		//printf("ftell(): %ld\n", ftell(fp));
		fread(&temp, sizeof(unsigned short), 1, fp);
		if(temp == 0){
			printf("read in entry: %hu\n", temp);
			printf("ftell(); %ld\n", ftell(fp));
			break;
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
	

