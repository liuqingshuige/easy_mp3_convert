/*
 * MP3解码封装：使用minimp3.h库实现
 * Copyright FreeCode. All Rights Reserved.
 * MIT License (https://opensource.org/licenses/MIT)
 * 2025 by liuqingshuige
 */
#ifndef __EASY_MP3_DECODER_H__
#define __EASY_MP3_DECODER_H__

/*
 * 创建解码器
 * return：成功返回解码器句柄，失败NULL
 */
void *EasyMp3DecoderCreate(void);
/*
 * 销毁解码器
 */
void EasyMp3DecoderDestroy(void *handle);
/*
 * 解码MP3数据
 * handle：解码句柄
 * mp3：MP3数据
 * mp3_bytes：作为输入时表示mp3数据大小，作为输出时表示解码消耗的mp3数据大小，单位byte
 * pcm：保存解码数据缓冲区，需要确保足够大
 * pcm_bytes：作为输入时表示pcm缓冲区大小，作为输出时表示解码数据的大小，单位byte
 * return：成功返回0，失败返回-1
 */
int EasyMp3DecoderDecode(void *handle, unsigned char *mp3, int &mp3_bytes, unsigned char *pcm, int &pcm_bytes);
/*
 * 获取MP3信息
 * handle：解码句柄
 * mp3：MP3数据
 * samplerate：采样率
 * channels：声道数
 * bitrate：码率，单位kbps
 * 注意：该函数须在调用EasyMp3DecoderDecode()之后再调用
 */
void EasyMp3DecoderInfo(void *handle, int &samplerate, int &channels, int &bitrate);


#endif

