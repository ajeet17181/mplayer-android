//#include <iostream.h>
#include <stdio.h>
#include <string.h>

//using namespace std;

//#pragma pack()
typedef struct tagBITMAPFILEHEADER {
  unsigned short bfType;
  unsigned int bfSize;
  unsigned short int bfReserved1;
  unsigned short int bfReserved2;
  unsigned int bfOffBits;
} BITMAPFILEHEADER;


//#pragma pack()
typedef struct tagBITMAPINFOHEADER {
  unsigned int biSize;
  unsigned int biWidth;
  unsigned int biHeight;
  unsigned short biPlanes;
  unsigned short biBitCount;
  unsigned int biCompression;
  unsigned int biSizeImage;
  unsigned int biXPelsPerMeter;
  unsigned int biYPelsPerMeter;
  unsigned int biClrUsed;
  unsigned int biClrImportant;
} BITMAPINFOHEADER;


int main()
{
            #define MAXW 500
			#define MAXH 500


            FILE *ptrBmpFile, *ptBmpFile;
            int num;
            unsigned char data[320][240][3]; 
		
            memset(data,0,320*240*3);
    


            BITMAPFILEHEADER bMapFileHeader;
            BITMAPINFOHEADER bMapInfoHeader;

            
            ptrBmpFile = fopen("file.bmp","rb");
            
            fseek(ptrBmpFile,0,SEEK_SET);            
            num = fread(&bMapFileHeader,sizeof(BITMAPFILEHEADER),1,ptrBmpFile);
            num = fread(&bMapInfoHeader,sizeof(BITMAPINFOHEADER),1,ptrBmpFile);
          
			 ptBmpFile=fopen("copy.bmp","w");
		fseek(ptBmpFile,0,SEEK_SET);
		
		    num = fwrite(&bMapFileHeader,sizeof(BITMAPFILEHEADER),1,ptBmpFile);
            num = fwrite(&bMapInfoHeader,sizeof(BITMAPINFOHEADER),1,ptBmpFile);
			
            //seek beginning of data in bitmap
            fseek(ptrBmpFile,54,SEEK_SET);
			

            
            //read in bitmap file to data
            fread(data,bMapInfoHeader.biSizeImage,1,ptrBmpFile);
 
            fclose(ptrBmpFile);

            //copy image to "copy.bmp"
           fseek(ptrBmpFile,54,SEEK_SET);
			fwrite(data,bMapInfoHeader.biSizeImage,1,ptBmpFile);
			fclose(ptBmpFile);
			
            return 0;
}

