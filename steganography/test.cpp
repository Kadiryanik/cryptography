#include <iostream>

#include <fstream>  //files
#include <iomanip>  //setw
#include <unistd.h> //getopt
#include <cstdlib>  //optarg, exit

#include "bmp.h"

using namespace std;

#define FILE_PATH_BINARY "images/binary.bmp"
#define FILE_PATH_GRAY "images/gray.bmp"
#define FILE_PATH_MERGED "images/merged.bmp"
#define FILE_PATH_SECRET "images/secret.bmp"
#define FILE_PATH_VC_FIRST "images/1.bmp"
#define FILE_PATH_VC_SECOND "images/2.bmp"

void usage(const char *);

int main(int argc, char *argv[]){
  int height, width;
  long size;
  ifstream file;

  const char* image = NULL;
  const char* secretImage = NULL;
  const char* mergedImage = NULL;
  const char* toBinaryImage = NULL;
  const char* visualImage = NULL;
  const char* toGrayImage = NULL;

  int secretHeight = 0, secretWidth = 0;
  long secretSize;

  int c;
  while((c = getopt(argc, argv, "c:s:d:w:h:b:g:v:")) != -1) {
    switch(c) {
    case 'c':
      image = optarg;
      break;
    case 's':
      secretImage = optarg;
    case 'd':
      mergedImage = optarg;
      break;
    case 'w':
      secretWidth = atoi(optarg);
      break;
    case 'h':
      secretHeight = atoi(optarg);
      break;
    case 'b':
      toBinaryImage = optarg;
      break;
    case 'g':
      toGrayImage = optarg;
      break;
    case 'v':
      visualImage = optarg;
      break;
    default:
      usage(argv[0]);
      exit(1);
      break;
    }
  }

  srand (time(NULL));

  if(image != NULL && secretImage != NULL){
    file.open(image, ios::in | ios::binary);
    BYTE* buffer = loadBMP(&height, &width, &size, file);
    if(buffer == NULL){
        cout << "Error: buffer null!" << endl;
        return 1;
    }
    file.close();

    file.open(secretImage, ios::in | ios::binary);
    BYTE* secretBuffer = loadBMP(&secretHeight, &secretWidth, &secretSize, file);
    if(secretBuffer == NULL){
        cout << "Error: secretBuffer null!" << endl;
        return 1;
    }
    file.close();

    cout << "secretWidth: " << secretWidth << endl << "secretHeight: " << secretHeight << endl;

    if(height * width / 8 < secretHeight * secretWidth){
        cout << "Secret image too big!" << endl;
        return 1;
    }

    BYTE* ramIntensity = convertBMPToIntensity(buffer, width, height);
    if(ramIntensity == NULL){
        cout << "Error: ramIntensity null, returned in convertBMPToIntensity()!" << endl;
        return 1;
    }

    BYTE* secretRamIntensity = convertBMPToIntensity(secretBuffer, secretWidth, secretHeight);
    if(secretRamIntensity == NULL){
        cout << "Error: secretRamIntensity null, returned in convertBMPToIntensity()!" << endl;
        return 1;
    }

    // process all bytes
    for(long i = 0; i < secretWidth * secretHeight; i++){
        processTheByte(ramIntensity + (i * 8), *(secretRamIntensity + i));
    }

    saveBMP(FILE_PATH_MERGED, height, width, convertIntensityToBMP(ramIntensity, width, height, &size));

    delete[] secretBuffer;
    delete[] secretRamIntensity;
    
    delete[] buffer;
    delete[] ramIntensity;
  } else if(mergedImage != NULL && secretWidth > 0 && secretHeight > 0){
    file.open(mergedImage, ios::in | ios::binary);

    BYTE* buffer = loadBMP(&height, &width, &size, file);
    if(buffer == NULL){
        cout << "Error: buffer null!" << endl;
        return 1;
    }
    file.close();

    BYTE* ramIntensity = convertBMPToIntensity(buffer, width, height);
    if(ramIntensity == NULL){
        cout << "Error: ramIntensity null, returned in convertBMPToIntensity()!" << endl;
        return 1;
    }

    BYTE* secret = decrypteTheSecret(ramIntensity, secretWidth, secretHeight);

    long empty;
    saveBMP(FILE_PATH_SECRET, secretHeight, secretWidth, convertIntensityToBMP(secret, secretWidth, secretHeight, &empty));
    
    delete[] buffer;
    delete[] ramIntensity;
    delete[] secret;
  } else if(toBinaryImage != NULL){
    int height, width;
    long size;
    ifstream file;

    file.open(toBinaryImage, ios::in | ios::binary);

    BYTE* buffer = loadBMP(&height, &width, &size, file);
    file.close();

    if(buffer == NULL){
        cout << "Error: buffer null, returned in LoadBMP()!" << endl;
        return 1;
    }
    
    BYTE* ramIntensity = convertBMPToIntensity(buffer, width, height);

    int tHold = thresHold(ramIntensity, width, height);

    BYTE* binaryImage = new BYTE[width * height];
    for(int i = 0; i < width*height; i++){
        *(binaryImage + i) = *(ramIntensity + i) > tHold ? 255 : 0;
    }
    saveBMP(FILE_PATH_BINARY, height, width, convertIntensityToBMP(binaryImage, width, height, &size));

    delete[] buffer;
    delete[] ramIntensity;
    delete[] binaryImage;
  } else if(visualImage != NULL){
    int height, width;
    long size;
    ifstream file;

    file.open(visualImage, ios::in | ios::binary);

    BYTE* buffer = loadBMP(&height, &width, &size, file);
    file.close();

    if(buffer == NULL){
        cout << "Error: buffer null, returned in LoadBMP()!" << endl;
        return 1;
    }
    
    BYTE* ramIntensity = convertBMPToIntensity(buffer, width, height);

    int secretWidth = width * 2, secretHeight = height * 2;

    BYTE* first = new BYTE[secretWidth * secretHeight];
    BYTE* second = new BYTE[secretWidth * secretHeight];

    getVisualSecrets(ramIntensity, width, height, first, second);

    saveBMP(FILE_PATH_VC_FIRST, secretHeight, secretWidth, convertIntensityToBMP(first, secretWidth, secretHeight, &size));
    saveBMP(FILE_PATH_VC_SECOND, secretHeight, secretWidth, convertIntensityToBMP(second, secretWidth, secretHeight, &size));

    delete[] buffer;
    delete[] ramIntensity;
    delete[] first;
    delete[] second;
  } else if(toGrayImage != NULL){
    int height, width;
    long size;
    ifstream file;

    file.open(toGrayImage, ios::in | ios::binary);

    BYTE* buffer = loadBMP(&height, &width, &size, file);
    file.close();

    if(buffer == NULL){
        cout << "Error: buffer null, returned in LoadBMP()!" << endl;
        return 1;
    }
    
    BYTE* ramIntensity = convertBMPToIntensity(buffer, width, height);

    saveBMP(FILE_PATH_GRAY, height, width, convertIntensityToBMP(ramIntensity, width, height, &size));

    delete[] buffer;
    delete[] ramIntensity;
  } else{
    usage(argv[0]);
    return 1;
  }

  return 0;
}

void usage(const char *name){
  cout << "Usage: " << name << " [-c <*.bmp>] [-s <*.bmp>] [-d <*.bmp>] [-w <secret width>] [-h <secret height>]" << endl;
  cout << "  -c use for crypte, give image path with this" << endl
    << "  -s secret image path" << endl
    << "  -d use for decrypte, give image path with this" << endl
    << "  -w secret image width (>0)" << endl
    << "  -h secret image height (>0)" << endl
    << "  -b convert input image to binary" << endl
    << "  -g convert input image to gray level" << endl
    << "  -v visual crypto, give image path with this" << endl
    << "Examples:" << endl
    << " Crypte: -c image.bmp -s secret.bmp" << endl
    << " Decrypte: -d image.bmp -w 32 -h 32" << endl
    << " Binary: -b image.bmp" << endl
    << " VisualCrypte: -v image.bmp" << endl;
}