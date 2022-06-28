#pragma once
#include "errorDefines.h"
#include "return_codes.h"
#include <malloc.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct
{
	int dataSize;
	int width;
	int length;
	int typeColor;
	unsigned char* data;
} data_Struct;

typedef struct
{
	unsigned char r;
	unsigned char g;
	unsigned char b;
} rgb_Pixel;

const int countArguments = 3;

int workWithFile(FILE* file, data_Struct* dataStr);
int checkSignaturePng(FILE* file);
int checkIHDR(FILE* file, data_Struct* dataStr);
int findData(FILE* file, data_Struct* dataStr);
int charToNumber(const unsigned char* array, int i0, int i1, int i2, int i3);
int convert(data_Struct* dataStr, char* pathOut);
int readData(FILE* file, data_Struct* dataStr, int dataLen);
bool checkChunk(const unsigned char* buffer, const unsigned char* chunkName, int n);
int paethPredictor(int a, int b, int c);
int filterRGB(unsigned char* hlp, unsigned char* arr, data_Struct dataStr, int* totalIndex);
int filterGrey(unsigned char* hlp, unsigned char* arr, data_Struct dataStruct, int* totalIndex);