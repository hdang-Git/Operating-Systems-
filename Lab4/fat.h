#ifndef FAT_H
#define FAT_H

/*
 * This header files contains structs for various locations on disk
 */

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
	/*
	unsigned short reserved1;		//2 bytes reserved/unused   //set to 0
	unsigned short createdTime;		//2 bytes created time		//change
	unsigned short createdDate;		//2 bytes created date		//change
	unsigned short accessDate;		//2 bytes last access date	//change
	unsigned short reserved2;		//2 bytes reserved/unused	//set to 0
	unsigned short modifiedTime;	//2 bytes modified time		//change
	unsigned short modifiedDate;	//2 bytes modified date		//change
	*/
	unsigned char datetime[13];		//13 bytes for datetime
	unsigned char occupied;			//1 byte for occupied (1 for occupied, 0 for free)
	unsigned short startCluster;	//2 bytes starting cluster	//change
	unsigned int fileSize;			//4 bytes file size
};

#endif

