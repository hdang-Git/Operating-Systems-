#include<stdio.h>
#include<stdlib.h>
#include <unistd.h>
#define REP 120			//Number of charecters to write
#define CHARSTART 33		//first ascii character is ! -> this is to avoid odd characters
#define CHAREND 126		
#define LINES 10
#define TIME 1

//function prototype
char randomAlpha();
void checkAlpha();
void readFile(int, char[], char[][REP]);
void writeFile(int, char[], char[][REP]);
void printArray(char[][REP]);
void printLine(int, char[][REP], char*);
void compareArrays(char[][REP], char[][REP]);
void deleteFile(char*);

//TODO: add while loop of time, to delay the I/O process

int main()
{

   char str[LINES][REP];		
   char copy[LINES][REP];		
   srand(time(NULL));  //Define globally by default, should only be created once for randomness.
   int lines = 10;
   char* fName = "outFile.txt";

   writeFile(lines, fName, str);
   readFile(lines, fName, copy);
   printf("\n");
   printArray(copy);
   compareArrays(str, copy);
   deleteFile(fName);
   return 0;
}

char randomAlpha()
{
   char random;
   random = (char) (CHARSTART + rand()%(CHAREND-CHARSTART));
   return random;
}

void writeFile(int lines, char fileName[], char str[][REP]){
   //printf("%d  %s\n", lines, fileName);
   int i;
   int j;
   
   FILE *output;
   
   output = fopen(fileName, "w");
   printf("Writing to file\n");
   for(i = 0; i < lines; i++){
      printf("filling array %d\n", i);
      sleep(TIME);
      for(j = 0; j < REP; j++){  
        str[i][j] = randomAlpha();
        //fprintf(output, "%c", str[i][j]); //This works too!     
      } 
      fwrite(str[i], sizeof(char), sizeof(str[i]), output);	
      printf("writing now\n");
      fprintf(output, "%s", "\r\n");
   }
   printf("DONE\n");
   fclose(output);
}

//TODO: add error handling if file DNE
void readFile(int lines, char fileName[], char copy[][REP]){
    int i;

    FILE *input;
    input = fopen(fileName, "rb+");
/*
    for(i = 0; i < lines; i++){
	printf("Seeking through file\n");
	fscanf(input, "%s", copy[i]);
    }
*/
    
    while(!feof(input)){
	if(i == 0)
	    fseek(input, i * 120,SEEK_SET);
	else 
	    fseek(input, i * 120 + i * 2, SEEK_SET);
	printf("ftell(): %ld\n",ftell(input));
        sleep(TIME);
	fread(copy[i],sizeof(char),sizeof(copy[i]),input);
	i++;
    }
    printf("DONE reading\n");
    fclose(input);
}


void printLine(int num, char arr[][REP], char* name){
    int i;
    printf("\n%s\n", name);
    for(i = 0; i < REP; i++){
        printf("%c", arr[num][i]);
    }
    printf("\n");
}

void printArray(char copy[][REP]){
   int i, j;
   for(i = 0; i < LINES; i++){
      printf("%d.\n", i);
      for(j = 0; j < REP; j++){
      	printf("%c", copy[i][j]);
      }
      printf("\n");
   }
}

void compareArrays(char str[][REP], char copy[][REP]){
    int random = rand() % 10;
    printf("\nrandom #: %d\n", random);
    printLine(random, copy, "copy");
    printLine(random, str, "str");
    int flag = 1; //true
    int i = 0;
    char* message = "Strings are the same";
    
    while(i < REP && flag){
       if(str[random][i] != copy[random][i]){
           flag = 0;
	   message = "FALSE NOT EQUAL "; 
       }        
       i++;
    }
    printf("%s\n",message);
    
}


void deleteFile(char* fileName){
    if(!remove(fileName))  //if removing the file returns the 0 code for deletion
       printf("File %s deleted successfully\n", fileName);
    else 
       printf("Error, unable to delete file: %s.\n", fileName);
}

//shows that time is defaulted to seconds
//Also using random numbers from 33 to 126 to avoid odd characters
//TODO: delete this func and all usagez
void checkAlpha(){
   int i;
   char random;
   for(i = 33; i <= 126; i++){
     random = randomAlpha();
     printf("%d:%c ", i, random);
     if(((i-32) % 10) == 0)
       printf("\n");
   }
   printf("\n");
}


