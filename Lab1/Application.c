#include<stdio.h>
#include<stdlib.h>

#define REP 120			//Number of charecters to write
#define CHARSTART 33		//first ascii character is ! -> this is to avoid odd characters
#define CHAREND 126		//
#define LINES 10

char str[LINES][REP];		//Global array
char copy[LINES][REP];		//Read 
//function prototype
char randomAlpha();
void checkAlpha();
void readFile(int, char[]);
void writeFile(int, char[]);
void printArray();
//void compareArrays();

//TODO: add while loop of time, to delay the I/O process

int main()
{
   srand(time(NULL));  //Define globally by default, should only be created once for randomness.
   //printf("Random number: %c\n", randomAlpha());
   //checkAlpha();
   int lines = 10;
   char* fName = "outFile.txt";
   //writeFile(lines, fName);
   readFile(lines, fName);
   printf("\n");
   printArray();
   return 0;
}

char randomAlpha()
{
   char random;
   random = (char) (CHARSTART + rand()%(CHAREND-CHARSTART));
   return random;
}

void writeFile(int lines, char fileName[]){
   //printf("%d  %s\n", lines, fileName);
   int i;
   int j;
   
   FILE *output;
   
   output = fopen(fileName, "w");
   printf("Writing to file\n");
   for(i = 0; i < lines; i++){
      printf("filling array %d\n", i);
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
void readFile(int lines, char fileName[]){
    int i;

    FILE *input;
    input = fopen(fileName, "rb+");
/*
    for(i = 0; i < lines; i++){
	printf("Seeking through file\n");
	fscanf(input, "%s", copy[i]);
    }
*/
/*
    while(!feof(input)){
	printf("Seeking through file\n");
        fread(copy[i], sizeof(char), sizeof(copy[i]), input);
	i++;
    }
*/
    
    while(!feof(input)){
	if(i == 0)
	    fseek(input, i * 120,SEEK_SET);
	else 
	    fseek(input, i * 120 + i * 2, SEEK_SET);
	printf("ftell(): %ld\n",ftell(input));
	fread(copy[i],sizeof(char),sizeof(copy[i]),input);
	i++;
    }
    printf("DONE reading\n");
    fclose(input);
}


void printArray(){
   int i, j;
   for(i = 0; i < LINES; i++){
      printf("%d.\n", i);
      for(j = 0; j < REP; j++){
      	printf("%c", copy[i][j]);
      }
      printf("\n");
   }
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


