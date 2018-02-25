#include <iostream>
#include <fstream>
#include <math.h>       // for fabs
#include <cstring>      // for memset
#include <iomanip>      // for setw
#include <ctime>
#include <cstdlib>

#include "bmp.h"

using namespace std;

#define DEBUG 1
#define LOAD_ANYWAY 0

#if DEBUG
#define dout cout
#else
#define dout 0 && cout
#endif

/*--------------------------------------------------------------------------------------*/
BYTE* loadBMP(int* height, int* width, long* size, ifstream &file){
    // declare bitmap structures
    BITMAPFILEHEADER bmpHead;
    BITMAPINFOHEADER bmpInfo;

    // value to be used in ReadFile funcs
    DWORD bytesread;

    // open file to read from
    if (NULL == file)
        return NULL; // coudn't open file

    // read file header
    if (!file.read(reinterpret_cast<char*>(&bmpHead), sizeof(BITMAPFILEHEADER))){
        dout << "Error: file.read header" << endl;
        file.close();
        return NULL;
    }
    //read bitmap info
    if (!file.read(reinterpret_cast<char*>(&bmpInfo), sizeof(BITMAPINFOHEADER))){
        dout << "Error: file.read info" << endl;
        file.close();
        return NULL;
    }
#if !LOAD_ANYWAY
    // check if file is actually a bmp
    if (bmpHead.bfType != 0x4D42){ //'MB'
        dout << "Error: file type" << endl;
        file.close();
        return NULL;
    }
#endif
    // get image measurements
    *width = bmpInfo.biWidth;
    *height = fabs(bmpInfo.biHeight);
#if !LOAD_ANYWAY
    // check if bmp is uncompressed
    if (bmpInfo.biCompression != 0){
        dout << "Error: biCompression != 0" << endl;
        file.close();
        return NULL;
    }
    // check if we have 24 bit bmp
    if (bmpInfo.biBitCount != 24) {
        dout << "Error: bitCount != 24 bit" << endl;
        file.close();
        return NULL;
    }
#endif
    // create buffer to hold the data
    *size = bmpHead.bfSize - bmpHead.bfOffBits;
    BYTE* Buffer = new BYTE[*size];
    // move file pointer to start of bitmap data
    file.seekg(bmpHead.bfOffBits, ios::beg);

    // read bmp data
    if (!file.read(reinterpret_cast<char*>(Buffer), *size))  {
        dout << "Error: file.read data" << endl;
        delete[] Buffer;
        file.close();
        return NULL;
    }
    // everything successful here: close file and return buffer
    file.close();

#if DEBUG > 9
    dout << "loadBMP headers" << endl;
    printStructers(bmpHead, bmpInfo);
#endif
    return Buffer;
}
/*--------------------------------------------------------------------------------------*/
int saveBMP(const char *filename, int height, int width, BYTE *data){
    BITMAPFILEHEADER bmpHead;
    BITMAPINFOHEADER bmpInfo;
    int size = width * height * 3;

    // andinitialize them to zero
    memset(&bmpHead, 0, sizeof (BITMAPFILEHEADER));
    memset(&bmpInfo, 0, sizeof (BITMAPINFOHEADER));

    // fill headers
    bmpHead.bfType = 0x4D42; // 'MB'
    bmpHead.bfSize= size + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER); // size + head + info no quad    
    bmpHead.bfReserved1 = bmpHead.bfReserved2 = 0;
    bmpHead.bfOffBits = bmpHead.bfSize - size; //equals 0x36 (54)
    // finish the initial of head

    bmpInfo.biSize = sizeof(BITMAPINFOHEADER);
    bmpInfo.biWidth = width;
    bmpInfo.biHeight = height;
    bmpInfo.biPlanes = 1;
    bmpInfo.biBitCount = 24;            // bit(s) per pixel, 24 is true color
    bmpInfo.biCompression = 0;          //BI_RGB
    bmpInfo.biSizeImage = 0;
    bmpInfo.biXPelsPerMeter = 0x0ec4;   // paint and PSP use this values
    bmpInfo.biYPelsPerMeter = 0x0ec4;
    bmpInfo.biClrUsed = 0;
    bmpInfo.biClrImportant = 0;
    // finish the initial of infohead

    ofstream file;
    file.open(filename, ios::out | ios::binary | ios::app);
    
    if(!file.is_open()) return 0;

    if(!file.write(reinterpret_cast<char*>(&bmpHead), sizeof(BITMAPFILEHEADER))){
        file.close();
        return 0;
    }
    if(!file.write(reinterpret_cast<char*>(&bmpInfo), sizeof(BITMAPINFOHEADER))){
        file.close();
        return 0;
    }
    if(!file.write(reinterpret_cast<char*>(data), size)){
        file.close();
        return 0;
    }

    file.close();

#if DEBUG > 9
    dout << endl << "saveBMP headers" << endl;
    printStructers(bmpHead, bmpInfo);
#endif
    return 1;
}
/*--------------------------------------------------------------------------------------*/
BYTE* convertBMPToIntensity(BYTE* Buffer, int width, int height){
    // first make sure the parameters are valid
    if ((NULL == Buffer) || (width == 0) || (height == 0))
        return NULL;

    // find the number of padding bytes

    int padding = 0;
    int scanlinebytes = width * 3;
    while ((scanlinebytes + padding) % 4 != 0)     // DWORD = 4 bytes
        padding++;
    // get the padded scanline width
    int psw = scanlinebytes + padding;

    // create new buffer
    BYTE* newbuf = new BYTE[width*height];

    // now we loop trough all bytes of the original buffer, 
    // swap the R and B bytes and the scanlines
    long bufpos = 0;
    long newpos = 0;
    for (int row = 0; row < height; row++){
        for (int column = 0; column < width; column++){
              newpos = row * width + column;
              bufpos = (height - row - 1) * psw + column * 3;
              //newbuf[newpos] = BYTE(0.11*Buffer[bufpos + 2] + 0.59*Buffer[bufpos + 1] + 0.3*Buffer[bufpos]);
              newbuf[newpos] = BYTE((Buffer[bufpos + 2] + Buffer[bufpos + 1] + Buffer[bufpos])/3);
          }
    }
    
    return newbuf;
}
/*--------------------------------------------------------------------------------------*/
BYTE* convertIntensityToBMP(BYTE* Buffer, int width, int height, long* newsize){
    // first make sure the parameters are valid
    if ((NULL == Buffer) || (width == 0) || (height == 0))
        return NULL;

    // now we have to find with how many bytes
    // we have to pad for the next DWORD boundary   

    int padding = 0;
    int scanlinebytes = width * 3;
    while ((scanlinebytes + padding) % 4 != 0)     // DWORD = 4 bytes
        padding++;
    // get the padded scanline width
    int psw = scanlinebytes + padding;
    // we can already store the size of the new padded buffer
    *newsize = height * psw;

    // and create new buffer
    BYTE* newbuf = new BYTE[*newsize];

    // fill the buffer with zero bytes then we dont have to add
    // extra padding zero bytes later on
    memset(newbuf, 0, *newsize);

    // now we loop trough all bytes of the original buffer, 
    // swap the R and B bytes and the scanlines
    long bufpos = 0;
    long newpos = 0;
    for (int row = 0; row < height; row++){
        for (int column = 0; column < width; column++){
            bufpos = row * width + column;                  // position in original buffer
            newpos = (height - row - 1) * psw + column * 3; // position in padded buffer
            newbuf[newpos] = Buffer[bufpos];                //  blue
            newbuf[newpos + 1] = Buffer[bufpos];            //  green
            newbuf[newpos + 2] = Buffer[bufpos];            //  red
        }
    }
    return newbuf;
}
/*--------------------------------------------------------------------------------------*/
static void setNewPixelValue(BYTE* pixelValueLeft, BYTE *pixelValueRight, uint8_t newValue){
    // (a + 2b) mod 5
    uint8_t mod = ((uint8_t)(*pixelValueLeft) + 2 * (uint8_t)(*pixelValueRight)) % 5;

    if(mod == newValue){
        return;
    }

    uint8_t left = (uint8_t)(*pixelValueLeft);
    uint8_t right = (uint8_t)(*pixelValueRight);

    if(mod > newValue){
        if(newValue + 1 == mod){
            (left == 0) ? (left += 4) : left--;
        } else if(newValue + 2 == mod){
            (right == 0) ? (right += 4) : right--;
        } else if(newValue + 3 == mod){
            (left < 3) ? (left += 2) : (left -= 3);
        } else if(newValue + 4 == mod){
            (right < 2) ? (right += 3) : right -= 2;
        } else{
            cout << "Something wrong!" << endl;
        }
    } else{
        if(newValue == mod + 1){
            (left < 255) ? (left += 1) : left -= 4;
        } else if(newValue == mod + 2){
            (right < 255) ? (right += 1) : right -= 4;
        } else if(newValue == mod + 3){
            (left < 253) ? (left += 3) : left -= 2;
        } else if(newValue == mod + 4){
            (right < 254) ? (right += 2) : right -= 3;
        } else{
            cout << "Something wrong!" << endl;
        }
    }

    *pixelValueLeft = (BYTE)left;
    *pixelValueRight = (BYTE)right;
}
/*--------------------------------------------------------------------------------------*/
void processTheByte(BYTE* start, BYTE secretByte){
    uint8_t bytesInFiveBase[4] = { 0 };
    uint8_t theByte = (uint8_t)secretByte;

    for(int i = 3; i >= 0; i--){
        bytesInFiveBase[i] = theByte % 5;
        theByte /= 5;
    }

    for(int i = 0; i < 8; i += 2){
        setNewPixelValue(start + i, start + i + 1, bytesInFiveBase[i / 2]);
    }
}
/*--------------------------------------------------------------------------------------*/
static BYTE getPixelValue(BYTE* start){
    uint8_t bytesInFiveBase[4] = { 0 };

    for(int i = 0; i < 8; i += 2){
        // (a + 2b) mod 5
        uint8_t mod = ((uint8_t)(*(start + i)) + 2 * (uint8_t)(*(start + i + 1))) % 5;
        
        bytesInFiveBase[i / 2] = mod;
    }

    return (BYTE)(bytesInFiveBase[0] * 125 + bytesInFiveBase[1] * 25 +
                  bytesInFiveBase[2] * 5 + bytesInFiveBase[3]);
}
/*--------------------------------------------------------------------------------------*/
BYTE* decrypteTheSecret(BYTE* merged, int width, int height){
    BYTE* secret = new BYTE[width * height];
    int merged_offset = 0;

    for(int i = 0; i < height; i++){
        for(int j = 0; j < width; j++){
            secret[i * width + j] = getPixelValue(merged + merged_offset);
            merged_offset += 8;
        }
    }

    return secret;
}
/*--------------------------------------------------------------------------------------*/
int thresHold(const BYTE* const ramIntensity, int width, int height){
    int* tHold = new int[ 256 ];
    int i, pixelTotal, thValue;
    DWORD sum = 0;
    memset(tHold, 0, 256 * sizeof(int));

    for(i = 0; i < width * height; i++){
            tHold[(int)*(ramIntensity + i)]++;
    }
    double total = 0;
    for (i = 0; i < 256; i++){
        sum += i * (*(tHold + i));
        if(*(tHold + i)) total++;
    }
    dout << endl;
    int done = 1;
    int T1 = 0, T2 = 255;
    while(done){
        DWORD sumT1 = 0, sumT2 = 0;
        float sumT1i = 0, sumT2i = 0;

        for(i = 0; i < 256; i++){
            if(fabs(T1 - i) < fabs(T2 - i)){
                sumT1 += i * (*(tHold + i));
                sumT1i += *(tHold + i);
            } else{
                sumT2 += i * (*(tHold + i));
                sumT2i += *(tHold + i);
            }
        }
        if(sumT1i == 0){
            sumT1i = 1;
        }
        if(sumT2i == 0){
            sumT2i = 1;
        }
        int T1u = sumT1/sumT1i;
        int T2u = sumT2/sumT2i;
        float epsilon = 2;
        if((fabs(T1 - T1u) < epsilon && fabs(T2 - T2u) < epsilon) || \
             fabs(T1 - T2u) < epsilon && fabs(T2 - T1u) < epsilon){
            dout << "T1 = " << T1 << endl;
            dout << "T2 = " << T2 << endl;
            done = 0;
        }else{
            T1 = T1u;
            T2 = T2u;
        }
    }
#if 0
    int min = T1;
    for(int i = (T1 < T2 ? T1 : T2); i < (T1 < T2 ? T2 : T1); i++){
        if(*(tHold + min) > *(tHold + i)) min = i;
    }
    
    dout << "thValue = " << min << endl;
    return min;
#else
    return (T1 + T2) / 2 - 10;
#endif
}
/*--------------------------------------------------------------------------------------*/
void getVisualSecrets(const BYTE* const ramIntensity, int width, int height, BYTE* secretFirstImage, BYTE* secretSecondImage){
    int secretWidth = width * 2, secretHeight = height * 2;
    
    BYTE gray_masks[4 * 5];
    gray_masks[0] = 0; gray_masks[1] = 255; gray_masks[2] = 255; gray_masks[3] = 0;
    gray_masks[4] = 255; gray_masks[5] = 0; gray_masks[6] = 0; gray_masks[7] = 255;
    gray_masks[8] = 255; gray_masks[9] = 0; gray_masks[10] = 255; gray_masks[11] = 0;
    gray_masks[12] = 255; gray_masks[13] = 255; gray_masks[14] = 0; gray_masks[15] = 0;
    gray_masks[16] = 0; gray_masks[17] = 0; gray_masks[18] = 255; gray_masks[19] = 255;
    

    for(int i = 0, row = 0; i < height; i++, row += 2){
        for(int j = 0, column = 0; j < width; j++, column += 2){
            int index = (rand() % 5) * 4;

            if(*(ramIntensity + i * width + j) == 0){
                *(secretFirstImage + row * secretWidth + column) = gray_masks[index];
                *(secretFirstImage + row * secretWidth + column + 1) = gray_masks[index + 1];
                *(secretFirstImage + (row + 1) * secretWidth + column) = gray_masks[index + 2];
                *(secretFirstImage + (row + 1) * secretWidth + column + 1) = gray_masks[index + 3];

                *(secretSecondImage + row * secretWidth + column) = gray_masks[index] ? 0 : 255;
                *(secretSecondImage + row * secretWidth + column + 1) = gray_masks[index + 1] ? 0 : 255;
                *(secretSecondImage + (row + 1) * secretWidth + column) = gray_masks[index + 2] ? 0 : 255;
                *(secretSecondImage + (row + 1) * secretWidth + column + 1) = gray_masks[index + 3] ? 0 : 255;
            } else{
                *(secretFirstImage + row * secretWidth + column) = gray_masks[index];
                *(secretFirstImage + row * secretWidth + column + 1) = gray_masks[index + 1];
                *(secretFirstImage + (row + 1) * secretWidth + column) = gray_masks[index + 2];
                *(secretFirstImage + (row + 1) * secretWidth + column + 1) = gray_masks[index + 3];

                *(secretSecondImage + row * secretWidth + column) = gray_masks[index];
                *(secretSecondImage + row * secretWidth + column + 1) = gray_masks[index + 1];
                *(secretSecondImage + (row + 1) * secretWidth + column) = gray_masks[index + 2];
                *(secretSecondImage + (row + 1) * secretWidth + column + 1) = gray_masks[index + 3];
            }
        }
    }
}
