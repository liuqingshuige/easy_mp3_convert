/*
 * MP3帧解析封装
 * Copyright FreeCode. All Rights Reserved.
 * MIT License (https://opensource.org/licenses/MIT)
 * 2025 by liuqingshuige
 */
#include "easy_mp3_parse_frame.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "print_log.h"

/*
 * 比特率表 (kbits/s)
 */
const short int BitrateTable[2][3][15] =
{
    {
        {0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448},
        {0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384},
        {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320}
    },
    {
        {0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256},
        {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160},
        {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160}
    }
};


/*
 * 采样率表
 */
const unsigned short SamplerateTable[3][3] =
{
    {44100, 48000, 32000}, // MPEG-1
    {22050, 24000, 16000}, // MPEG-2
    {11025, 12000, 8000}  // MPEG-2.5
};


/*
 * 解析MPEG音频帧头
 */
static int parseMpegFrameHdr(unsigned char *hdrBuf, int bufSize, MpegAudioFrameInfo *info, bool firstFrame);

/*
 * 解析"XING"或者"INFO"头，只会出现在第一帧音频数据区域中
 */
static int parseVbrXINGHdr(unsigned char *xingHdr, int bufSize, MpegAudioFrameInfo *info);

/*
 * 解析"VBRI"头，只会出现在第一帧音频数据区域中
 */
static int parseVbrVBRIHdr(unsigned char *vbriHdr, int bufSize, MpegAudioFrameInfo *info);

/*
 * 对Frame info中的数据进行计算
 */
static int calculateFrame(MpegAudioFrameInfo *info);

/*
 * big-endian to local endian
 */
static int bigendian2Local(unsigned char *valueAddr, int len);

/*
 * little-endian to local endian
 */
static int litendian2Local(unsigned char *valueAddr, int len);

/*
 * swap endian:
 * little-endian -> big-endian
 * big-endian -> little-endian
 */
static void exchangeByteEndian(unsigned char *valueAddr, int len);

/*
 * 在运行时判断是否为大端字节序
 */
static inline int isBigendian()
{
    struct bigend
    {
        union
        {
            int a;
            char c;
        }u;
    }b;
    b.u.a = 1;
    return !(b.u.c);
}

/*
 * 寻找下一帧MPEG音频帧的信息
 * buf：MP3数据
 * bufSize：数据大小
 * info：MPEG音频帧的信息，可以为NULL
 * firstFrame：是否是寻找和解析第一帧数据
 * return：MpegAudioResult.errCode为MPEG_AUDIO_OK，则代表解析成功；否则就是需要更多数据。
 *       当解析成功的时候，MpegAudioResult.nextPos代表了下一帧的偏移量；否则代表当下一次传
 *       入更多的数据，应该开始解析的偏移量。
 */
MpegAudioResult findMpegAudioFramePos(unsigned char *buf, int bufSize, MpegAudioFrameInfo *info, bool firstFrame)
{
    MpegAudioResult ret = { MPEG_AUDIO_NEED_MORE, 0 };
    MpegAudioFrameInfo tmp;
    MpegAudioFrameInfo *pFrameInfo = info;

    if (!pFrameInfo)
        pFrameInfo = &tmp;

    // 添加ID3V2标签判断 9-7
    int ID3Size = 0;
    Mp3ID3V23Tag *id3V2_3 = (Mp3ID3V23Tag *)buf;// 查看是否有ID3V2标签
    if ((id3V2_3->header[0] == 'I') && (id3V2_3->header[1] == 'D') && (id3V2_3->header[2] == '3'))
    {
        // 计算标签大小
        ID3Size = (id3V2_3->size[0] << 21) | (id3V2_3->size[1] << 14) | (id3V2_3->size[2] << 7) | (id3V2_3->size[3]) + 10;
        LOG("ID3Size %d bufSize %d\n", ID3Size, bufSize);
        if (ID3Size > bufSize)
        {
            ret.nextPos = ID3Size + 1152 * 2; // 至少需要这么多数据
            return ret;
        }
    }

    int loopSize = bufSize - 1;
    int i = ID3Size ? ID3Size - 1 : 0;
    for (; i < loopSize; i++)
    {
        // 帧同步标识: 1111 1111 111x xxxxb
        if (buf[i] == 0xff && (buf[i + 1] & 0xe0) == 0xe0)
        {
            memset(pFrameInfo, 0, sizeof(*pFrameInfo));
            ret.errCode = parseMpegFrameHdr(buf + i, bufSize - i, pFrameInfo, firstFrame);
            if (MPEG_AUDIO_OK == ret.errCode
                || MPEG_AUDIO_NEED_MORE == ret.errCode)
            {
                break;
            }
        }
        else if (i == loopSize - 1 && buf[i + 1] != 0xff)
        {
            i++;
        }
    }

    ret.nextPos = i + pFrameInfo->frameSize; // 文件指针的位置
    if (i > 0 && ret.errCode != MPEG_AUDIO_OK)
        memset(pFrameInfo, 0, sizeof(MpegAudioFrameInfo));

    return ret;
}


static int parseMpegFrameHdr(unsigned char *hdrBuf, int bufSize, MpegAudioFrameInfo *info, bool firstFrame)
{
    if (bufSize < 4) // 帧头至少有4个字节
        return MPEG_AUDIO_NEED_MORE;

    // Protection Bit
    info->protection = (hdrBuf[1] & 0x01);
    info->protection = (0 == info->protection ? 1 : 0);

    if (info->protection && bufSize < 6)
    {
        // protected by 16 bit CRC following header
        return MPEG_AUDIO_NEED_MORE;
    }

    // MPEG版本
    info->mpegVersion = ((hdrBuf[1] >> 3) & 0x03);
    switch (info->mpegVersion)
    {
    case 0:
        info->mpegVersion = 25;
        break;
    case 2:
        info->mpegVersion = 20;
        break;
    case 3:
        info->mpegVersion = 10;
        break;
    default:
        info->mpegVersion = 0;
        return MPEG_AUDIO_ERR;
    };

    // Layer index
    info->layer = ((hdrBuf[1] >> 1) & 0x03);
    switch (info->layer)
    {
    case 1:
        info->layer = 3;
        break;
    case 2:
        info->layer = 2;
        break;
    case 3:
        info->layer = 1;
        break;
    default:
        info->layer = 0;
        return MPEG_AUDIO_ERR;
    };

    // 比特率
    info->bitrate = ((hdrBuf[2] >> 4) & 0x0f);
    if (info->bitrate == 0x0f)
        return MPEG_AUDIO_ERR;

    unsigned char index_I = (info->mpegVersion == 10 ? 0 : 1);
    unsigned char index_II = info->layer - 1;

	info->bitrate = BitrateTable[index_I][index_II][info->bitrate];

	// 采样率
	info->samplerate = ((hdrBuf[2] >> 2) & 0x03);
	if (info->samplerate == 0x03)
		return MPEG_AUDIO_ERR;

	index_I = 2; // MPEG-2.5 by default
	if (info->mpegVersion == 20)
		index_I = 1;
	else if (info->mpegVersion == 10)
		index_I = 0;

	info->samplerate = SamplerateTable[index_I][info->samplerate];

	// Padding size
	info->paddingSize = ((hdrBuf[2] >> 1) & 0x01);
	if (info->paddingSize)
	{
		info->paddingSize = (info->layer == 1 ? 4 : 1);
	}

	// channel mode
	info->channelMode = ((hdrBuf[3] >> 6) & 0x03);
	switch (info->channelMode)
	{
	case 0:
		info->channelMode = 3;
		break;
	case 1:
		info->channelMode = 0;
		break;
	case 2:
		info->channelMode = 2;
		break;
	case 3:
	default:
		info->channelMode = 1;
	};

	// 在MPEG-1 Layer II中，只有某些比特率和某些模式的组合是允许的
	// 在MPEG -2/2.5，没有此限制
	if (info->mpegVersion == 10 && info->layer == 2)
	{
		if (32 == info->bitrate
			|| 48 == info->bitrate
			|| 56 == info->bitrate
			|| 80 == info->bitrate)
		{
			if (info->channelMode != 1)
				return MPEG_AUDIO_ERR;
		}
		else if (224 == info->bitrate
			|| 256 == info->bitrate
			|| 320 == info->bitrate
			|| 384 == info->bitrate)
		{
			if (1 == info->channelMode)
				return MPEG_AUDIO_ERR;
		}
	}

	// Extension Mode
	info->extensionMode = ((hdrBuf[3] >> 4) & 0x03);
	info->copyrightBit = ((hdrBuf[3] >> 3) & 0x01);
	info->originalBit = ((hdrBuf[3] >> 2) & 0x01);

	// The emphasis indication is here to tell the decoder that the file must be 
	// de-emphasized, that means the decoder must 're-equalize' the sound after 
	// a Dolby-like noise suppression. It is rarely used.
	info->emphasis = (hdrBuf[3] & 0x03);
	if (0x2 == info->emphasis)
		return MPEG_AUDIO_ERR;

	if (info->protection)
	{
		// This checksum directly follows the frame header and is a big-endian 
		// WORD
		// So maybe you shoud convert it to little-endian
		info->CRCValue = *((unsigned short int *)(hdrBuf + 4));
		bigendian2Local((unsigned char *)(&(info->CRCValue)), sizeof(info->CRCValue));
	}

	// 每帧数据的采样数
	info->samplesPerFrame = 1152;
	if (1 == info->layer)
	{
		info->samplesPerFrame = 384;
	}
	else if (info->mpegVersion != 10 && 3 == info->layer)
	{
		info->samplesPerFrame = 576;
	}

	// 边信息大小
	info->sideInfoSize = 0;
	if (3 == info->layer)
	{
		if (info->mpegVersion != 10) // MPEG-2/2.5 (LSF)
		{
			if (info->channelMode != 1) // Stereo, Joint Stereo, Dual Channel
				info->sideInfoSize = 17;
			else // Mono
				info->sideInfoSize = 9;
		}
		else // MPEG-1.0
		{
			if (info->channelMode != 1) // Stereo, Joint Stereo, Dual Channel
				info->sideInfoSize = 32;
			else // Mono
				info->sideInfoSize = 17;
		}
	}

	info->bitrateType = 0; // common CBR by default

	if (firstFrame)
	{
		short int reqSize = 4;

		// DELETE by gansc23 at 2010-12-16 for ignore CRC data size
		//if (info->protection)
		//    reqSize += 2;

		reqSize += info->sideInfoSize;
		reqSize += 4; // "XING" OR "INFO"

		// "XING" OR "INFO"
		int ret1 = parseVbrXINGHdr(hdrBuf + reqSize - 4, bufSize - reqSize + 4, info);
		if (MPEG_AUDIO_OK == ret1)
		{
			goto label_get_XING_or_INFO;
		}
		else if (MPEG_AUDIO_NEED_MORE == ret1)
		{
			return MPEG_AUDIO_NEED_MORE;
		}

		// no "XING" OR "INFO", try to find "VBRI"
		reqSize -= ( info->sideInfoSize + 4 );
		reqSize += 32;

		ret1 = parseVbrVBRIHdr(hdrBuf + reqSize, bufSize - reqSize, info);
		if (MPEG_AUDIO_NEED_MORE == ret1)
		{
			return MPEG_AUDIO_NEED_MORE;
		}
	}

label_get_XING_or_INFO:
	calculateFrame(info);
	return MPEG_AUDIO_OK;
}

int parseVbrXINGHdr(unsigned char *xingHdr, int bufSize, MpegAudioFrameInfo *info)
{
	if (bufSize < 4)
		return MPEG_AUDIO_NEED_MORE;

	info->bitrateType = 0;
	// for "XING"
	if ((xingHdr[0] == 'x' || xingHdr[0] == 'X')
		&& (xingHdr[1] == 'i' || xingHdr[1] == 'I')
		&& (xingHdr[2] == 'n' || xingHdr[2] == 'N')
		&& (xingHdr[3] == 'g' || xingHdr[3] == 'G'))
	{
		// VBR(XING)
		info->bitrateType = 2;
	}
	// for "INFO"
	else if ((xingHdr[0] == 'i' || xingHdr[0] == 'I')
		&& (xingHdr[1] == 'n' || xingHdr[1] == 'N')
		&& (xingHdr[2] == 'f' || xingHdr[2] == 'F')
		&& (xingHdr[3] == 'o' || xingHdr[3] == 'O'))
	{
		// CBR(INFO)
		info->bitrateType = 1;
	}

	if (!info->bitrateType) // no "XING" or "INFO" header
		return MPEG_AUDIO_ERR;

	int offset = 8;
	if (bufSize < offset)
		return MPEG_AUDIO_NEED_MORE;

	// Modified by gansc23 at 2010-12-16 for fixing XING flag bug.
	//unsigned char flags = xingHdr[5];
	unsigned char flags = xingHdr[7];

	if (flags & 0x01) // Frames field is present
	{
		if (bufSize < offset+4)
		{
			return MPEG_AUDIO_NEED_MORE;
		}

		//info->totalFrames = *((int *)(xingHdr+offset));
		memcpy(&info->totalFrames, &xingHdr[offset], 4);
		bigendian2Local((unsigned char *)(&(info->totalFrames)), sizeof(info->totalFrames));
		offset += 4;
	}

	if (flags & 0x02) // Bytes field is present
	{
		if (bufSize < offset+4)
			return MPEG_AUDIO_NEED_MORE;
	
		/*info->totalBytes = *((int *)(xingHdr+offset));*/
		memcpy(&info->totalBytes, &xingHdr[offset], 4);
		bigendian2Local((unsigned char *)(&(info->totalBytes)), sizeof(info->totalBytes));
		offset += 4;
	}

	if (flags & 0x04) // TOC field is present
	{
		if (bufSize < offset+100)
			return MPEG_AUDIO_NEED_MORE;

		offset += 100;
	}

	if (flags & 0x08) // Quality indicator field is present
	{
		if (bufSize < offset+4)
			return MPEG_AUDIO_NEED_MORE;

		/*int quality = *((int *)(xingHdr+offset));*/
		int quality = 0;
		memcpy(&quality, &xingHdr[offset], 4);
		bigendian2Local((unsigned char *)(&quality), sizeof(quality));
		info->quality = 100 - quality;
		offset += 4;
	}

	return MPEG_AUDIO_OK;
}

int parseVbrVBRIHdr(unsigned char *vbriHdr, int bufSize, MpegAudioFrameInfo *info)
{
	if (bufSize < 4)
		return MPEG_AUDIO_NEED_MORE;

	info->bitrateType = 0;

	// for "VBRI"
	if ((vbriHdr[0] == 'v' || vbriHdr[0] == 'V')
		&& (vbriHdr[1] == 'b' || vbriHdr[1] == 'B')
		&& (vbriHdr[2] == 'r' || vbriHdr[2] == 'R')
		&& (vbriHdr[3] == 'i' || vbriHdr[3] == 'I'))
	{
		// VBR
		info->bitrateType = 3;
	}

	if (!info->bitrateType) // no "VBRI" header
		return MPEG_AUDIO_ERR;

	if (bufSize < 26)
		return MPEG_AUDIO_NEED_MORE;

	unsigned char *offset = (vbriHdr + 4);

	// VBRI version
	info->VBRIVersionID = *((short int *)offset);
	bigendian2Local((unsigned char *)(&(info->VBRIVersionID)), sizeof(info->VBRIVersionID));

	offset += 2;

	// Delay，不清楚作用
	(info->VBRIDelay)[0] = *offset;
	(info->VBRIDelay)[1] = *(offset + 1);

	offset += 2;

	// Quality indicator
	info->quality = *((short int *)offset);
	bigendian2Local((unsigned char *)(&(info->quality)), sizeof(info->quality)); // 不确定是以大端字节序存放的
	info->quality = 100 - info->quality; // 不确定

	offset += 2;

	// total bytes
	info->totalBytes = *((int *)offset);
	bigendian2Local((unsigned char *)(&(info->totalBytes)), sizeof(info->totalBytes));

	offset += 4;

	// total frames number
	info->totalFrames = *((int *)offset);
	bigendian2Local((unsigned char *)(&(info->totalFrames)), 
		sizeof(info->totalFrames));

	offset += 4;

	// Number of entries within TOC table
	info->entriesNumInTOCTable = *((short int *)offset);
	bigendian2Local((unsigned char *)(&(info->entriesNumInTOCTable)), sizeof(info->entriesNumInTOCTable));

	offset += 2;

	// Scale factor of TOC table entries
	info->TOCTableFactor = *((short int *)offset);
	bigendian2Local((unsigned char *)(&(info->TOCTableFactor)), sizeof(info->TOCTableFactor));

	offset += 2;

	// size per table entry in bytes (max 4)
	short int entrySize = *((short int *)offset);
	bigendian2Local((unsigned char *)(&entrySize), sizeof(entrySize));
	info->entrySize = entrySize;

	offset += 2;

	// Frames per table entry
	info->framesNumPerTable = *((short int *)offset);
	bigendian2Local((unsigned char *)(&(info->framesNumPerTable)), sizeof(info->framesNumPerTable));

	offset += 2;

	return MPEG_AUDIO_OK;
}

int calculateFrame(MpegAudioFrameInfo *info)
{
	info->frameSize = (info->samplesPerFrame * info->bitrate * 1000)
		/ (8 * info->samplerate)
		+ info->paddingSize;
	return MPEG_AUDIO_OK;
}

int bigendian2Local(unsigned char *valueAddr, int len)
{
	if (len <= 0 || len % 2 != 0)
		return -1;
	if (isBigendian())
	{
		exchangeByteEndian(valueAddr, len);
	}
	return 0;
}

int litendian2Local(unsigned char *valueAddr, int len)
{
	if (len <= 0 || len % 2 != 0)
		return -1;
	if (isBigendian())
		exchangeByteEndian(valueAddr, len);
	return 0;
}

void exchangeByteEndian(unsigned char *valueAddr, int len)
{
	int n = len >> 1;
	for (int i = 0; i < n; i++)
	{
		unsigned char v = *(valueAddr + i);
		*(valueAddr + i) = *(valueAddr + len - 1 - i);
		*(valueAddr + len - 1 - i) = v;
	}
}
