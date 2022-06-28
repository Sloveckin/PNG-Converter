#include "header.h"

#if defined(ZLIB)
	#include <zlib.h>
#elif defined(LIBDEFLATE)
	#include <libdeflate.h>
#elif defined(ISAL)
	#include <include/igzip_lib.h>
#endif

int main(int args, char* argv[])
{
	if (args != countArguments)
	{
		fprintf(stderr, INPUT_ERROR);
		return ERROR_INVALID_PARAMETER;
	}

	FILE* inputFile = fopen(argv[1], "rb");
	if (inputFile == NULL)
	{
		fprintf(stderr, FILE_NOT_FOUND);
		return ERROR_FILE_NOT_FOUND;
	}

	data_Struct dataPNG = { 0, 0, 0, 0 };

	int checkRes = workWithFile(inputFile, &dataPNG);
	if (checkRes)
	{
		fclose(inputFile);
		free(dataPNG.data);
		return checkRes;
	}

	int finalRes = convert(&dataPNG, argv[2]);
	if (finalRes)
	{
		free(dataPNG.data);
		fclose(inputFile);
		return finalRes;
	}

	free(dataPNG.data);
	fclose(inputFile);
	return 0;
}

int workWithFile(FILE* file, data_Struct* dataStr)
{
	int ch1 = checkSignaturePng(file);
	if (ch1)
	{
		return ch1;
	}

	int ch2 = checkIHDR(file, dataStr);
	if (ch2)
	{
		return ch2;
	}

	int ch3 = findData(file, dataStr);
	if (ch3)
	{
		return ch3;
	}

	return 0;
}

int findData(FILE* file, data_Struct* dataStr)
{
	bool wasIDAT = false;
	bool wasIEND = false;
	bool wasSkipped = false;
	while (true)
	{
		unsigned char const IHDR[4] = { 0x49, 0x44, 0x41, 0x54 };
		unsigned char const IEND[4] = { 0x49, 0x45, 0x4E, 0x44 };
		unsigned char const PLTE[4] = { 0x50, 0x4C, 0x54, 0x45 };
		unsigned char partLength[4];
		size_t t = fread(partLength, sizeof(unsigned char), 4, file);
		if (t != 4)
		{
			fprintf(stderr, ERROR_FREAD);
			return ERROR_MEMORY;
		}

		int dataLen = charToNumber(partLength, 0, 1, 2, 3);
		unsigned char nameOfChunk[4];
		size_t r = fread(nameOfChunk, sizeof(unsigned char), 4, file);
		if (r == 0)
		{
			break;
		}

		if (r != 4)
		{
			fprintf(stderr, ERROR_FREAD);
			return ERROR_MEMORY;
		}

		bool isPLTE = checkChunk(nameOfChunk, PLTE, 4);
		if (isPLTE && dataStr->typeColor == 0)
		{
			fprintf(stderr, PLTE_WITH_COLOR_ZERO);
			return ERROR_INVALID_DATA;
		}

		bool isData = checkChunk(nameOfChunk, IHDR, 4);
		if (isData && wasSkipped)
		{
			fprintf(stderr, IDAT_BLOCK);
			return ERROR_INVALID_DATA;
		}
		if (isData)
		{
			wasIDAT = true;
			int checkRes = readData(file, dataStr, dataLen);
			if (checkRes)
			{
				return checkRes;
			}
			continue;
		}
		bool isEnd = checkChunk(nameOfChunk, IEND, 4);
		if (isEnd)
		{
			wasIEND = true;
			int ch = fseek(file, 4, SEEK_CUR);
			if (ch)
			{
				fprintf(stderr, ERROR_FSEEK);
				return ERROR_MEMORY;
			}
			break;
		}
		int ch = fseek(file, dataLen + 4, SEEK_CUR);
		if (ch)
		{
			fprintf(stderr, ERROR_FSEEK);
			return ERROR_MEMORY;
		}
		if (wasIDAT)
		{
			wasSkipped = true;
		}
	}

	if (wasIDAT == false || wasIEND == false)
	{
		fprintf(stderr, NO_IMP_CHUNK);
		return ERROR_INVALID_DATA;
	}
	unsigned char byff[1];
	size_t t = fread(byff, sizeof(unsigned char), 1, file);
	if (t != 0)
	{
		fprintf(stderr, CHUNK_AFTER_IEND);
		return ERROR_INVALID_DATA;
	}
	return 0;
}

int checkSignaturePng(FILE* file)
{
	unsigned char sign[8] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };
	unsigned char bytes[8];
	fread(bytes, sizeof(unsigned char), 8, file);
	bool checkRes = checkChunk(bytes, sign, 8);
	if (!checkRes)
	{
		fprintf(stderr, INCORRECT_FILE);
		return ERROR_INVALID_DATA;
	}
	return 0;
}

int checkIHDR(FILE* file, data_Struct* dataStr)
{
	unsigned const char IHDR[8] = { 0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52 };
	unsigned char bytes[17];
	size_t t = fread(bytes, sizeof(unsigned char), 8, file);
	if (t < 8)
	{
		fprintf(stderr, ERROR_FREAD);
		return ERROR_MEMORY;
	}

	bool res = checkChunk(bytes, IHDR, 8);
	if (!res)
	{
		fprintf(stderr, NO_IHDR);
		return ERROR_INVALID_DATA;
	}
	size_t check = fread(bytes, sizeof(unsigned char), 17, file);

	if (check < 17)
	{
		fprintf(stderr, ERROR_FREAD);
		return ERROR_MEMORY;
	}

	dataStr->width = charToNumber(bytes, 0, 1, 2, 3);

	if (dataStr->width == 0)
	{
		fprintf(stderr, INCORRECT_WIDTH);
		return ERROR_INVALID_DATA;
	}

	dataStr->length = charToNumber(bytes, 4, 5, 6, 7);
	if (dataStr->length == 0)
	{
		fprintf(stderr, INCORRECT_LENGTH);
		return ERROR_INVALID_DATA;
	}

	if (bytes[8] != 8)	  /// Проверяем глубину
	{
		fprintf(stderr, INCORRECT_DEPTH);
		return ERROR_INVALID_DATA;
	}

	if (bytes[10])	  /// Проверяем метод сжатия (должен быть всегда 0, по спецификации)
	{
		fprintf(stderr, INCORRECT_COMPRESSION);
		return ERROR_INVALID_DATA;
	}
	dataStr->typeColor = bytes[9];
	if (dataStr->typeColor != 0 && dataStr->typeColor != 2)
	{
		fprintf(stderr, INCORRECT_COLOR_TYPE);
		return ERROR_INVALID_DATA;
	}
	return 0;
}

int charToNumber(const unsigned char* array, int i0, int i1, int i2, int i3)
{
	const int t = 16 * 16;
	int res = ((array[i0] * t + array[i1]) * t + array[i2]) * t + array[i3];
	return res;
}

int readData(FILE* file, data_Struct* dataStr, int dataLen)
{
	unsigned char* akk = malloc(sizeof(unsigned char) * dataLen + 4);
	if (akk == NULL)
	{
		fprintf(stderr, errorMemory);
		return ERROR_NOT_ENOUGH_MEMORY;
	}

	size_t check = fread(akk, sizeof(unsigned char), dataLen + 4, file);
	if (check < dataLen + 4)
	{
		fprintf(stderr, ERROR_FREAD);
		return ERROR_MEMORY;
	}
	unsigned char* h = realloc(dataStr->data, sizeof(unsigned char) * (dataStr->dataSize + dataLen));

	if (h == NULL)
	{
		free(akk);
		fprintf(stderr, errorMemory);
		return ERROR_NOT_ENOUGH_MEMORY;
	}

	dataStr->data = h;

	for (int i = 0; i < dataLen; i++)
	{
		(dataStr->data)[i + dataStr->dataSize] = akk[i];
	}
	dataStr->dataSize += dataLen;
	free(akk);
	return 0;
}

bool checkChunk(const unsigned char* buffer, const unsigned char* chunkName, int n)
{
	for (int i = 0; i < n; i++)
	{
		if (buffer[i] != chunkName[i])
		{
			return false;
		}
	}
	return true;
}

int convert(data_Struct* dataStr, char* pathOut)
{
	unsigned char* hlp;
	size_t ss = (dataStr->typeColor + 1) * dataStr->length * dataStr->width + dataStr->length;
	hlp = malloc(sizeof(unsigned char) * ss);
	if (hlp == NULL)
	{
		fprintf(stderr, errorMemory);
		return ERROR_NOT_ENOUGH_MEMORY;
	}
#if defined(ZLIB)
	uLong s = (uLong)ss;
	int res = uncompress((Bytef*)hlp, &s, (Bytef*)dataStr->data, (uLong)dataStr->dataSize);
	if (res)
	{
		free(hlp);
		fprintf(stderr, ERROR_LIB);
		return res;
	}
#elif defined(LIBDEFLATE)
	int res = libdeflate_zlib_decompress(libdeflate_alloc_decompressor(), dataStr->data, dataStr->dataSize, hlp, ss, &ss);
	if (res != LIBDEFLATE_SUCCESS)
	{
		free(hlp);
		fprintf(stderr, ERROR_LIB);
		return res;
	}
#elif defined(ISAL)
	struct inflate_state state;
	state.next_in = dataStr->data;
	state.avail_in = dataStr->dataSize;
	state.next_out = hlp;
	state.avail_out = ss;
	state.crc_flag = ISAL_ZLIB;
	int res = isal_inflate_stateless(&state);
	if (res != ISAL_DECOMP_OK)
	{
		free(hlp);
		fprintf(stderr, ERROR_LIB);
		return res;
	}
#endif
	unsigned char* arr = malloc(sizeof(unsigned char) * ss - dataStr->length);
	if (arr == NULL)
	{
		free(hlp);
		fprintf(stderr, errorMemory);
		return ERROR_NOT_ENOUGH_MEMORY;
	}
	int totalIndex = 0;
	int p;

	if (dataStr->typeColor == 2)
	{
		int err1 = filterRGB(hlp, arr, *dataStr, &totalIndex);
		if (err1)
		{
			free(arr);
			free(hlp);
			return err1;
		}
		p = 6;
	}
	else
	{
		int err1 = filterGrey(hlp, arr, *dataStr, &totalIndex);
		if (err1)
		{
			free(arr);
			free(hlp);
			return err1;
		}
		p = 5;
	}

	FILE* out = fopen(pathOut, "wb");
	if (!out)
	{
		free(hlp);
		free(arr);
		fprintf(stderr, FILE_NOT_FOUND);
		return ERROR_FILE_NOT_FOUND;
	}

	fprintf(out, "P%i\n%i %i\n255\n", p, dataStr->width, dataStr->length);
	fwrite(arr, sizeof(unsigned char), totalIndex, out);
	free(hlp);
	free(arr);
	fclose(out);
	return 0;
}

int paethPredictor(int a, int b, int c)
{
	int p = a + b - c;
	int pa = abs(p - a);
	int pb = abs(p - b);
	int pc = abs(p - c);

	if (pa <= pb && pa <= pc)
	{
		return a;
	}
	else if (pb <= pc)
	{
		return b;
	}
	return c;
}

int filterRGB(unsigned char* hlp, unsigned char* arr, data_Struct dataStr, int* totalIndex)
{
	*totalIndex = 0;
	int t = 0;
	int step = dataStr.width * 3 + 1;
	for (int i = 0; i < dataStr.length; i++)
	{
		unsigned char filter = hlp[t];
		rgb_Pixel a = { 0, 0, 0 };
		for (int j = 1; j < dataStr.width * 3 + 1; j += 3)
		{
			rgb_Pixel x = { hlp[j + t], hlp[j + t + 1], hlp[j + t + 2] };
			rgb_Pixel b = { 0, 0, 0 };
			if (t > 0)
			{
				b.r = hlp[j + t - step];
				b.g = hlp[j + t + 1 - step];
				b.b = hlp[j + t + 2 - step];
			}

			rgb_Pixel c = { 0, 0, 0 };
			if (t > 0 && j > 1)
			{
				c.r = hlp[j + t - step - 3];
				c.g = hlp[j + t - step - 2];
				c.b = hlp[j + t - step - 1];
			}

			if (filter == 1)
			{
				x.r = (x.r + a.r) % 256;
				x.g = (x.g + a.g) % 256;
				x.b = (x.b + a.b) % 256;
			}
			else if (filter == 2)
			{
				x.r = (x.r + b.r) % 256;
				x.g = (x.g + b.g) % 256;
				x.b = (x.b + b.b) % 256;
			}
			else if (filter == 3)
			{
				x.r = (x.r + ((a.r + b.r) / 2)) % 256;
				x.b = (x.b + ((a.b + b.b) / 2)) % 256;
				x.g = (x.g + ((a.g + b.g) / 2)) % 256;
			}
			else if (filter == 4)
			{
				x.r = (x.r + paethPredictor(a.r, b.r, c.r)) % 256;
				x.g = (x.g + paethPredictor(a.g, b.g, c.g)) % 256;
				x.b = (x.b + paethPredictor(a.b, b.b, c.b)) % 256;
			}
			else
			{
				fprintf(stderr, ERROR_FILTER);
				return ERROR_INVALID_DATA;
			}

			hlp[j + t] = x.r;
			hlp[j + t + 1] = x.g;
			hlp[j + t + 2] = x.b;

			arr[(*totalIndex)++] = x.r;
			arr[(*totalIndex)++] = x.g;
			arr[(*totalIndex)++] = x.b;
			a = x;
		}
		t += step;
	}
	return 0;
}

int filterGrey(unsigned char* hlp, unsigned char* arr, data_Struct dataStr, int* totalIndex)
{
	*totalIndex = 0;
	int t = 0;
	int step = dataStr.width + 1;
	for (int i = 0; i < dataStr.length; i++)
	{
		unsigned char filter = hlp[t];
		int a = 0;
		for (int j = 1; j < dataStr.width + 1; j++)
		{
			int x = hlp[j + t];
			int b = 0;
			if (t > 0)
			{
				b = hlp[j + t - step];
			}

			int c = 0;
			if (t > 0 && j > 1)
			{
				c = hlp[j + t - step - 1];
			}

			if (filter == 1)
			{
				x = (x + a) % 256;
			}
			else if (filter == 2)
			{
				x = (x + b) % 256;
			}
			else if (filter == 3)
			{
				x = (x + ((a + b) / 2)) % 256;
			}
			else if (filter == 4)
			{
				x = (x + paethPredictor(a, b, c)) % 256;
			}
			else
			{
				fprintf(stderr, ERROR_FILTER);
				return ERROR_INVALID_DATA;
			}
			hlp[j + t] = x;
			arr[(*totalIndex)++] = x;
			a = x;
		}
		t += step;
	}
	return 0;
}