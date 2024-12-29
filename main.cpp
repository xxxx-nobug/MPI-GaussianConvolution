#define _CRT_SECURE_NO_WARNINGS
#include <math.h>
#include <stdio.h>
#include <sys/types.h>
#include <iostream>
#include <fstream>
#include <mpi.h>
#include <string.h>
#include <stdlib.h>
#include <chrono>  // 添加时间测量

#pragma pack(1)

typedef struct BITMAPFILEHEADER {
    uint16_t bfType;
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;
} BITMAPFILEHEADER;

typedef struct BITMAPINFOHEADER {
    uint32_t biSize;
    uint32_t biWidth;
    uint32_t biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    uint32_t biXPelsPerMeter;
    uint32_t biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
} BITMAPINFOHEADER;

using namespace std;

#define BMP_FILE_NAME "dog.bmp"

const int N = 20;
double GsCore[N][N];
unsigned char *pBmpBuf = NULL;  // 读入图像数据的指针
int BmpWidth;    // 图像的宽
int BmpHeight;   // 图像的高

/*******************************************************************************/

void showBmpHead(BITMAPFILEHEADER &pBmpHead) {
    cout << "==========位图文件头==========" << endl;
    cout << "文件头类型:" << pBmpHead.bfType << endl;
    cout << "文件大小:" << pBmpHead.bfSize << endl;
    cout << "保留字_1:" << pBmpHead.bfReserved1 << endl;
    cout << "保留字_2:" << pBmpHead.bfReserved2 << endl;
    cout << "实际位图数据的偏移字节数:" << pBmpHead.bfOffBits << endl << endl;
}

void showBmpInforHead(BITMAPINFOHEADER &pBmpInforHead) {
    cout << "==========位图信息头==========" << endl;
    cout << "结构体的长度:" << pBmpInforHead.biSize << endl;
    cout << "位图宽:" << pBmpInforHead.biWidth << endl;
    cout << "位图高:" << pBmpInforHead.biHeight << endl;
    cout << "biPlanes平面数:" << pBmpInforHead.biPlanes << endl;
    cout << "biBitCount采用颜色位数:" << pBmpInforHead.biBitCount << endl;
    cout << "压缩方式:" << pBmpInforHead.biCompression << endl;
    cout << "biSizeImage实际位图数据占用的字节数:" << pBmpInforHead.biSizeImage << endl;
    cout << "X方向分辨率:" << pBmpInforHead.biXPelsPerMeter << endl;
    cout << "Y方向分辨率:" << pBmpInforHead.biYPelsPerMeter << endl;
    cout << "使用的颜色数:" << pBmpInforHead.biClrUsed << endl;
    cout << "重要颜色数:" << pBmpInforHead.biClrImportant << endl;
}

void readBmp(FILE *fp, unsigned char *&pBmpBuf, int BmpWidth, int BmpHeight,
             int BiBitCount) {
    int lineByte = (BmpWidth * BiBitCount / 8 + 3) / 4 * 4;
    pBmpBuf = new (nothrow) unsigned char[lineByte * BmpHeight];
    if (pBmpBuf == NULL) {
        cerr << "Mem alloc failed." << endl;
        exit(-1);
    }
    fread(pBmpBuf, lineByte * BmpHeight, 1, fp);
}

bool saveBmp(const char *bmpName, unsigned char *imgBuf, int width, int height,
             int biBitCount) {
    if (!imgBuf) return 0;
    int colorTablesize = 0;
    if (biBitCount == 8) colorTablesize = 1024;

    int lineByte = (width * biBitCount / 8 + 3) / 4 * 4;
    FILE *fp = fopen(bmpName, "wb");
    if (fp == 0) {
        cerr << "Open file error." << endl;
        return 0;
    }

    BITMAPFILEHEADER fileHead;
    fileHead.bfType = 0x4D42;
    fileHead.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) +
                      colorTablesize + lineByte * height;
    fileHead.bfReserved1 = 0;
    fileHead.bfReserved2 = 0;
    fileHead.bfOffBits = 54 + colorTablesize;
    fwrite(&fileHead, sizeof(BITMAPFILEHEADER), 1, fp);

    BITMAPINFOHEADER head;
    head.biBitCount = biBitCount;
    head.biClrImportant = 0;
    head.biClrUsed = 0;
    head.biCompression = 0;
    head.biHeight = height;
    head.biPlanes = 1;
    head.biSize = 40;
    head.biSizeImage = lineByte * height;
    head.biWidth = width;
    head.biXPelsPerMeter = 0;
    head.biYPelsPerMeter = 0;
    fwrite(&head, sizeof(BITMAPINFOHEADER), 1, fp);
    fwrite(imgBuf, height * lineByte, 1, fp);
    fclose(fp);
    return 1;
}

void readGsCore() {
    ifstream fin;
    try {
        fin.open("gscore.txt");
    }
    catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        exit(-1);
    }

    int i, j;
    for (i = 0; i < N; i++) {
        for (j = 0; j < N; j++) {
            fin >> GsCore[i][j];
        }
    }
    fin.close();
}

unsigned char getGsValue(int x, int y, const unsigned char *channal) {
    double sum = 0;
    int pixStep = 3;
    for (int i : {0, 1, 2, 3, 4}) {
        for (int j : {0, 1, 2, 3, 4}) {
            int pix_y = y + i - 2;
            int pix_x = x + j - 2;
            if (pix_y < 0 || pix_y >= BmpHeight || pix_x < 0 || pix_y >= BmpWidth)
                continue;
            sum += channal[((BmpWidth * pix_y) + pix_x) * pixStep] * GsCore[i][j];
        }
    }
    return sum;
}

unsigned char* convolution(int base_y, int conv_height) {
    int pixStep = 3;
    const unsigned char* Rp = pBmpBuf + 2;
    const unsigned char* Gp = pBmpBuf + 1;
    const unsigned char* Bp = pBmpBuf;
    unsigned char* resBuf = NULL;
    int conv_byte_size = BmpWidth * conv_height * pixStep;

    resBuf = new(nothrow) unsigned char[conv_byte_size];

    unsigned char* resRp = resBuf + 2;
    unsigned char* resGp = resBuf + 1;
    unsigned char* resBp = resBuf;

    for (int i = 0; i < conv_height; i++)
        for (int j = 0; j < BmpWidth; j++) {
            *resRp = getGsValue(j, base_y + i, Rp);
            *resGp = getGsValue(j, base_y + i, Gp);
            *resBp = getGsValue(j, base_y + i, Bp);

            resRp += pixStep;
            resGp += pixStep;
            resBp += pixStep;
        }

    return resBuf;
}

// 计算执行时间的辅助函数
double get_execution_time(std::chrono::time_point<std::chrono::high_resolution_clock> start) {
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;
    return duration.count();
}

int main(int argc, char *argv[]) {
    int size, myrank, dest;
    MPI_Status status;
    double start_time, end_time;

    // 记录串行计算时间
    auto serial_start = std::chrono::high_resolution_clock::now();

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    start_time = MPI_Wtime();
    cout << "Task " << myrank << " start." << endl;

    BITMAPFILEHEADER BmpHead;
    BITMAPINFOHEADER BmpInfo;

    FILE *fp = fopen(BMP_FILE_NAME, "rb");
    if (fp == 0) {
        cerr << "Can not open " << BMP_FILE_NAME << endl;
        return 0;
    }
    fread(&BmpHead, sizeof(BITMAPFILEHEADER), 1, fp);
    fread(&BmpInfo, sizeof(BITMAPINFOHEADER), 1, fp);

    BmpWidth = BmpInfo.biWidth;
    BmpHeight = BmpInfo.biHeight;
    int BiBitCount = BmpInfo.biBitCount;

    readBmp(fp, pBmpBuf, BmpWidth, BmpHeight, BiBitCount);
    readGsCore();

    unsigned char* result = NULL;
    unsigned char* resBuf = NULL;
    int base_y, convHeight, lastConvHeight;
    int conv_byte_size;

    result = new(nothrow) unsigned char[BmpWidth * BmpHeight * 3];
    if (result == NULL) {
        cerr << "Result new error." << endl;
        exit(-1);
    }

    convHeight = BmpHeight / size;
    base_y = myrank * convHeight;
    lastConvHeight = BmpHeight - (size - 1) * convHeight;
    if (myrank != 0) {
        if (myrank == size - 1) {
            convHeight = lastConvHeight;
        }

        conv_byte_size = BmpWidth * convHeight * 3;
        resBuf = convolution(base_y, convHeight);
        if (resBuf == NULL) goto END;
        dest = 0;
        end_time = MPI_Wtime();
        cout << "Task " << myrank << " end, cost " << end_time - start_time << " second(s)." << endl;
        MPI_Send(resBuf, conv_byte_size, MPI_UNSIGNED_CHAR, dest, 99, MPI_COMM_WORLD);
    }
    else {
        base_y = 0;
        resBuf = convolution(base_y, convHeight);
        if (resBuf == NULL)
            cerr << "0# resBuf error." << endl;

        conv_byte_size = BmpWidth * convHeight * 3;
        memcpy(result, resBuf, conv_byte_size);
        delete[] resBuf;

        int convByteSize = BmpWidth * convHeight * 3;
        int lastConvByteSize = BmpWidth * lastConvHeight * 3;

        resBuf = new(nothrow) unsigned char[lastConvByteSize];
        for (int i = 0; i < size - 1; i++) {
            MPI_Recv(resBuf, conv_byte_size, MPI_UNSIGNED_CHAR, MPI_ANY_SOURCE, 99, MPI_COMM_WORLD, &status);
            memcpy(result + (status.MPI_SOURCE * convHeight * BmpWidth * 3), resBuf,
                    (status.MPI_SOURCE == size - 1) ? lastConvByteSize : convByteSize);
        }
        end_time = MPI_Wtime();
        cout << "Task " << myrank << " end, cost " << end_time - start_time << " second(s)." << endl;
        saveBmp("result.bmp", result, BmpWidth, BmpHeight, BiBitCount);
    }

END:
    if (resBuf) delete[] resBuf;
    MPI_Finalize();
    if (pBmpBuf) delete[] pBmpBuf;
    if (result) delete[] result;
    fclose(fp);

    // 串行执行时间
    double serial_time = get_execution_time(serial_start);
    std::cout << "Serial execution time: " << serial_time << " seconds" << std::endl;

    // 并行执行时间
    double parallel_time = MPI_Wtime() - start_time;
    std::cout << "Parallel execution time with " << size << " processes: " << parallel_time << " seconds" << std::endl;

    // 计算加速比
    double speedup = serial_time / parallel_time;
    std::cout << "Speedup: " << speedup << std::endl;

    return 0;
}
