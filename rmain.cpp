#define _CRT_SECURE_NO_WARNINGS
#include <math.h>
#include <stdio.h>
#include <sys/types.h>
#include <iostream>
#include <fstream>
#include <mpi.h>
#include <string.h>
#include <stdlib.h>
#include <chrono>
// mpic++ -std=c++11 rmain.cpp -o rmain
// mpirun -np 4 ./rmain
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
} BITMAPINFODEADER;

using namespace std;

#define BMP_FILE_NAME "dog_result2.bmp"

const int N = 5;
double GsCore[N][N];
unsigned char *pBmpBuf = NULL;  //读入图像数据的指针
int BmpWidth;    //图像的宽
int BmpHeight;   //图像的高
/*******************************************************************************/

void showBmpHead(BITMAPFILEHEADER &pBmpHead) {
    cout << "==========位图文件头==========" << endl;
    cout << "文件头类型:" << pBmpHead.bfType << endl;
    cout << "文件大小:" << pBmpHead.bfSize << endl;
    cout << "保留字_1:" << pBmpHead.bfReserved1 << endl;
    cout << "保留字_2:" << pBmpHead.bfReserved2 << endl;
    cout << "实际位图数据的偏移字节数:" << pBmpHead.bfOffBits << endl << endl;
}

void showBmpInforHead(BITMAPINFODEADER &pBmpInforHead) {
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
    /**
     * 灰度图像有颜色表，且颜色表表项为256
     * (可以理解为lineByte是对bmpWidth的以4为步长的向上取整)
     */
    int lineByte = (BmpWidth * BiBitCount / 8 + 3) / 4 * 4;

    //申请位图数据所需要的空间，读位图数据进内存
    pBmpBuf = new (nothrow) unsigned char[lineByte * BmpHeight];

    if (pBmpBuf == NULL) {
        cerr << "Mem alloc failed." << endl;
        exit(-1);
    }

    fread(pBmpBuf, lineByte * BmpHeight, 1, fp);

    return;
}

//给定一个图像位图数据、宽、高、颜色表指针及每像素所占的位数等信息,将其写到指定文件中
bool saveBmp(const char *bmpName, unsigned char *imgBuf, int width, int height,
             int biBitCount) {
    //如果位图数据指针为0，则没有数据传入，函数返回
    if (!imgBuf) return 0;

    //颜色表大小，以字节为单位，灰度图像颜色表为1024字节，彩色图像颜色表大小为0
    int colorTablesize = 0;

    if (biBitCount == 8) colorTablesize = 1024;  // 8*128

    //待存储图像数据每行字节数为4的倍数
    int lineByte = (width * biBitCount / 8 + 3) / 4 * 4;

    //以二进制写的方式打开文件
    FILE *fp = fopen(bmpName, "wb");

    if (fp == 0) {
        cerr << "Open file error." << endl;
        return 0;
    }

    //申请位图文件头结构变量，填写文件头信息
    BITMAPFILEHEADER fileHead;

    fileHead.bfType = 0x4D42;  // bmp类型

    // bfSize是图像文件4个组成部分之和
    fileHead.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) +
                      colorTablesize + lineByte * height;
    fileHead.bfReserved1 = 0;
    fileHead.bfReserved2 = 0;

    // bfOffBits是图像文件前3个部分所需空间之和
    fileHead.bfOffBits = 54 + colorTablesize;

    //写文件头进文件
    fwrite(&fileHead, sizeof(BITMAPFILEHEADER), 1, fp);

    //申请位图信息头结构变量，填写信息头信息
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

    //写位图信息头进内存
    fwrite(&head, sizeof(BITMAPINFOHEADER), 1, fp);

    //写位图数据进文件
    fwrite(imgBuf, height * lineByte, 1, fp);

    //关闭文件
    fclose(fp);

    return 1;
}

void readGsCore() {
    ifstream fin;
    try
    {
        fin.open("gscore.txt");
    }
    catch(const std::exception& e)
    {
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

void genGsCore() {
    int i, j;
    double sigma = 3;
    double sum = 0.0;

    for (i = 0; i < N; i++) {
        for (j = 0; j < N; j++) {
            GsCore[i][j] = exp(-((i - N / 2) * (i - N / 2) + (j - N / 2) * (j - N / 2)) / (2.0 * sigma * sigma));
            sum += GsCore[i][j];
        }
    }

    // 对卷积核进行归一化
    for (i = 0; i < N; i++) {
        for (j = 0; j < N; j++) {
            GsCore[i][j] /= sum;  // 确保权重之和为1
        }
    }

    // 将卷积核保存到文件（可选）
    FILE *fp;
    fp = fopen("gs.txt", "w");
    for (i = 0; i < N; i++) {
        for (j = 0; j < N; j++) {
            fprintf(fp, "%f ", GsCore[i][j]);
        }
        fprintf(fp, "\n");
    }

    fclose(fp);
}

// 限制卷积结果的范围
unsigned char clipValue(double value) {
    if (value < 0) return 0;
    if (value > 255) return 255;
    return static_cast<unsigned char>(value);
}

// 卷积计算时增强亮度
unsigned char enhanceBrightness(unsigned char value) {
    int newValue = value + 0;  // 增加亮度（根据需要调整）
    return clipValue(newValue);
}

/**
 * 给出像素二维坐标位置和通道指针，计算高斯和
 */
unsigned char getGsValue(int x, int y, const unsigned char *channal)
{
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
    unsigned char value = clipValue(sum);
    return enhanceBrightness(value);  // 增强亮度
}


/**
 * 卷积公共计算部分
 * 返回rgb位图数组
 */
unsigned char* convolution(int base_y, int conv_height) {
    int pixStep = 3;    // 每个像素占3个字节（RGB）
    const unsigned char* Rp = pBmpBuf + 2;
    const unsigned char* Gp = pBmpBuf + 1;
    const unsigned char* Bp = pBmpBuf;
    unsigned char* resBuf = NULL;
    int conv_byte_size = BmpWidth * conv_height * pixStep;

    resBuf = new(nothrow) unsigned char[conv_byte_size];

    // 指向结果的RGB指针
    unsigned char* resRp = resBuf + 2;
    unsigned char* resGp = resBuf + 1;
    unsigned char* resBp = resBuf;

    for(int i = 0; i < conv_height; i++)
        for(int j = 0; j < BmpWidth; j++) {
            *resRp = enhanceBrightness(getGsValue(j, base_y + i, Rp));
			*resGp = enhanceBrightness(getGsValue(j, base_y + i, Gp));
			*resBp = enhanceBrightness(getGsValue(j, base_y + i, Bp));


            resRp += pixStep;
            resGp += pixStep;
            resBp += pixStep;
        }

    return resBuf;
}



double get_serial_execution_time(int BmpHeight, int BmpWidth, unsigned char* pBmpBuf) {
    auto start_time = std::chrono::high_resolution_clock::now();

    // 串行卷积计算
    unsigned char* serial_result = new(nothrow) unsigned char[BmpWidth * BmpHeight * 3];
    if (serial_result == NULL) {
        cerr << "Memory allocation failed." << endl;
        exit(-1);
    }

    for (int y = 0; y < BmpHeight; y++) {
        for (int x = 0; x < BmpWidth; x++) {
            serial_result[(y * BmpWidth + x) * 3] = getGsValue(x, y, pBmpBuf); // Red
            serial_result[(y * BmpWidth + x) * 3 + 1] = getGsValue(x, y, pBmpBuf); // Green
            serial_result[(y * BmpWidth + x) * 3 + 2] = getGsValue(x, y, pBmpBuf); // Blue
        }
    }

    delete[] serial_result;

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;
    return elapsed.count();
}


int main(int argc, char *argv[]) {
    int size, myrank, dest;
    MPI_Status status;
    double start_time, end_time;
    unsigned char* result = NULL;
    BITMAPFILEHEADER BmpHead;
    BITMAPINFODEADER BmpInfo;
    int BiBitCount;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // 打开文件并读取头部信息
    FILE *fp = fopen(BMP_FILE_NAME, "rb");
    if (fp == 0) {
        cerr << "Can not open " << BMP_FILE_NAME << endl;
        return 0;
    }

    fread(&BmpHead, sizeof(BITMAPFILEHEADER), 1, fp);
    fread(&BmpInfo, sizeof(BITMAPINFOHEADER), 1, fp);
    BmpWidth = BmpInfo.biWidth;
    BmpHeight = BmpInfo.biHeight;
    BiBitCount = BmpInfo.biBitCount;

    // 读取图像数据
    readBmp(fp, pBmpBuf, BmpWidth, BmpHeight, BiBitCount);
    readGsCore();

    // 计算串行执行时间
    double serial_time = get_serial_execution_time(BmpHeight, BmpWidth, pBmpBuf);
    if (myrank == 0) {
        cout << "Serial execution time: " << serial_time << " seconds" << endl;
    }

    // 并行计算部分
    start_time = MPI_Wtime();

    // MPI 并行计算
    unsigned char* resBuf = NULL;
    int base_y, convHeight, lastConvHeight;
    int conv_byte_size;

    result = new(nothrow) unsigned char[BmpWidth * BmpHeight * 3];
    if(result == NULL) {
        cerr << "Result new error." << endl;
        exit(-1);
    }

    convHeight = BmpHeight / size;
    base_y = myrank * convHeight;
    lastConvHeight = BmpHeight - (size - 1) * convHeight;

    if (myrank != 0) {  // 非0号进程发送消息
        if (myrank == size - 1) {
            convHeight = lastConvHeight;
        }

        conv_byte_size = BmpWidth * convHeight * 3;
        resBuf = convolution(base_y, convHeight);

        if (resBuf == NULL)
            goto END;

        dest = 0;
        end_time = MPI_Wtime();
        cout << "Task " << myrank << " end, cost " << end_time - start_time << " second(s)." << endl;

        MPI_Send(resBuf, conv_byte_size, MPI_UNSIGNED_CHAR, dest, 99, MPI_COMM_WORLD);
    }
    else {   // myrank == 0，即0号进程参与计算并负责接受数据
        base_y = 0;
        resBuf = convolution(base_y, convHeight);
        if (resBuf == NULL) {
            cerr << "0# resBuf error." << endl;
        }

        conv_byte_size = BmpWidth * convHeight * 3;
        memcpy(result, resBuf, conv_byte_size);
        delete resBuf;

        int convByteSize = BmpWidth * convHeight * 3;
        int lastConvByteSize = BmpWidth * lastConvHeight * 3;

        resBuf = new(nothrow) unsigned char[lastConvByteSize];

        // 合并结果
        for (int i = 0; i < size - 1; i++) {
            MPI_Recv(resBuf, conv_byte_size, MPI_UNSIGNED_CHAR, MPI_ANY_SOURCE, 99, MPI_COMM_WORLD, &status);
            memcpy(result + (status.MPI_SOURCE * convHeight * BmpWidth * 3), resBuf,
                   (status.MPI_SOURCE == size - 1) ? lastConvByteSize : convByteSize);
        }

        end_time = MPI_Wtime();
        cout << "Parallel execution time with " << size << " processes: " << end_time - start_time << " seconds" << endl;
        double parallel_time = end_time - start_time;

        // 计算加速比
        double speedup = serial_time / parallel_time;
        cout << "Speedup: " << speedup << endl;

        saveBmp("dog_result3.bmp", result, BmpWidth, BmpHeight, BiBitCount);
    }

END:
    if (resBuf) delete resBuf;
    MPI_Finalize();

    if (pBmpBuf) delete pBmpBuf;
    if (result) delete result;

    fclose(fp);

    return 0;
}
