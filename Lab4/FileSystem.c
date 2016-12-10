#include <stdio.h>
#include <stdlib.h>
#include <time.h>	
#include <string.h>
#include "fat.h"
#define SECTOR_SIZE 512
#define CLUSTER_SIZE 1024;
#define VBR_OFFSET 0
#define FAT_OFFSET 1
#define RD_OFFSET 121
#define DataRD_OFFSET 153
#define DATA_OFFSET 665
#define END 4096


//function prototypes
void fillReservedSec(FILE*);
void myCreate(FILE*);
void writeToFat(FILE*, unsigned short*, int);
void writeToData(FILE*, char*, int);
void myWrite();
void myRead();

void mapFatData(FILE*, struct RD*, char[], int);
int countFileSectorSize(FILE*, struct RD*);
void parseDir(char*, char*[]);
void copyDataToArr(char*,int, char[][SECTOR_SIZE], int);
int findEmptySector(FILE*, int, int);
int findEmptyEntry(FILE*, int, int);
int findFreeDirFileEntry(FILE*, int, int);
void getDateTime(struct RD*);


unsigned short BPS; 
unsigned short totalSectors;
int currently_written_sector;

/*
	CALCULATION NOTES:
 	2MB = 2,097,152 bytes  = 4096 sectors   
 	max number of blocks for data for fat alone: 3431 or 0xD67
 */

int main(){
	FILE* fp = fopen("Drive2MB", "r+");
	fillReservedSec(fp);
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

/*
 * Can create empty files and directories
 */
void myCreate(FILE* fp){
	printf("myCreate(): \n");
	//find free entry in root directory and return location
	int location = findFreeDirFileEntry(fp, RD_OFFSET, DATA_OFFSET-1);
	printf("Location of RD: %d bytes in\n", location);
	//find first empty data entry to write to fat
	int fatLocation = findEmptyEntry(fp, FAT_OFFSET*SECTOR_SIZE, (RD_OFFSET-1)*SECTOR_SIZE);
	printf("Free fat location: %d\n", fatLocation);
	
	//TODO: update this in the case of subdirectories
	
	//if entry space exists and aren't being used
	if(location != -1 && fatLocation != -1){
		//printf("Root directory is found + %d\n", location);
		//Create file/dir struct
		struct RD* r = malloc(sizeof(struct RD));
		strcpy(r->filename, "Testing");
		strcpy(r->ext, "txt");
		r->attr = 0;		//set as a file
		getDateTime(r);		//set datetime
		//printf("date time %s\n",r->datetime);
		r->occupied=1;		//set as occupied 
		r->startCluster = fatLocation;	//store fat byte address entry 
		r->fileSize = 48;//1322;
		
		//if it is a directory type 
		//point to an empty sector as usual in startcluster
		//write directory field there
		
		//test input data from text file
		char* dataTest = "HELLO BLA BLAH BLAH TESTING OKAY WHATEVER PEACE.";
		char data[r->fileSize];
		strcpy(data, dataTest);
		
		//write the file struct into the root directory 
		fseek(fp, location, SEEK_SET);
		fwrite(r, sizeof(struct RD), 1, fp);
		
		//map to the fat and data sections
		mapFatData(fp, r, data, fatLocation);
		
	} else if(location == -1){
		printf("Root Directory has no space or error. %d\n", location);
	} else if(fatLocation == 0xD67){	//specific for shorts
		printf("No more space in FAT!!! %d\n", fatLocation);
	} else {
		printf("Error\n");	
	}
}


/*
 * This function counts number of sectors to write to.
 */
int countFileSectorSize(FILE* fp, struct RD* entry){
	int size = entry->fileSize;
	int count = 0; //default assume 0 sectors
	while(size > 0){
		printf("size of file: %d\n", size);
		count++;
		size -= SECTOR_SIZE;
	}
	printf("Sector count: %d\n", count);
	return count;
}

/*
 * This function writes to the fat and the data section, creating a mapping between the two.
 * data input is  segments at a time;
 */
void mapFatData(FILE* fp, struct RD* r, char data[], int fatLocation){
	//get number of sectors to write data
	int sectorCount = countFileSectorSize(fp, r);
	printf("\tsectorCount %d\n", sectorCount);
	int i;
	//current empty fat entry
	int currentFatLocation = fatLocation; 
	int fatL;
	char blockData[sectorCount][SECTOR_SIZE]; 	//number of blocks each of 512 char big
	//copy data to blockData array
	copyDataToArr(data, sectorCount, blockData, r->fileSize);
	unsigned short fatVal = 0;
	int dataL = -1;
	printf("\t Current fat location: %d\n", fatL);
	
	//loop around the sectors and write
	for(i = 0; i < sectorCount; i++){
		//find the next empty entry in fat table
		fatL = findEmptyEntry(fp, fatL+2, (RD_OFFSET-1)*SECTOR_SIZE);
		printf("\t Free fat entry %d. %d\n", i, fatL);
		//find empty sector in data region to map to
		dataL = findEmptySector(fp, DATA_OFFSET*SECTOR_SIZE, (END-1)*SECTOR_SIZE);
		printf("\t Free data block %d.\n", dataL);
		//error check
		if(dataL != -1 && fatL != -1){
			//if last sector to write to, add terminating 0xFFFF to note end of file
			if(i == sectorCount - 1){
				fatVal = 0xFFFF; 
			} else {
				//get the fat value of the next empty fat to write into the current entry
				fatVal = fatL-SECTOR_SIZE;
			}
			//write to fat and write data
			printf("fat value to write: %hu\n", fatVal);
			//writeToFat(fp, &fatVal, fatL);
			//writeToData(fp, blockData[i], dataL);
			//writeToFat(fp, &fatVal, fatL);
			fseek(fp, currentFatLocation, SEEK_SET);
			fwrite(&fatVal,  sizeof(unsigned short), 1, fp);
			printf("fwrite fatVal to sector %d\n", currentFatLocation/SECTOR_SIZE);
			//writeToData(fp, blockData[i], dataL);
			fseek(fp, dataL, SEEK_SET);
			fwrite(blockData[i], sizeof(char), sizeof(blockData[i]), fp);
			printf("%s\n", blockData[i]);
			printf("fwrite blockdata to sector %d\n", dataL/SECTOR_SIZE);
		}
		//update current fat location to next 
		currentFatLocation = fatL;
	}
}


/*
 * This function copies data from one array to the the array blocks where length is the 
 * length of data.
 */
void copyDataToArr(char data[], int sectorCount, char block[][SECTOR_SIZE], int length)
{
	printf("copyDataToArr() called.\n");
	int i;
	int j;
	int k;
	int start = 0;
	int end = length;
	for(i = 0; i < sectorCount; i++){
		printf("ITERATION %d:\n", i); 
		//write the characters
		for(j = 0;  data[start] != '\0' && end > 0 && j < length; j++){
			block[i][j] = data[start]; 
			printf("\tblock[%d][%d] = %c \n", i, j, block[i][j]);
			start++;
		}
		//write 0s everywhere else because of random character on fwrite
		for(k = start; k < SECTOR_SIZE; k++){
			block[i][k] = 0; 
			printf("\tblock[%d][%d] = %c \n", i, k, block[i][k]);
			start++;
		}
		end -= SECTOR_SIZE;
		printf("----end size: %d\n", end);
	}
}

/*
 * This function writes to the Fat the next fat entry as a unsigned short 
 * in the current fat position
 */
void writeToFat(FILE* fp, unsigned short* data, int location){
	printf("writeToFat() called to location %d, sector %d\n", location, location/SECTOR_SIZE);
	fseek(fp, location, SEEK_SET);
	fwrite(&data, sizeof(unsigned short), 1, fp);
}

/*
 * This function writes to the data section an array of char of size 512.
 */
void writeToData(FILE* fp, char* data, int location){
	printf("writeToData() called to location %d, sector %d\n", location, location/SECTOR_SIZE);
	fseek(fp, location, SEEK_SET);
	fwrite(&data, strlen(data), 1, fp);
}


void myread(){
	//count number '/'
	//create array of with size = # of '/' + 1
	//if has '/' in it
		//call method to token array
		////parse directories and filename into an array (tokenize it)
		//strcpy into a local array knowing length
		//also make sure each element is a char array
		//Loop search through directories (attr == 1) by comparing names
			//if name matches, and not last element of array 
				//go to next directory 
			//if at end of array
				//look for file
			//else error message for unknown directory name
	////////////////////////////////////////////////////////////////////////////
	//Simple read
	//locate file name
	//loop read in root directory entries
		//compare file name to file names in root directory
		//if success, 
			
		//else print error message
	
}

/*
 * This function finds the empty sector in a data region. Used to write to data region. 
 * Returns the sector in bytes so must divide by SECTOR_NUMBER to get sector number. 
 */
int findEmptySector(FILE* fp, int start, int end){
	printf("findEmptySector() called\n");
	unsigned short temp = -1;
	int i;
	int byteNumber;
	int byteSeek = -1;
	for(i = start; i < end && temp != 0; i+=SECTOR_SIZE){
		//start at first sector/cluster of offset
		fseek(fp, i, SEEK_SET);
		//printf("\tftell(): %ld\n", ftell(fp));
		//printf("\tByte Number: %d\n", byteNumber);
		//read in first two bytes
		fread(&temp, sizeof(unsigned short),1, fp); 
		//printf("\ttemp value: %hu\n", temp);
		//if first val isn't 0, then move on to next sector/cluster
		if(temp == 0){
			byteSeek = i;
			//printf("\tfree space at sector %d\n", byteSeek/SECTOR_SIZE);
		} else {		//TODO: delete else statement
			//printf("\tSorry, not a free sector %d\n", byteSeek);
		}
	} 
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
	for(i = start; i < end; i+=2){
		fseek(fp, i, SEEK_SET);
		//printf("ftell(): %ld\n", ftell(fp));
		fread(&temp, sizeof(unsigned short), 1, fp);
		if(temp == 0){
			byteLocation = i;
			//printf("read in entry: %hu\n", temp);
			//printf("ftell(); %ld\n", ftell(fp));
			//printf("bytelocation in findEmptyEntry(): %d\n", byteLocation);
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
		//printf("dir/file entry location in bytes = %d\n", i);
		fseek(fp, i, SEEK_SET);
		//printf("ftell(); %ld\n", ftell(fp));
		fread(bytes_read, sizeof(unsigned char), entrySize, fp);	//read into array
		//if 'occupied' field bit  is 0, then it is free, else move on 
		if(bytes_read[25] == 0){
			printf("SUCCESS!!!\n");
			byteLocation = i;
			break;
		//} else if(bytes_read[25] == 1){
		//	printf("directory ftell() %c %ld\n", bytes_read[25], ftell(fp));
		} else{
			//printf("No. not here. Such fail. %c\n", bytes_read[25]);
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
    //printf("Formatted date & time : |%s|\n", buffer ); 
	strcpy(rd->datetime, buffer);
}
	
/*********************************************************************************************
 * This function parses the passed in command string and stores it into an array. Each       *
 * index of the array stores an argument which is separated by spaces in the string          *
 * Preconditions:                                                                            *
 * @params input - string command typed by user                                              *
 * @params arr - array of strings that will store parsed command                             * 
 * Postconditions:                                                                           *
 * @return void                                                                              *
 *********************************************************************************************/
void parseDir(char* input, char* arr[]){
   char* token;
   const char delimiter[2] = "/";
   int i = 0;
   //Get the first token turning the first space to a Null character
   token = strtok(input, delimiter);
   arr[i] = token;
   i++;
   //Loop through all tokens with a space and aren't null
   while(token != NULL){
   	   token = strtok(NULL, delimiter);	
   	   arr[i] = token;
   	   i++;	
   }
   printf("\n");
}

