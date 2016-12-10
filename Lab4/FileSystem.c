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
#define BYTE_FAT 2

//function prototypes
void fillReservedSec(FILE*);
void myCreate(FILE*);
void writeToFat(FILE*, unsigned short*, int);
void writeToData(FILE*, char*, int);
void myWrite();
void myRead();
void mapFatData(FILE*, struct RD*, char[], int);
int countFileSectorSize(FILE*, struct RD*);
void parseCmd(char*, char*[]);
int countDelim(char*, char);
void copyDataToArr(char*,int, char[][SECTOR_SIZE], int);
int findEmptySector(FILE*, int, int);
int findEmptyEntry(FILE*, int, int);
int findFreeDirFileEntry(FILE*, int, int);
void getDateTime(struct RD*);
void findEntry(FILE*, int , int, char*[]);
void traverse(FILE*, char*[], int);

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
	
	//assume format is " ../.. where '/' can't be at end
	/*
	char input[] = "home/testing/file.txt";	
	int numDirFiles = countDelim(input, '/');	
	printf("%d\n", numDirFiles);	
	char* dir_files[numDirFiles];
	parseCmd(input, dir_files);
	printf("dir_files[0] = %s\n", dir_files[0]);
	
	*/
	
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

//TODO
void traverse(FILE* fp, char* dir_files[], int size){
	int i;
	int offset = RD_OFFSET;
	unsigned char metadata[32];
	for(i = 0; i < size; i++){
		//call findEntry() and copy over entry into metadata
		
		
	}
}

//TODO
void findEntry(FILE* fp, int s_offset, int e_offset, char* bytes_read[]){
	printf("findEntry() called\n");
	int i;
	int j;
	int byteLocation = -1;
	int entrySize = 32;			//each file or directory entry is 32 bytes big
	//unsigned char bytes_read[entrySize];	//read in first 32 bytes into this array
	char* arr[entrySize];
	int nameLength = 8;
	char* fileName[nameLength];
	//scan for 'occupied' field and compare to 0
	for(i = s_offset*SECTOR_SIZE; i <= e_offset*SECTOR_SIZE; i+=32){
		printf("dir/file entry location in bytes = %d\n", i);
		//scan every 32 bytes
		fseek(fp, i, SEEK_SET);
		printf("ftell(); %ld\n", ftell(fp));
		fread(bytes_read, sizeof(unsigned char), entrySize, fp);	//read into array
		
		//copy over /concatenate  string of first 8 characters
		for(j = 0; j < nameLength; j++){
			fileName[j] = bytes_read[j];
		}
		//if 'occupied' field bit  is 1, then it is free, else move on 
		if(*bytes_read[25] == '1'){
			printf("SUCCESS!!!\n");
			byteLocation = i;
			break;
		//} else if(bytes_read[25] == 1){
		//	printf("directory ftell() %c %ld\n", bytes_read[25], ftell(fp));
		} else{
			//printf("No. not here. Such fail. %c\n", bytes_read[25]);
		}
		
	}
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
		r->fileSize = 48; //0; //1322;
		
		//if it is a directory type 
		//point to an empty sector as usual in startcluster
		//write directory field there
		
		//test input data from text file
		char* dataTest = "HELLO BLA BLAH BLAH TESTING OKAY WHATEVER PEACE.";
		//dataTest = "";
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
	printf("-> currentFatLocation: %d\n", fatLocation); 
	int fatL = fatLocation;
	char blockData[sectorCount][SECTOR_SIZE]; 	//number of blocks each of 512 char big
	//copy data to blockData array
	copyDataToArr(data, sectorCount, blockData, r->fileSize);
	unsigned short fatVal = 0;
	int dataL = -1;
	printf("\t Current fat location: %d\n", fatL);
	
	//if creating an empty file, reserve one data block 
	if(sectorCount == 0){
		fatVal = 0xFFFF;
		char dummy = '*';
		dataL = findEmptySector(fp, DATA_OFFSET*SECTOR_SIZE, (END-1)*SECTOR_SIZE);
		printf("sector size is 0, data sector is %d\n", dataL/SECTOR_SIZE);
		if(dataL != -1){
			//find empty entry in fat and write
			fseek(fp, currentFatLocation, SEEK_SET);
			fwrite(&fatVal,  sizeof(unsigned short), 1, fp);
			//find empty data region and reserve it with a dummy character
			fseek(fp, dataL, SEEK_SET);
			fwrite(&dummy, sizeof(char), 1, fp);
			printf("wrote empty file to fat\n");
		}
	} else {
		//loop around the sectors and write
		for(i = 0; i < sectorCount; i++){
			//find the next empty entry in fat table
			fatL = findEmptyEntry(fp, fatL+BYTE_FAT, (RD_OFFSET-1)*SECTOR_SIZE);
			printf("\t --Free fat entry %d\n", fatL);
			//find empty sector in data region to map to (with 1-to-1 fat to data mapping)
			int offsetD = (currentFatLocation-SECTOR_SIZE)/2;
			printf("---------> offset Data : %d\n", offsetD);
			dataL = findEmptySector(fp, ((DATA_OFFSET+offsetD)*SECTOR_SIZE), (END-1)*SECTOR_SIZE);
			printf("\t --Free data block %d. Sector: %d\n", dataL, dataL/SECTOR_SIZE);
			
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
				printf("\tfat value to write: %hu\n", fatVal);
				printf("\tcurrent fat value location: %d\n", currentFatLocation);
				//TODO: refactor these method calls and delete the lines afterwards
				//writeToFat(fp, &fatVal, fatL);
				//writeToData(fp, blockData[i], dataL);
				//writeToFat(fp, &fatVal, fatL);
				fseek(fp, currentFatLocation, SEEK_SET);
				fwrite(&fatVal,  sizeof(unsigned short), 1, fp);
				printf("\tfwrite fatVal to sector %d\n", currentFatLocation/SECTOR_SIZE);
				//writeToData(fp, blockData[i], dataL);
				fseek(fp, dataL, SEEK_SET);
				fwrite(blockData[i], sizeof(char), sizeof(blockData[i]), fp);
				//printf("%s\n", blockData[i]);
				printf("\tfwrite blockdata to sector %d\n", dataL/SECTOR_SIZE);
			}
			//update current fat location to next 
			currentFatLocation = fatL;
		}
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
			//printf("\tblock[%d][%d] = %c \n", i, j, block[i][j]);
			start++;
		}
		//write 0s everywhere else because of random character on fwrite
		for(k = start; k < SECTOR_SIZE; k++){
			block[i][k] = 0; 
			//printf("\tblock[%d][%d] = %c \n", i, k, block[i][k]);
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
	//have a method that counts number of '/'
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
 * index of the array stores an argument which is separated by slashes in the string         *
 * Preconditions:                                                                            *
 * @params input - string command typed by user                                              *
 * @params arr - array of strings that will store parsed command                             * 
 * Postconditions:                                                                           *
 * @return void                                                                              *
 *********************************************************************************************/
void parseCmd(char* input, char* arr[]){
   char* token;
   const char delimiter[2] = "/";
   int i = 0;
   //Get the first token turning the first slash to a Null character
   token = strtok(input, delimiter);
   arr[i] = token;
   i++;
   //Loop through all tokens with a slash and aren't null
   while(token != NULL){
   	   token = strtok(NULL, delimiter);	
   	   arr[i] = token;
   	   i++;	
   }
   printf("\n");
}


/*********************************************************************************************
 * This function counts the numbers of delimiters in the user entered command. For the scope *
 * of this program slashes are counted.                                                      *
 *                                                                                           *
 * Preconditions:                                                                            *
 * @params input - input string command                                                      *
 * @params delim - character of delimiter to look for                                        *
 * Postconditions:                                                                           *
 * @return number of spaces (arguments) in the string                                        *
 *********************************************************************************************/
int countDelim(char* input, char delim){
    int i;
    int count = 0;
	for(i = 0; input[i] != '\0'; i++)
		if(input[i] == delim)
			count++;
	return count + 1;
}



