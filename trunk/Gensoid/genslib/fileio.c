
#include "app.h"


/*
    Load a normal file, or ZIP/GZ archive.
    Returns NULL if an error occured.
*/
void get_archive_filename(char *filename, char *longfilename)
{
   FILE *fd = NULL;
   unsigned char romname_len=0;
   char buf[1];
   fd = (FILE*)fopen(filename, "rb");
   if(fd)
   {    
     fseek(fd, 26,SEEK_SET);
     if(fread(buf, 1, 1, fd)==1) // read 1 byte - name length
     {
	// now copy name string to te
	romname_len=(unsigned char)buf[0];
	longfilename[0]=romname_len;
	fseek(fd, 30,SEEK_SET);
	fread(longfilename,1,romname_len,fd);
	if(longfilename[romname_len-4] == '.')
	{
	  longfilename[romname_len-1]=0;
	  longfilename[romname_len-2]=0;
	  longfilename[romname_len-3]=0;
	  longfilename[romname_len-4]=0;
	}
     }
     fclose(fd);
    }
}
			
int get_archive_crc(char *filename)
{
   FILE *fd = NULL;
   unsigned char romname_len=0;
   int buf;
   
   fd = (FILE*)fopen(filename, "rb");
   if(fd)
   {    
     fseek(fd, 14,SEEK_SET);
     if(fread((char*)&buf, 1, 4, fd)==4) // read 1 byte - name length
     {
        fclose(fd);
	return(buf);
     }
     else
     {
       fclose(fd);
       return(0);
     }
    }
    
    return(0);
}

int load_archive(char *filename, char *buffer, int *file_size)
{
    int size = 0;
    uint8 *buf = NULL;
	unzFile *fd = NULL;
	unz_file_info info;
	int ret = 0;
	 
	/* Attempt to open the archive */
	fd = unzOpen(filename);
	if(!fd)
	{
		return (0);
	}

	/* Go to first file in archive */
	ret = unzGoToFirstFile(fd);
	if(ret != UNZ_OK)
	{
		unzClose(fd);
		return (0);
	}

	ret = unzGetCurrentFileInfo(fd, &info, NULL, 0, NULL, 0, NULL, 0);
	if(ret != UNZ_OK)
	{
		unzClose(fd);
		return (0);
	}

	/* Open the file for reading */
	ret = unzOpenCurrentFile(fd);
	if(ret != UNZ_OK)
	{
		unzClose(fd);
		return (0);
	}

	/* Allocate file data buffer */
	size = info.uncompressed_size;
	if(size>*file_size)
	{
	   unzClose(fd);
	   return (0);
	}
	buf = buffer;

	/* Read (decompress) the file */
	ret = unzReadCurrentFile(fd, buf, info.uncompressed_size);
	if(ret != info.uncompressed_size)
	{
		//free(buf);
		unzCloseCurrentFile(fd);
		unzClose(fd);
		return (0);
	}

	/* Close the current file */
	ret = unzCloseCurrentFile(fd);
	if(ret != UNZ_OK)
	{
		//free(buf);
		unzClose(fd);
		return (0);
	}

	/* Close the archive */
	ret = unzClose(fd);
	if(ret != UNZ_OK)
	{
		//free(buf);
		return (0);
	}

	/* Update file size and return pointer to file data */
	*file_size = size;
	return (1);
}


/*
    Verifies if a file is a ZIP archive or not.
    Returns: 1= ZIP archive, 0= not a ZIP archive
*/
int check_zip(char *filename)
{
    uint8 buf[2];
    FILE *fd = NULL;
    fd = (FILE*)fopen(filename, "rb");
    if(!fd) return (0);
    fread(buf, 1, 2, fd);
    fclose(fd);
    if(memcmp(buf, "PK", 2) == 0) return (1);
    return (0);
}


/*
    Returns the size of a GZ compressed file.
*/
int gzsize(gzFile *gd)
{
    #define CHUNKSIZE   (0x10000)
    int size = 0, length = 0;
    unsigned char buffer[CHUNKSIZE];
    gzrewind(gd);
    do {
        size = gzread(gd, buffer, CHUNKSIZE);
        if(size <= 0) break;
        length += size;
    } while (!gzeof(gd));
    gzrewind(gd);
    return (length);
    #undef CHUNKSIZE
}

int save_archive(char *filename, char *buffer, int size)
{
    uint8 *buf = NULL;
    zipFile *fd = NULL;
    int ret = 0;
    fd=zipOpen(filename,0);
    if(!fd)
    {
       return (0);
    }
    ret=zipOpenNewFileInZip(fd,"GENSOID",
			    NULL,
                            NULL,0,
			    NULL,0,
			    NULL,
			    Z_DEFLATED,
			    Z_DEFAULT_COMPRESSION);
			    
    if(ret != ZIP_OK)
    {
       zipClose(fd,NULL);
       return (0);    
    }
	
    ret=zipWriteInFileInZip(fd,buffer,size);
    if(ret != ZIP_OK)
    {
      zipCloseFileInZip(fd);
      zipClose(fd,NULL);
      return (0);
    }
	
    ret=zipCloseFileInZip(fd);
    if(ret != ZIP_OK)
    {
      zipClose(fd,NULL);
      return (0);
    }
	
    ret=zipClose(fd,NULL);
    if(ret != ZIP_OK)
    {
      return (0);
    }
	
    return(1);
}
