/*
 * This program writes a file with ten lines of random characters. During the write process, these 
 * random characters are stored in a two-dimensional array. Then the program reads the output file 
 * and then compares it against what's stored in local memory. Afterwards, the file is then deleted.
 *
 *
 * 	Note: 
 * 		sample cmd - 
 *              Ex.  ./app output.txt "32323325" time.csv
 *              Ex.  ./a.out testing.txt "910938598" t.csv
 */

#define LINES 10		//Number of lines/rows to write
#define REP 120			//Number of characters/columns to write
#define CHARSTART 33		//first readable ascii character
#define CHAREND 126		//last readable ascii character
#define TIME 125000		//Sleep time is in microseconds

#include<stdio.h>
#include<stdlib.h>
#include <unistd.h>
#include<sys/time.h>


//function prototypes
char randomAlpha();
void readFile(char[], char[][REP]);
void writeFile(char[], char[][REP]);
void printArray(char[][REP]);
void printLine(int, char[][REP], char*);
void compareArrays(char[][REP], char[][REP]);
void deleteFile(char*);


/*********************************************************************************************
 * The main function takes in arguments and calls the writeFile(), readFile(), printArray(), *
 * compareArrays(), and deleteFile() methods that create a 2 dimensional array, fill it with *
 * random char characters, writes to file, then reads file, stores read data to another 2d   *
 * array, compares the two arrays, and deletes the file.                                     *
 * Preconditions:                                                                            *
 * Expects argv[1] to store a string filename i.e. "output.txt"                              *
 * Expects argv[2] to store the start time passed in as a string  i.e. "4993292930"          *
 * Expects argv[3] to store the output filename for the process time i.e. "timeOutput.csv"   *
 *********************************************************************************************/
int main(int argc, char* argv[])
{
   
   char str[LINES][REP];   		//two-dim array to store characters when writing	
   char copy[LINES][REP];  		//two-dim array to store characters from reading
   srand(time(NULL));  	   		//Define globally by default, should only be created once for randomness.
   char* fName = argv[1];  			     //store the filename for this program's output 
   char* ptr;					
   long start = strtol(argv[2], &ptr, 10);	     //Convert string to long	
   printf("Start time: %ld\n", start); 
   
   struct timeval t;
   gettimeofday(&t, NULL);			    //get the end time in microseconds
   long int end = t.tv_sec * 1000000L + t.tv_usec;  //Convert s & us to us
   long int diff = end - start;			    //calculate the difference
   char* csvFile= argv[3];			    //get timer output filename
   FILE*  outputCSV = fopen(csvFile, "a");	    //Open csv file					
   printf("\nEnd time of child : %ld\n", end);
   printf("TIME DIFF: %ld\n", diff);   
   fprintf(outputCSV, "%ld,\n", diff);		    //write to file the difference delimited by ,& \n
   printf("Wrote to timer file\n");
   fclose(outputCSV); 				    //close the file I/O stream

   writeFile(fName, str);  	//Call writeFile() to write random char to a file and store in str
   readFile(fName, copy);	//Call readFile() to read in file to copy array
   printf("\n");
   printArray(copy);		//Call printArray() to print out copy array 
   compareArrays(str, copy);	//Call compareArrays() to compare both str and copy array in memory
   deleteFile(fName);		//Call deleteFile() to delete file from current directory 
   return 0;
}


/*********************************************************************************************
 * This function creates a random character by using the rand() function and casting it to a *
 * char. And then it returns the character                                                   *
 *                                                                                           *
 * Postconditions:                                                                           *
 * @returns random character to caller.                                                      *
 *********************************************************************************************/
char randomAlpha()
{
   char random;
   random = (char) (CHARSTART + rand()%(CHAREND-CHARSTART));
   return random;
}


/*********************************************************************************************
 * This function iterates through the desired lines and calls randomAlpha() to return a      *
 * random character. When the character is returned, it is stored in each column of the 2-d  *
 * array and each row of the array is then written to the output file.                       *
 *                                                                                           *
 * Preconditions:                                                                            *
 * @param lines - number of lines                                                            *
 * @param fileName - name of the file,                                                       *
 * @param str - array to populate                                                            *
 * @return void										     *
 *********************************************************************************************/
void writeFile(char fileName[], char str[][REP]){
   int i;
   int j;
   
   FILE *output;
   
   output = fopen(fileName, "w");				//open or create file if DNE, and set mode to write
   printf("Writing to file\n");
   for(i = 0; i < LINES; i++){
      printf("filling array %d\n", i);
      usleep(TIME);						//Pause for each line write to lengthen time of process
      for(j = 0; j < REP; j++){  
        str[i][j] = randomAlpha();    				//Call randomAlpha() to return a random character and store in array
      } 
      
      fwrite(str[i], sizeof(char), sizeof(str[i]), output);	//write each line stored in array to output file 
      printf("writing now\n");
      fprintf(output, "%s", "\r\n");				//write a new line to file as a delimiter
   }
   printf("DONE\n");
   fclose(output);						//Close the file I/O stream
}

/*********************************************************************************************
 * This function reads in a file by setting up an input file stream, seeking through the     *
 * file for the desired position and then read into the 2-d array the data from the file     *
 * while the file is not empty                                                               *
 *                                                                                           *
 * Preconditions:                                                                            *
 * @param lines - number of lines                                                            *
 * @param fileName - name of the file,                                                       *
 * @param copy - array to populate                                                           *
 * @return void		                                                                     *
 *********************************************************************************************/
void readFile(char fileName[], char copy[][REP]){
    int i;

    FILE *input; 
    input = fopen(fileName, "r");				//open file for reading
   
    while(!feof(input)){                                        //while not the end of the file
	if(i == 0)	
	    fseek(input, i * 120,SEEK_SET);                     //set cursor position to beginning of file
	else 
	    fseek(input, i * 120 + i * 2, SEEK_SET);            //set cursor to calculated position to account for \r\n delimiters
	printf("ftell(): %ld\n",ftell(input));			//print to screen the position of the cursor
        usleep(TIME);						//Pause the program to lengthen time of process
	fread(copy[i],sizeof(char),sizeof(copy[i]),input);	//read each line of the file and populate it into the array
	i++;
    }
    printf("DONE reading\n");
    fclose(input);						//Close the file I/O stream
}

/*********************************************************************************************
 * This function prints to console/terminal the values stored in the current row of the      *
 * array.                                                                                    *
 *                                                                                           *
 * Preconditions:                                                                            *
 * @param num - row number                                                                   *
 * @param arr - two-dimensional array                                                        *
 * @param name - name of the array                                                           *
 * @return void		                                                                     *
 *********************************************************************************************/
void printLine(int num, char arr[][REP], char* name){
    int i;
    printf("\n%s\n", name);
    for(i = 0; i < REP; i++){
        printf("%c", arr[num][i]);
    }
    printf("\n");
}

//TODO: chagnge name of copy to arr
/*********************************************************************************************
 * This function loops through the array and prints out its contents                         *
 *                                                                                           *
 * Preconditions:                                                                            *
 * @param arr - two-dimensional array                                                        *
 * @return void                                                                              * 
 *********************************************************************************************/
void printArray(char arr[][REP]){
   int i, j;
   for(i = 0; i < LINES; i++){
      printf("%d.\n", i);
      for(j = 0; j < REP; j++){
      	printf("%c", arr[i][j]);
      }
      printf("\n");
   }
}

/*********************************************************************************************
 * This function takes two arrays and creates a random number from 0 to 9. It then chooses   *
 * that index as the line number to compare the contents stored at that index for both       *
 * arrays.                                                                                   *
 *                                                                                           *
 * Preconditions:                                                                            *
 * @param str - two-dimensional array storing written data                                   *
 * @param copy - two-dimensional array storing read data                                     *
 *********************************************************************************************/
void compareArrays(char str[][REP], char copy[][REP]){
    int random = rand() % 10;		 	//generate a random number from 0 to 9 for the index
    printf("\nrandom #: %d\n", random);
    printLine(random, copy, "copy");		//Call printLine() to print to screen the selected row of copy
    printLine(random, str, "str");   		//Call printLine() to print to screen the selected row of str
    int flag = 1; 				//set a flag to true
    int i = 0;
    char* message = "Strings are the same";     //Default message if true
    
    while(i < REP && flag){                     //while counter is less than the number of columns and the flag is true
       if(str[random][i] != copy[random][i]){   //if the characters don't match set the flag to 0 and change the message
           flag = 0;
	   message = "FALSE NOT EQUAL "; 
       }        
       i++;
    }
    printf("%s\n",message);
    
}

/*********************************************************************************************
 * This function deletes the output file from the current diretory.                          *
 *                                                                                           *
 * Preconditions:                                                                            * 
 * @param fileName - name of the file to delete                                              *
 *********************************************************************************************/
//Note: Don't use 'if(!remove(fileName))' because error will occur 
void deleteFile(char* fileName){
    if(remove(fileName) == 0)  			//if removing the file returns the 0 code for deletion
       printf("File %s deleted successfully\n", fileName);
    else 
       printf("Error, unable to delete file: %s.\n", fileName);
}



