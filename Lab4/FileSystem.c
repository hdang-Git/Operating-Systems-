#include <stdio.h>
#include <stdlib.h>
#include <time.h>	
#include <string.h>
#include <ctype.h>
#include <libgen.h>
#include <errno.h>
#include "fat.h"

#define SECTOR_SIZE 512
#define CLUSTER_SIZE 1024;
#define VBR_OFFSET 0
#define FAT_OFFSET 1
#define RD_OFFSET 121
#define DATA_OFFSET 153
#define END 4096
#define BYTE_FAT 2

//function prototypes
void fillReservedSec(FILE*);
void myCreate(FILE*, char*, char*, char, int, char*);
void writeToFat(FILE*, unsigned short*, int);
void writeToData(FILE*, char*, int);
void myWrite(FILE*, char*);
void myRead(FILE*, char*);
void mapFatData(FILE*, struct RD*, char[], int);
int countFileSectorSize(FILE*, struct RD*);
void parseCmd(char*, char*[], char*);
int countDelim(char*, char);
int verifyInput(char*);
void copyDataToArr(char*,int, char[][SECTOR_SIZE], int);
int findEmptySector(FILE*, int, int);
int findEmptyEntry(FILE*, int, int);
int findFreeDirFileEntry(FILE*, int, int);
void getDateTime(struct RD*);
char* getFileName(char*, char*);
void append(FILE*, char*[]);
void traverse(FILE*, char*[], int);
int findEntry(FILE*, int , int, char*, char*);
char* trim(char*, char);
int countCharNum(FILE*);
void readData(FILE*, char[], int);
void copyArrays(char*, char*, int);
void printPrompt();
void chooseOption(FILE*, int, char*);
int check(char*[], int, char*[]);
void createEmptyFile(FILE*, char*);
void createEmptyDir(FILE*, char*);


unsigned short BPS; 
unsigned short totalSectors;
int currently_written_sector;



/*********************************************************************************************
 * This function calls on the operations needed to read/write from the virtual dre along     *
 * with related string parsing.                                                              *
 *                                                                                           *
 *	CALCULATION NOTES:                                                                       *
 *	2MB = 2,097,152 bytes  = 4096 sectors                                                    *
 *	max number of blocks for data for fat alone: 3431 or 0xD67                               *
 * 			//assume format is " ../.. where '/' can't be at end							 *
 *********************************************************************************************/
int main(){
	
	FILE* fp = fopen("Drive2MB", "r+");
	fillReservedSec(fp);
	//myWrite(fp, "test1.txt");
	//createEmptyFile(fp, "Hello.txt");
	//createEmptyDir(fp, "Documents");
	myRead(fp, "test1.txt");
	/*
	//myCreate(fp);
    char* inputCmd = NULL;							//store user command
    int bytesRead; 									//# of bytes read from getline() func
    size_t inSize = 100;							//size of input from getLine() func
   	int i;
   	char* fs_cmd[] = {"create", "createDir", "write", "read", "\0"}; 
   	
   	while(1){ 
   		printPrompt();
   		bytesRead = getline(&inputCmd,&inSize,stdin); //get user input and the error code
   		if(bytesRead == -1){	//if Error reading, send a error msg to user
          printf("Error! %d\n", bytesRead);	
      	} else if(verifyInput(inputCmd)){				//Do nothing
      	} else {

   			inputCmd = trim(inputCmd, ' ');			//get rid of leading & trailing spaces
        	int size = countDelim(inputCmd, ' ');	//count the # of args delimited by spaces
        	char* cmd[size];						//create array with size of # of args
        	parseCmd(inputCmd,cmd, " ");			//parse input into each index of array cmd

			int num = check(cmd, size, fs_cmd); 
			printf("num: %d\n", num);
        	chooseOption(fp, num+1, cmd[1]);
   		}
    }
    */
	fclose(fp);
	return 0;
}

void printPrompt(){
	printf("\n\n\n\n");
	printf("**************************************************************************\n");
	printf("Please type in file system commands for those listed below ");
	printf("\nin the following format:\n");
	printf("- create <file name>\n");
	printf("- createDir <directory name>\n");
	printf("- write <externalfile.txt> - writes external file into the file system\n");
	printf("- read  <path>|<filename> - reads from disk the data of that specific file\n");
	printf("CTRL-C to exit\n");
	printf("**************************************************************************\n\n");
}


/*********************************************************************************************
 * This function chooses what to function to perform on the file system                      *
 *********************************************************************************************/
void chooseOption(FILE* fp, int choice, char* fileName){
	switch(choice){
		case 1 	:	createEmptyFile(fp, fileName);
					break;
		case 2	:	createEmptyDir(fp, fileName);
					break;
		case 3  :	myWrite(fp, fileName);
					break;
		case 4	:	myRead(fp, fileName);
		default :	printf("Command DNE\n");
					break; 
		
	}
}


/*********************************************************************************************
 * This function checks if the passed string is a valid file system command or not.          *
 *                                                                                           *
 * Preconditions:                                                                            *
 * @params input[] - stores command in first index with optional command options in rest     * 
 * @params size - size of the input[] array                                                  *
 * @params fs_cmd[] - list of valid file system commands                                     *
 * Postconditions:                                                                           *
 * @return valid flag that tells if it is a file system command or not                       *
 *********************************************************************************************/
int check(char* input[], int size, char* fs_cmd[]){
    int i;
    int index = -1;
    //Loop through commands and compare against user input
    for(i = 0; strcmp(fs_cmd[i],"\0") != 0; i++){
    	//if the first user argument is in the list of builtins
    	if(strcmp(input[0], fs_cmd[i]) == 0){
    		index = i;									//Retrieve index in built_in array 
    	}
    } 
   return index;
}

/*********************************************************************************************
 * This function writes extenral data to the file system.                                    *
 *********************************************************************************************/
void myWrite(FILE* fp, char* name){
	printf("Opening %s\n", name);
	FILE* inputFile = fopen(name, "r+");
	if(inputFile != NULL){
		int fileSize = countCharNum(inputFile);
		printf("fileSize: %d\n", fileSize);
		char fileData[fileSize];
		readData(inputFile, fileData, fileSize);
		
		//get file name
		char* bname = getFileName(name, bname);
		printf("basename: %s\n", bname);
		//store separated file name and extension 
		char* fileName[2];
		//populate array and remove .
		parseCmd(bname, fileName, ".");
		//call myCreate() to write to file all data
		myCreate(fp, fileName[0], fileName[1],'1', fileSize, fileData);
	} else {
		perror("Error opening file");
	}
		
}

/*********************************************************************************************
 * This function reads data from the file system and displays to screen.                     *
 *********************************************************************************************/
void myRead(FILE* fp, char* input){
	printf("myRead() called\n");
	//char input[] = "Testing.txt";	

	int i;
	//make a copy of input before string manipulations
	char* input2 = (char *) malloc(strlen(input)+1);
	strcpy(input2, input);
	
	//get file name
	char* bname = getFileName(input, bname);
	printf("basename: %s\n", bname);
	//store separated file name and extension 
	char* fileName[2];
	//populate array and remove .
	parseCmd(bname, fileName, ".");
	printf("fileName[0] %s\n", fileName[0]);
	printf("file path: %s\n", input);
	
	//get number of directory/files
	int numDirFiles = countDelim(input2, '/');	
	printf("%d\n", numDirFiles);	
	//create directoryfiles array separating the names 
	char* dir_files[numDirFiles];
	//populate array and remove slashes
	parseCmd(input2, dir_files, "/");
	printf("dir_files[0] = %s\n", dir_files[0]);

	//replace with file extension with filename
	dir_files[numDirFiles-1] = fileName[0];
	for(i = 0; i < numDirFiles; i++){
		printf("%s\n", dir_files[i]);
	}
	//read in data
	traverse(fp, dir_files, numDirFiles);
}

/*********************************************************************************************
 * This function creates an empty file in the file system.                                   *
 *********************************************************************************************/
void createEmptyFile(FILE* fp, char* name){
	printf("createEmptyFile() called\n");
	//get file name
	char* bname = getFileName(name, bname);
	printf("basename: %s\n", bname);
	//store separated file name and extension 
	char* fileName[2];
	//populate array and remove .
	parseCmd(bname, fileName, ".");
	//call myCreate() to write to file all data
	myCreate(fp, fileName[0], fileName[1],'1', 0, "");
	
}
/*********************************************************************************************
 * This function creates an empty directory in the file system.                              *
 *********************************************************************************************/
void createEmptyDir(FILE* fp, char* name){
	printf("createEmptyDir() called\n");
	//get file name
	char* bname = getFileName(name, bname);
	printf("basename: %s\n", bname);
	//store separated file name and extension 
	char* fileName[2];
	//populate array and remove .
	parseCmd(bname, fileName, ".");
	//call myCreate() to write to file all data
	myCreate(fp, fileName[0], "   ", '0', 0, "");
}

/*********************************************************************************************
 * This function returns the filename from a directory path.                                 *
 *********************************************************************************************/
char* getFileName(char* path, char* bname){
	char* path2 = strdup(path);
	bname = basename(path2);
	return bname;
}

/*********************************************************************************************
 * This function counts the number of characters in the data file.                           *
 * Use case is to get size of array dynamically at runtime.                                  *
 *********************************************************************************************/
int countCharNum(FILE* file){
	rewind(file);
	char c;
	int count = 0;
	while((c = fgetc(file))){
		if(c != EOF){
			count++;
			//printf("character %c count %d\n", c, count);
		} else {
			break;
		}
	}
	return count;
}

/*********************************************************************************************
 * This function reads in a data file into a char array.                                     *
 *********************************************************************************************/
void readData(FILE* file, char arr[], int size){
	rewind(file);
	int i;
	for(i = 0; i < size; i++){
		arr[i] = fgetc(file);
		printf("arr[%d] = %c\n", i, arr[i]);
	}
}

/*********************************************************************************************
 * This function copies one array into the other assuming that they are the same size.       *
 *********************************************************************************************/
void copyArrays(char* src, char* dest, int size){
	int i;
	for(i = 0; i < size; i++){
		src[i] = dest[i];
	}
}


/*********************************************************************************************
 * This function writes to the boot sector and sets the global variables.                    *
 *********************************************************************************************/
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

//TODO: refactor into smaller methods
/*********************************************************************************************
 * This function traverses through directories to find the file location                   
 *********************************************************************************************/
void traverse(FILE* fp, char* dir_files[], int size){
	int i;
	int startOffset = RD_OFFSET;
	int endOffset = DATA_OFFSET - 1;
	unsigned char metadata[32];
	char attr;
	unsigned char fileChSize[4];
	int fileSize = 0;
	unsigned short fatVal = -1;
	unsigned short cur_Val = -1;
	int offset = findEntry(fp, startOffset, endOffset, dir_files[0], metadata);
	printf("offset %d\n", offset);
	if(offset != -1){
		//get file type, dir or file
		attr = metadata[11];
		//get file size via byte manipulation 
		fileSize += metadata[28] | (metadata[29]<<8) | (metadata[30]<<16) | (metadata[31]<<24);
		printf("fileSize %d\n", fileSize);
		unsigned char dataRead[fileSize];	//stores data to read
		
		//TODO: reimplement to handle noncontiguous files
		if(attr == '1'){		//if is a file
			printf("I'm a file %c\n", attr);
		//loop until you find 0xffff
		//read in current fat value at offset
		//fseek(fp, (offset+FAT_OFFSET)*SECTOR_SIZE, SEEK_SET);
		//fread(&fatVal, sizeof(unsigned short), 1, fp);
			
		//keep looping until stop condition -1 is read	
		//do{
			//look for respective data region from 1-to-1 mapping
			fseek(fp, (offset+DATA_OFFSET)*SECTOR_SIZE, SEEK_SET);
			//since we know the file size, read in all data assuming contiguous
			fread(dataRead, sizeof(unsigned char), fileSize, fp);
			/*
			for(i = 0; i < fileSize; i++){
				printf("dataRead[%d]: %c\n", i, dataRead[i]);
			}
			*/
			printf("input:\n %s\n", dataRead);

			//update fat val
		//}while(fatVal != 0xFFFF)		
		} else if(attr == '0'){ //attr is a directory keep going
			printf("I'm a directory %c\n", attr);
		} else {
			printf("error no such attribute for file/directory\n");
		}
	} else {
		printf("Error, no such file or directory, offset was -1\n");
	}
}



//TODO: refactor into smaller methods
/*********************************************************************************************
 * This function finds specific entries from fat of the current file/directory in the data   *
 * region and returns fat entry on success, -1 otherwise                                     *
 *********************************************************************************************/
int findEntry(FILE* fp, int s_offset, int e_offset, char* name, char* metadata){
	printf("findEntry() called\n");
	int i;
	int entrySize = 32;			//each file or directory entry is 32 bytes big
	int nameLength = 8;			//file name length
	unsigned short val = 0;
	int offset = -1;

	unsigned char bytes_read[entrySize];
	char fileName[nameLength];
	//scan directory for matching entry specified by filename
	for(i = s_offset*SECTOR_SIZE; i <= e_offset*SECTOR_SIZE; i+=32){
		printf("dir/file entry location in bytes = %d\n", i);
		fseek(fp, i, SEEK_SET);										//scan every 32 bytes
		//printf("ftell(); %ld\n", ftell(fp));
		fread(bytes_read, sizeof(unsigned char), entrySize, fp);	//read into array 32 bytes
		copyArrays(fileName, bytes_read, nameLength); 	//read into char array filename
		copyArrays(metadata, bytes_read, entrySize);	//read into char array copy of meta data

		//Read in starting cluster which is 26, 27. Use memcpy to read into unsigned short
		/*
		unsigned char temp[2];
		temp[1] = bytes_read[26];
		temp[2] = bytes_read[27];
		printf("%c%c\n", bytes_read[26], bytes_read[27]);
		printf("temp = %s\n", temp);
		memcpy(&val, temp, sizeof(val));
		printf("starting cluster: %hd\n", val);
		*/
		val += bytes_read[26] | (bytes_read[27]<<8);
		printf("starting cluster: %hd\n", val);
		//get file comparison
		int fileMatch = strcmp(fileName, name);
		//compare file names, attr, ext, and starting cluster
		if(fileMatch == 0){
			printf("SUCCESS!!!\n");
			//offset from fat = sector offset from data region
			offset = val - SECTOR_SIZE; 
			printf("offset is %d\n", offset);
			break;
		} else {
			printf("No, not here.%c\n", bytes_read[25]);
		}
	}
	return offset; 
}


//TODO: refactor into smaller methods
/*********************************************************************************************
 * Can create empty files and directories and duplicate existing files with data             *
 *********************************************************************************************/
void myCreate(FILE* fp, char* fname, char* extName, char attr, int fileSize, char* data){
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
		//Create file/dir struct
		struct RD* r = malloc(sizeof(struct RD));
		strcpy(r->filename, fname);
		strcpy(r->ext, extName);		
		r->attr = attr;					//set as a file or as directory
		getDateTime(r);					//set datetime

		r->occupied='1';				//set as occupied 
		r->startCluster = fatLocation;	//store fat byte address entry 
		r->fileSize = fileSize; //0; //1322;
		
		//if it is a directory type 
		//point to an empty sector as usual in startcluster
		//write directory field there
		
		//test input data from text file
		//char* dataTest = "HELLO BLA BLAH BLAH TESTING OKAY WHATEVER PEACE.";
		//dataTest = "";
		
		//char data[r->fileSize];
		//strcpy(data, fileData);
		
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


/*********************************************************************************************
 * This function counts number of sectors to write to returning the sector count.            *
 *********************************************************************************************/
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

//TODO: refactor into smaller methods & reimplement other write functions
/*********************************************************************************************
 * This function writes to the fat and the data section, creating a mapping between the two. *
 * data input is  segments at a time;                                                        *
 * Handles mapping for file/dir with or without data                                         *
 *********************************************************************************************/
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
	} else {		//write nonempty file
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


/*********************************************************************************************
 * This function copies data from one array to the the array blocks where length is the      *
 * length of data. If any size is less than 512, the rest of it is 0 so as to not            *
 *********************************************************************************************/
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

/*********************************************************************************************
 * This function writes to the Fat the next fat entry as a unsigned short                    *
 * in the current fat position                                                               *
 *********************************************************************************************/
void writeToFat(FILE* fp, unsigned short* data, int location){
	printf("writeToFat() called to location %d, sector %d\n", location, location/SECTOR_SIZE);
	fseek(fp, location, SEEK_SET);
	fwrite(&data, sizeof(unsigned short), 1, fp);
}

/*********************************************************************************************
 * This function writes to the data section an array of char of size 512.                    *
 *********************************************************************************************/
void writeToData(FILE* fp, char* data, int location){
	printf("writeToData() called to location %d, sector %d\n", location, location/SECTOR_SIZE);
	fseek(fp, location, SEEK_SET);
	fwrite(&data, strlen(data), 1, fp);
}



/*********************************************************************************************
 * This function finds the empty sector in a data region. Used to write to data region.      *
 * Returns the sector in bytes so must divide by SECTOR_NUMBER to get sector number.         *
 *********************************************************************************************/
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


/*********************************************************************************************
 * This function finds the first empty byte entry in the region and returns the location.    *
 * The start and end offsets define the region boundaries.                                   *
 * start is defined as the start of one region, end is the start of the next region.         *
 *********************************************************************************************/
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

/*********************************************************************************************
 * This function finds an empty location in the root directory or directory structure by     *
 * checking the occupied field in the virtual drive s.                                       *
 *********************************************************************************************/
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

/*********************************************************************************************
 * This function puts in the datetime into the struct element in the form HHMMSSMMDDYY.      *
 * The order is time first followed by date.                                                 *
 *********************************************************************************************/
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
void parseCmd(char* input, char* arr[], char* delim){
   char* token;
   char delimiter[2];
   strcpy(delimiter, delim);
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

/*********************************************************************************************
 * This function checks user input to see if the command entered is for instance a newline,  *
 * spaces, or characters that aren't alphanumeric and sets the flag to true.                 *
 *                                                                                           *
 * Preconditions:                                                                            *
 * @params cmd - the command entered into terminal                                           *
 * Postconditions:                                                                           *
 * @return flag of true or false                                                             *
 *********************************************************************************************/
int verifyInput(char* cmd){
	int flag = 1;				//flag up for \n, ' ', etc. 
	while(*(cmd) != '\0'){		
		if(isalpha(*(cmd))){	//if input contains any alphanumeric, set flag down
			flag = 0; 
		}
		*(cmd)++;
	}
	return flag;
}

/*********************************************************************************************
 * This function removes leading and trailing characters of the delimiter passed in,         *
 * specifically white spaces for this program.                                               *
 *                                                                                           *
 * Preconditions:                                                                            *
 * @params input - input string to change                                                    * 
 * @params delim - character of delimiter to look for and remove                             *
 * Postconditions:                                                                           *
 * @return formatted string without trailing/leading delimiters                              *
 *********************************************************************************************/
char* trim(char* input, char delim){
	//remove leading whitespace
	while(*input == delim){		     			//Move from front to back of string 
		input++;					 
	}								
	
	//remove trailing whitespace
	int length = strlen(input);
	char* str = input + length - 1;
	while(length > 0 && *input == delim){		//Move from back to front of string
		str--;
	}
	*(str+1) = '\0';							//Add a null character at end
	return input;
}
