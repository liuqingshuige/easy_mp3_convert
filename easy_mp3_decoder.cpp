/*
 * MP3解码封装：使用minimp3.h库实现
 * Copyright FreeCode. All Rights Reserved.
 * MIT License (https://opensource.org/licenses/MIT)
 * 2025 by liuqingshuige
 */
#include <stdlib.h>
#include <stdio.h>
#include "easy_mp3_decoder.h"
#define MINIMP3_IMPLEMENTATION
#include "minimp3.h"

 // 解码器结构
typedef struct EasyMp3Decoder
{
    mp3dec_t mp3d;
    mp3dec_frame_info_t minfo;
}EasyMp3Decoder;

/*
 * 创建解码器
 * return：成功返回解码器句柄，失败NULL
 */
void *EasyMp3DecoderCreate(void)
{
    EasyMp3Decoder *decoder = new EasyMp3Decoder();
    if (decoder)
    {
        mp3dec_init(&decoder->mp3d);
        memset(&decoder->minfo, 0, sizeof(decoder->minfo));
    }
    return decoder;
}

/*
 * 销毁解码器
 */
void EasyMp3DecoderDestroy(void *handle)
{
    EasyMp3Decoder *decoder = (EasyMp3Decoder *)handle;
    if (decoder)
    {
        delete decoder;
    }
}

/*
 * 解码MP3数据
 * handle：解码句柄
 * mp3：MP3数据
 * mp3_bytes：作为输入时表示mp3数据大小，作为输出时表示解码消耗的mp3数据大小，单位byte
 * pcm：保存解码数据缓冲区，需要确保足够大
 * pcm_bytes：作为输入时表示pcm缓冲区大小，作为输出时表示解码数据的大小，单位byte
 * return：成功返回0，失败返回-1
 */
int EasyMp3DecoderDecode(void *handle, unsigned char *mp3, int &mp3_bytes, unsigned char *pcm, int &pcm_bytes)
{
    EasyMp3Decoder *decoder = (EasyMp3Decoder *)handle;
    if (!decoder)
        return -1;
    decoder->minfo.frame_bytes = 0;
    int res = mp3dec_decode_frame(&decoder->mp3d, mp3, mp3_bytes, (mp3d_sample_t *)pcm, &decoder->minfo);
    mp3_bytes = decoder->minfo.frame_bytes; // 返回消耗掉的MP3数据
    pcm_bytes = res * decoder->minfo.channels * sizeof(mp3d_sample_t); // 解码数据大小：解码成功或跳过ID3/非法数据，需要更多数据进行解码
    return 0;
}

/*
 * 获取MP3信息
 * handle：解码句柄
 * mp3：MP3数据
 * samplerate：采样率
 * channels：声道数
 * bitrate：码率，单位kbps
 * 注意：该函数须在调用EasyMp3DecoderDecode()之后再调用
 */
void EasyMp3DecoderInfo(void *handle, int &samplerate, int &channels, int &bitrate)
{
    EasyMp3Decoder *decoder = (EasyMp3Decoder *)handle;
    if (!decoder)
        return;

    samplerate = decoder->minfo.hz;
    channels = decoder->minfo.channels;
    bitrate = decoder->minfo.bitrate_kbps;
}

