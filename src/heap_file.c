#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "heap_file.h"
#include "../include/bf.h"

// We use CALL_OR_DIE to manage bf_errors that may occur in our HP_Functions
#define BF_CALL_OR_DIE(call)  \
  {                           \
    BF_ErrorCode code = call; \
    if (code != BF_OK) {      \
      BF_PrintError(code);    \
      exit(code);             \
    }                         \
  }                          

// Print block's data
void printdata(char* data)
{
  printf("Printing Data...\n");
  int i = 0;
  for ( i = 0 ; i <= BF_BLOCK_SIZE ; i++)
  {  
        printf("%c", data[i]);
        if(data[i] == '\0')
          printf("n" );
  }
  printf("\n");
}

HP_ErrorCode HP_Init() {
  // Nothing to see here , you can continue scrolling
  return HP_OK;
}

HP_ErrorCode HP_CreateIndex(const char *filename) {
  // printf("Hey from Create Index\n");
  BF_Block* block;
  BF_Block_Init(&block);
  int fd; 
  char* data;
  BF_CALL_OR_DIE(BF_CreateFile(filename)); // Create the empty blocks file
  BF_CALL_OR_DIE(BF_OpenFile(filename,&fd)); // Open the file so we can do some things to it
  BF_CALL_OR_DIE(BF_AllocateBlock(fd,block)); // Create our first metadata-manager block (ain't something special)
  data = BF_Block_GetData(block);
  memset(data, 'h' , 1); // data[0] = h [heap] will baptize our file as heap
  memset(data+1,'0', 1); // number of records that our heap file has
  BF_Block_SetDirty(block); // dirty cause we touched it
  BF_CALL_OR_DIE(BF_UnpinBlock(block)); //unpin
  BF_CALL_OR_DIE(BF_CloseFile(fd)); // Close 
  return HP_OK;
}

HP_ErrorCode HP_OpenFile(const char *fileName, int *fileDesc){
  // printf("Hey from Open File\n");
  BF_Block* block;
  BF_Block_Init(&block);
  char* data;  
  BF_CALL_OR_DIE(BF_OpenFile(fileName,fileDesc)); // Open again what we made
  BF_CALL_OR_DIE(BF_GetBlock(*fileDesc,0,block)); // Get our first block to get some info
  data = BF_Block_GetData(block);
  
  // Let's see if this file belongs to the right place
  if ( data[0] == 'h')
    printf("It is a heap file!\n");
  else
  {
    printf("Not Heap File\n");
    return HP_ERROR; // You can go this way sir...
  }
  BF_CALL_OR_DIE(BF_UnpinBlock(block)); // unpin
  return HP_OK;
}

HP_ErrorCode HP_CloseFile(int fileDesc) {
  //printf("Hey from Close File\n");
  BF_CALL_OR_DIE(BF_CloseFile(fileDesc)); // Yeah marvelous function
  return HP_OK;
}

HP_ErrorCode HP_InsertEntry(int fileDesc, Record record) {
  // printf("Welcome from Insert Entry");
  int blocks_num; 
  char *data; 
  BF_Block* block; 
  BF_Block_Init(&block);
  
  int already = 0; // do we have a starting record holding block ?
  
  BF_CALL_OR_DIE(BF_GetBlockCounter(fileDesc,&blocks_num)); // Let's see how many block do we have
  // printf("Number Of blocks %d\n",blocks_num);
  
  // If there are no blocks in our heap file , create the first one
  if(blocks_num == 1)
  {
    BF_CALL_OR_DIE(BF_AllocateBlock(fileDesc,block));
    data = BF_Block_GetData(block);
    // data = [Number Of Records|-|-|...]
    memset(data,'0', 1);
    already = 1; // okey , now we have
  }
  else
  {  
    // Get last block 
    BF_CALL_OR_DIE(BF_GetBlock(fileDesc,blocks_num-1,block));
    data = BF_Block_GetData(block);
  }

  int nor = data[0] - '0'; // number of records in block
  int fup = nor * 60 + 1; // first unwritten cell , achieves it with right indexing
  // printf("fup :%d\n ", fup);

  // check if you fit , if you don't get a block guys
  if (!already && 1024 - fup < 60)
  {
    //printf("We will use a new one\n");
    BF_CALL_OR_DIE(BF_UnpinBlock(block)); // unpin the last block
    
    BF_CALL_OR_DIE(BF_AllocateBlock(fileDesc,block));
    data = BF_Block_GetData(block);
    memset(data,'0', 1); // data[0] = Number of records in the block , don't forget that

    nor = data[0] - '0'; // make it integer
    fup = nor * 60 + 1; // first unwritten cell
  }

  // Insert record after last record
      // Warning ! Complex math follows , but it's for indexing reasons
            // write id
  char sid[5];
  sprintf(sid, "%d", record.id);
  strncpy(data+fup,sid,5);
  fup += 5;
            // write name
  strncpy(data+fup ,record.name , 15);
  fup += 15;
            // write surname
  strncpy(data+fup , record.surname , 20);
  fup += 20;
            // write city
  strncpy(data+fup , record.city , 20);
  fup += 20;

  nor ++ ; // we have a new records here , write him
  data[0] = nor +'0';
  // -------------------------------
  //printdata(data);
  
  BF_Block_SetDirty(block); // we did touch this block pretty hard 
  BF_CALL_OR_DIE(BF_UnpinBlock(block)); // unpin

  // update the metadata manager
  BF_CALL_OR_DIE(BF_GetBlock(fileDesc,0,block));
  data = BF_Block_GetData(block);
  nor = data[1] - '0'; 
  nor ++ ;
  data[1] = nor +'0';
  BF_Block_SetDirty(block);
  BF_CALL_OR_DIE(BF_UnpinBlock(block));

  return HP_OK;
}


HP_ErrorCode HP_PrintAllEntries(int fileDesc) {
  //printf("Welcome from Print All Entries");
  int blocks_num; 
  int nor;
  char *data; 
  BF_Block* block; 
  BF_Block_Init(&block);

  int i,j;

  // number of blocks that we will print
  BF_CALL_OR_DIE(BF_GetBlockCounter(fileDesc, &blocks_num));
  for (i = 1; i < blocks_num; ++i) 
  {
    BF_CALL_OR_DIE(BF_GetBlock(fileDesc, i, block));
    data = BF_Block_GetData(block);
    // number of records contained in block , that we will print
    nor = data[0] - '0';
    
    //printdata(data);
    //printf("block = %d\n",i);
 
    // Same kind of black magic math , so we can print the right things   
    data += 1;
    for(j = 1; j<=nor ; j++ )
    {
      printf("Record \n------------\n");
      printf("ID : %s\n",data);
      data += 5;
      printf("Name : %s\n",data);
      data += 15;
      printf("Surame : %s\n",data);
      data += 20;
      printf("City: %s\n \n",data);
      data += 20;
    }
    BF_CALL_OR_DIE(BF_UnpinBlock(block)); // unpin
  }

  return HP_OK;
}

HP_ErrorCode HP_GetEntry(int fileDesc, int rowId, Record *record) {
  //printf("Welcome from insert entry");
  int blocks_num; 
  char *data; 
  BF_Block* block; 
  BF_Block_Init(&block);

  // Last time you see our math mechanics , please don't leave us 
  
  int toblock; // the block that our rowId block hides...
  if ((rowId%17) != 0)
    toblock = (rowId / 17) + 1;
  else
    toblock = rowId/17;

  int inblock; // the exact spot where rowId block hides..
  if (rowId%17 == 0) 
    inblock = 16;
  else
    inblock = (rowId%17)-1;

  //printf("%d\n", toblock);
  //printf("%d\n", inblock);

  // Busted , we found him , bring him here
  BF_CALL_OR_DIE(BF_GetBlock(fileDesc, toblock , block));
  data = BF_Block_GetData(block);  

  // Split our data
  data = data + 1 + inblock * 60;

  // Get the info in record , and back to base
  record->id = atoi(data);
  data += 5;
  strcpy(record->name,data);
  data += 15;
  strcpy(record->surname,data);
  data += 20;
  strcpy(record->city,data);
  data += 20;

  BF_CALL_OR_DIE(BF_UnpinBlock(block)); // unpin

  return HP_OK; // mission accomplished 
}
