#include <stdio.h>
#include <stdlib.h>
#include <time.h>	

#define SECTOR_SIZE 512
#define VBR_OFFSET 0
#define FAT_OFFSET 1
#define RD_OFFSET 241
#define DATA_OFFSET 273

//function prototypes
void fillReservedSec();
void getDateTime();


unsigned char byte; 		//uint8_t 
unsigned short twoBytes;  	//uint16_t
FILE* fp;

//Reserved Region/Volume Boot Record
struct VBR{
	unsigned short BPS;				//bytes per sector (size of sector)  2 bytes
	unsigned short SPC; 			//sectors per cluster
	unsigned short RSC;				//reserved sector count
	unsigned short NFT;				//Number of FAT tables
	unsigned short MNRE;			//Max number of root entries
	unsigned short totalSectors;	//total number of sectors
	unsigned short ptrSize;			//size of pointer (2 MB)
	unsigned short sysType;			//FAT system type
};


//Root Directory (32 bytes) 
struct RD{
	unsigned char filename[8];		//8 bytes filename
	unsigned char ext[3];			//3 bytes extension
	unsigned char attr;				//1 byte attribute
	unsigned short reserved1;		//2 bytes reserved/unused   //set to 0
	unsigned short createdTime;		//2 bytes created time		//change
	unsigned short createdDate;		//2 bytes created date		//change
	unsigned short accessDate;		//2 bytes last access date	//change
	unsigned short reserved2;		//2 bytes reserved/unused	//set to 0
	unsigned short modifiedTime;	//2 bytes modified time		//change
	unsigned short modifiedDate;	//2 bytes modified date		//change
	unsigned short startCluster;	//2 bytes starting cluster	//change
	unsigned int fileSize;			//4 bytes file size
};


/*
	CALCULATION NOTES:
 	2MB = 2,097,152 bytes  = 4096 sectors   
 
 */

int main(){
	fp = fopen("Drive2MB", "r+");
	fillReservedSec();
	return 0;
}

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




//convert date to hexadecimal
//get timestamp
//subtract year from 1980 remainder is the value we want
//   remainder, month number, day number
//	  all two bytes together

//convert time to hexadecimal
//


void getDateTime(){
	time_t rawtime;
   struct tm *info;
   char buffer[80];

   time( &rawtime );

   info = localtime( &rawtime );

   strftime(buffer,80,"%x %I:%M%p", info);
   printf("Formatted date & time : |%s|\n", buffer );
}
	
