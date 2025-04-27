/*
 * MP3重编码
 * Copyright FreeCode. All Rights Reserved.
 * MIT License (https://opensource.org/licenses/MIT)
 * 2025 by liuqingshuige
 */
#ifndef __EASY_MP3_CONVERT_H__
#define __EASY_MP3_CONVERT_H__

#include "easy_mp3_decoder.h"
#include "easy_mp3_encoder.h"
#include "mp3_file_parse.h"
#include "MediaAudioResampleEx.h"
#include "print_log.h"
#include "CycleBuffer.h"

#include <string>
#include <vector>
using namespace std;

// MP3重编码实现
class EasyMp3Converter
{
public:
    EasyMp3Converter(const std::string &mp3FileName, int destSampleRate, int destChannel, int destBitRate);
    ~EasyMp3Converter();

    /* 获取一帧/多帧重编码后的MP3数据 */
    bool convert(std::vector<std::vector<unsigned char> > &buffer);

private:
    int operateMonoStereo(int channel, int origin_channel, char *in_ptr, int in_size, char *out_ptr, int out_size);

private:
    int m_destRate, m_destChannel, m_destBitRate;
    CCycleBuffer m_decodedBuf; // 保存MP3解码后的PCM数据
    CCycleBuffer m_resamplerBuf; // 保存PCM重采样后的数据

    Mp3FileParse *m_parser; // MP3帧解析器
    void *m_decoder; // MP3解码器
    EasyMp3Encoder *m_encoder; // MP3编码器
    CResampleEx *m_resampler; // PCM重采样器
};

// 对EasyMp3Converter的改进
class EasyMp3Converter0
{
public:
    EasyMp3Converter0(int destSampleRate, int destChannel, int destBitRate);
    ~EasyMp3Converter0();

    /* 修改要重编码的MP3文件 */
    bool open(const std::string &mp3FileName);

    /* 获取一帧重编码后的MP3数据 */
    bool convert(std::vector<unsigned char> &frame);

private:
    int m_destRate, m_destChannel, m_destBitRate;
    EasyMp3Converter *m_converter;
    std::vector<std::vector<unsigned char> > m_buffer;
};


#endif


