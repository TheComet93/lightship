#include "util/file.h"
#include "util/memory.h"
#include "framework/log.h"
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>

/* ------------------------------------------------------------------------- */
uint32_t
file_load_into_memory(const char* file_name, void** buffer, file_opts_e opts)
{
    FILE* fp;
    int fd;
    struct stat stbuf;
    off_t buffer_size;
    
    /* get file pointer */
    if(opts & FILE_BINARY)
        fp = fopen(file_name, "rb");
    else
        fp = fopen(file_name, "r");
    if(!fp)
    {
        fprintf(stderr, "fopen() failed for file \"%s\"\n", file_name);
        return 0;
    }
    
    /* get file descriptor and stat file */
    fd = open(file_name, O_RDONLY);
    if(fd == -1)
    {
        fprintf(stderr, "open() failed for file \"%s\"\n", file_name);
        return 0;
    }
    if(fstat(fd, &stbuf) != 0)
    {
        fprintf(stderr, "fstat() failed for file \"%s\"\n", file_name);
        return 0;
    }
    
    /* ensure file is regular file, as stbuf.st_size is only valid for regular files */
    if(!S_ISREG(stbuf.st_mode))
    {
        fprintf(stderr, "File \"%s\" is not a regular file\n", file_name);
        return 0;
    }
    
    /* compute file size */
    buffer_size = stbuf.st_size;
    
    /* allocate buffer to copy into */
    if(opts & FILE_BINARY)
        *buffer = MALLOC(buffer_size);
    else
        *buffer = MALLOC(buffer_size + sizeof(char));
    if(*buffer == NULL)
    {
        fprintf(stderr, "malloc() failed in file_load_into_memory() -- not enough memory");
        return 0;
    }
    
    /* copy file into buffer */
    if((intptr_t)fread(*buffer, 1, buffer_size, fp) != buffer_size)
    {
        fprintf(stderr, "fread() failed for file \"%s\"\n", file_name);
        return 0;
    }
    fclose(fp);

    /* append null terminator if not in binary mode */
    if((opts & FILE_BINARY) == 0)
        ((char*)(*buffer))[buffer_size] = '\0';
    
    return (uint32_t)buffer_size;
}

/* ------------------------------------------------------------------------- */
void
free_file(void* ptr)
{
    FREE(ptr);
}
