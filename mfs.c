// Name : Sanket Salunke | UTA ID : 1001764897
// Name : Darshan Dani   | UTA ID : 1001764937

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <time.h>
#include <stdbool.h>
#include<assert.h>



#define NUM_BLOCKS    4226
#define BLOCK_SIZE    8192
#define NUM_FILES     128
#define MAX_FILE_SIZE 10240000    //1250*8192  (blocks* BLOCK_SIZE)

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
// so we need to define what delimits our tokens.
// In this case  white space
// will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size

#define MAX_NUM_ARGUMENTS 15     // supports upto 15 arguments


extern int errno;
char *forclose;
char s[64];

FILE * fd;
uint8_t blocks[NUM_BLOCKS][BLOCK_SIZE];

struct Directory_Entry
{
    uint8_t valid; // flag to say directory is being used
    char    filename[32];
    uint32_t inode;

};

struct Inode
{
    uint8_t  valid;
    uint8_t  attributes;
    uint32_t size;
    uint32_t blocks[1250]; //pointers to the block- for each file block is around 10MB
	time_t time;
};

//directory is the pointer which points to block zero

//Lets declare pointer to the directory entry
struct Directory_Entry * dir;
struct Inode 		   * inodes;
uint8_t                * freeBlockList;
uint8_t                * freeInodeList;

void initializeBlockList()
{
    int i;
    for(i=0; i< NUM_BLOCKS; i++)
    {
        freeBlockList[i] = 1;
    }

}

void initializeInodes()
{
    int i;
    for(i=0; i< NUM_BLOCKS; i++)
    {
        inodes[i].valid      = 0;
        inodes[i].size       = 0;
        inodes[i].attributes = 0;

        int j;
        for (j=0; j< 1250; j++)
        {
            inodes[i].blocks[j] = -1;

        }

    }

}


void initializeInodeList()
{
    int i;
    for(i=0; i< NUM_FILES; i++)
    {
        freeInodeList[i] = 1;
    }

}



void initializeDirectory()
{
    int i;
    for(i=0; i< NUM_FILES; i++)
    {
        dir[i].valid =  0;      //Valid value of zero indicates that it is not used
        dir[i].inode = -1;      //any other inode value will be valid value

        memset( dir[i].filename, 0,32); //memset filename to zero
    }

}

int df()
{
    int i;
    int free_space=0;
    for(i=0; i< NUM_BLOCKS; i++)
    {
        if( freeBlockList[i] == 1)
        {
            free_space = free_space + BLOCK_SIZE;
        }

    }
    printf("Disk size remaining : %d",free_space);
    return free_space;
}

int df1()
{
    int i;
    int free_space=0;
    for(i=0; i< NUM_BLOCKS; i++)
    {
        if( freeBlockList[i] == 1)
        {
            free_space = free_space + BLOCK_SIZE;
        }

    }
    printf("%d \t",free_space);
    return free_space;
}

int findfreeBlock()
{

    int i;
    int ret = -1;
    for( i=10; i< NUM_BLOCKS;  i++)
    {
        if( freeBlockList[i] == 1 )
        {
            freeBlockList[i] = 0;
            return i;
        }

    }
    return ret;
}


int findFreeInode()
{

    int i;
    int ret = -1;
    for( i=10; i< NUM_FILES;  i++)
    {
        if( inodes[i].valid == 0 )
        {
            inodes[i].valid =1;
            return i;
        }

    }
    return ret;
}


int findDirectoryEntry( char * filename)
{
    //check for existing entry

    int i;
    //int ret = -1;
    for( i=0; i< NUM_FILES;  i++)
    {
        if(dir[i].valid == 1 && strncmp(filename, dir[i].filename, strlen(filename))==0)
        {
            return i;
        }

    }

    //find free space

    for( i=0; i< NUM_FILES;  i++)
    {
        if(dir[i].valid == 0 )
        {
            //dir[i].valid = 1;
            return i;
        }

    }
    return -1;
}

//function to put the file inside FS
int put(char *filename)
{
    struct stat buf;
    int ret;

    ret = stat( filename, &buf );

    if( ret == -1)
    {
        printf("File does not exists 2\n");
        return -1;
    }

    int size = buf.st_size;

    if( size > MAX_FILE_SIZE)
    {
        printf("File size too big\n");
        return -1;
    }


    if( size > df())     //remaining space on disk
    {
        printf("File exceeds remaining disk space\n");
        return -1;

    }
    //put file in the image

    int directoryIndex = findDirectoryEntry(filename);
    int inodeIndex     = findFreeInode();
	
	 if (inodeIndex == -1)
  {
     printf("Error is there\n");
     return -1;
  }

    dir[directoryIndex].inode = inodeIndex;
    strncpy( dir[directoryIndex].filename, filename, strlen( filename ));
	dir[directoryIndex].valid = 1;

    // The following chunk of code demonstrates similar functionality of your put command
    //

    int status;                   // Hold the status of all return values.
    // struct stat buf;                 // stat struct to hold the returns from the stat call

    // Call stat with out input filename to verify that the file exists.  It will also
    // allow us to get the file size. We also get interesting file system info about the
    // file such as inode number, block size, and number of blocks.  For now, we don't
    // care about anything but the filesize.
    status =  stat( filename, &buf );

    // If stat did not return -1 then we know the input file exists and we can use it.
    if( status != -1 )
    {

        // Open the input file read-only
        FILE *ifp = fopen ( filename, "r" );
        ifp = fopen ( filename, "r" );
        printf("Reading %d bytes from %s\n", (int) buf . st_size, filename );

        // Save off the size of the input file since we'll use it in a couple of places and
        // also initialize our index variables to zero.
        int copy_size   = buf . st_size;

        // We want to copy and write in chunks of BLOCK_SIZE. So to do this
        // we are going to use fseek to move along our file stream in chunks of BLOCK_SIZE.
        // We will copy bytes, increment our file pointer by BLOCK_SIZE and repeat.
        int offset      = 0;
		int block = 0;

        // We are going to copy and store our file in BLOCK_SIZE chunks instead of one big
        // memory pool. Why? We are simulating the way the file system stores file data in
        // blocks of space on the disk. block_index will keep us pointing to the area of
        // the area that we will read from or write to.

        // copy_size is initialized to the size of the input file so each loop iteration we
        // will copy BLOCK_SIZE bytes from the file then reduce our copy_size counter by
        // BLOCK_SIZE number of bytes. When copy_size is less than or equal to zero we know
        // we have copied all the data from the input file.
        while( copy_size > 0 )
        {
            //int block = 0;

            int block_index = findfreeBlock();
           


            inodes[inodeIndex].blocks[block] = block_index;
           // inodes[inodeIndex].valid = 1;
            inodes[inodeIndex].size  = buf.st_size;
			
            // Index into the input file by offset number of bytes.  Initially offset is set to
            // zero so we copy BLOCK_SIZE number of bytes from the front of the file.  We
            // then increase the offset by BLOCK_SIZE and continue the process.  This will
            // make us copy from offsets 0, BLOCK_SIZE, 2*BLOCK_SIZE, 3*BLOCK_SIZE, etc.
            fseek( ifp, offset, SEEK_SET );

            // Read BLOCK_SIZE number of bytes from the input file and store them in our
            // data array.
            int bytes  = fread( blocks[block_index], BLOCK_SIZE, 1, ifp );


            // If bytes == 0 and we haven't reached the end of the file then something is
            // wrong. If 0 is returned and we also have the EOF flag set then that is OK.
            // It means we've reached the end of our input file.
            if( bytes == 0 && !feof( ifp ) )
            {
                printf("An error occured reading from the input file.\n");
                return -1;
            }

            // Clear the EOF file flag.
            clearerr( ifp );

            // Reduce copy_size by the BLOCK_SIZE bytes.
            copy_size -= BLOCK_SIZE;

            // Increase the offset into our input file by BLOCK_SIZE.  This will allow
            // the fseek at the top of the loop to position us to the correct spot.
            offset    += BLOCK_SIZE;
			block++;

            //fd = fopen(filename, "w");
            //fwrite( blocks[block_index], copy_size, BLOCK_SIZE, fd);
        }


			//inodes[inodeIndex].time  = buf.st_ctime;     //for time change

        // We are done copying from the input file so close it out.
        fclose( ifp );

    }
}


//get function fetches the file from the file system to the Current Working dir.
int get(char * filename, char *fileOp)
{
	
	int directoryIndex = findDirectoryEntry(filename);
	
	
	int inodeIndex     = dir[directoryIndex].inode;
	
	if(directoryIndex == -1)
	{
		return -1;
	}
	
	
	
	struct Inode * inode = &(inodes[inodeIndex]);

	
	 if(!fileOp)
	{
        fileOp = filename;
	}
  
    FILE *ofp;
    ofp = fopen(fileOp, "w");
    if( ofp == NULL )
    {
        printf("Could not open output file: %s\n", fileOp );
        perror("Opening output file returned");
        return -1;
    }
    //int bytes  = fread( blocks[block_index], BLOCK_SIZE, 1, fd );
    // Initialize our offsets and pointers just we did above when reading from the file.
    int block_index = 0;
    int copy_size   = inodes[inodeIndex].size;                //buf . st_size;
    int offset      = 0;

   // printf("Writing %d bytes to %s\n", (int) buf . st_size, filename );

	printf("Writing %d bytes to %s\n", copy_size, fileOp );
    // Using copy_size as a count to determine when we've copied enough bytes to the output file.
    // Each time through the loop, except the last time, we will copy BLOCK_SIZE number of bytes from
    // our stored data to the file fp, then we will increment the offset into the file we are writing to.
    // On the last iteration of the loop, instead of copying BLOCK_SIZE number of bytes we just copy
    // how ever much is remaining ( copy_size % BLOCK_SIZE ).  If we just copied BLOCK_SIZE on the
    // last iteration we'd end up with gibberish at the end of our file.
    while( copy_size > 0 )
    {

        int num_bytes;
        //num_bytes  = fread( blocks[block_index], BLOCK_SIZE, 1, fd );   
			
		if( copy_size < BLOCK_SIZE )
        {
            num_bytes = copy_size;
        }
        else
        {
            num_bytes = BLOCK_SIZE;
        }
			
			
			
		//int num_bytes = min(BLOCK_SIZE, copy_size);
		int filledBlockIndex = inode->blocks[block_index];
		fwrite(&blocks[filledBlockIndex], num_bytes, 1, ofp); 
		//I changed this
        // If the remaining number of bytes we need to copy is less than BLOCK_SIZE then
        // only copy the amount that remains. If we copied BLOCK_SIZE number of bytes we'd
        // end up with garbage at the end of the file.
       /* if( copy_size < BLOCK_SIZE )
        {
            num_bytes = copy_size;
        }
        else
        {
            num_bytes = BLOCK_SIZE;
        } */
		
		

        // Write num_bytes number of bytes from our data array into our output file.
       // fwrite( &blocks[block_index], num_bytes, 1, ofp );           //-

        // Reduce the amount of bytes remaining to copy, increase the offset into the file
        // and increment the block_index to move us to the next data block.
        copy_size -= num_bytes;
        offset    += num_bytes;
        block_index ++;

        // Since we've copied from the point pointed to by our current file pointer, increment
        // offset number of bytes so we will be ready to copy to the next area of our output file.
        fseek( ofp, offset, SEEK_SET );
    }

    // Close the output file, we're done.
    fclose( ofp );
}


//******************************************************************************************************************

//function to gi9ve details of all the existing files in the FS
void list()
{
    int i;
    int got =0;
	if(fd == NULL)
	{
		printf("List: no files found");
	}
	
    for(i=0; i< NUM_FILES; i++)
    {
    
        if(dir[i].valid)
        {
            got =1;
            printf("%s %d \t", dir[i].filename, dir[i].inode);
            df1();
            time_t t = time(NULL);
            struct tm *tm = localtime(&t);
            char s[64];
            assert(strftime(s, sizeof(s), "%c", tm));
            printf("%s\n", s);


        }

    }
	if (got==0)
	{
		
		printf("List: No Files found\n");
	}
}

//open the File System to perform operations
void openFile(char * filename)
{
    fd = fopen(filename, "r");
    fread(&blocks[0], NUM_BLOCKS,BLOCK_SIZE, fd);
    if (fd == NULL)
    {
        //fread(&blocks[0], BLOCK_SIZE,NUM_BLOCKS, fd);
        printf("File does not exist 1\n");
        return;
    }
	printf(" Opening file !\n");
    fclose(fd);
  
}


//function to close the file which also writes the FS
void closeFile()     
{
    fd = fopen(forclose, "w");
    if (fd == NULL)
    {
        printf("File is not opened \n");
        return;
    }

    fwrite( &blocks[0], NUM_BLOCKS, BLOCK_SIZE, fd);

    fclose( fd );
    printf("File System closed !\n");
}

//delete function to delete existing file inside FS
void delete(char * delfilename)

{
	
	int i;
	for( i=0; i< NUM_BLOCKS;  i++)
	{
		if(strcmp(dir[i].filename, delfilename)==0)
	 {
		
		 printf("Lets delete this file: %p\n", delfilename);
		 dir[i].valid    =  0; 
		 dir[i].inode    = -1;
		 freeBlockList[i]=  1;
		 freeInodeList[i] = 1;
	 }


	}
    //return ret;
	//fclose(fd);
	
}
//function to create File System with all the initializations
void createfs( char * filename)
{
    fd = fopen( filename, "w" );

    memset( &blocks[0], 0, NUM_BLOCKS * BLOCK_SIZE);

    initializeDirectory();   //Initializations
    initializeBlockList();
    initializeInodeList();
    initializeInodes();

    fwrite( &blocks[0], NUM_BLOCKS, BLOCK_SIZE, fd);

    fclose( fd );
    printf("File system created successfully !\n");
}


int main ()
{
    dir             = (struct Directory_Entry *)&blocks[0]; //directory points to address of block zero ,
    inodes          = (struct Inode*) &blocks[7];
    freeInodeList   = (uint8_t *)&blocks[5];
    freeBlockList   = (uint8_t *)&blocks[6];

    char * cmd_str = (char*) malloc( MAX_COMMAND_SIZE );

    while( 1 )
    {
        // Print out the mfs prompt
        printf ("mfs> ");

        // Read the command from the commandline.  The
        // maximum command that will be read is MAX_COMMAND_SIZE
        // This while command will wait here until the user
        // inputs something since fgets returns NULL when there
        // is no input
        while( !fgets (cmd_str, MAX_COMMAND_SIZE, stdin) );

        /* Parse input */
        char *token[MAX_NUM_ARGUMENTS];

        int   token_count = 0;

        // Pointer to point to the token
        // parsed by strsep
        char *arg_ptr;

        char *working_str  = strdup( cmd_str );

        // we are going to move the working_str pointer so
        // keep track of its original value so we can deallocate
        // the correct amount at the end
        char *working_root = working_str;

        // Tokenize the input stringswith whitespace used as the delimiter
        while ( ( (arg_ptr = strsep(&working_str, WHITESPACE ) ) != NULL) &&
                (cmd_str !=0))
        {
            token[token_count] = strndup( arg_ptr, MAX_COMMAND_SIZE );
            if( strlen( token[token_count] ) == 0 )
            {
                token[token_count] = NULL;
            }
            token_count++;
        }

        // Now print the tokenized input as a debug check

        int token_index  = 0;
        int char_index=0;
        /*  for(token_index=0; token_index < token_count-1; token_index ++ )
          {
            printf("token[%d] = %s\n", token_index, token[token_index] );
          }*/
        if (strcmp(token[0],"createfs") == 0)   //comapring ip string to call the appropriate function
        {
            createfs(token[1]);
        }

        else if (strcmp(token[0],"put") == 0)
        {
            put(token[1]);
        }
        else if (strcmp(token[0],"open") == 0)
        {
            openFile(token[1]);
            forclose = token[1];
        }
        else if (strcmp(token[0],"close") == 0)
        {
            closeFile(token[1]);
        }
        else if (strcmp(token[0],"list") == 0)
        {
            list();
        }
        else if (strcmp(token[0],"delete") == 0)
        {
            delete(token[1]);
        }
        else if (strcmp(token[0],"df") == 0)
        {
            df();
        }

        else if (strcmp(token[0],"quit") == 0)
        {
            return 1;
        }
		 else if (strcmp(token[0],"get") == 0)
        {
            get(token[1], token[2]);
        }
        else
            {
            printf("Invalid command !\n");
            }
        free( working_root );
    }
}
