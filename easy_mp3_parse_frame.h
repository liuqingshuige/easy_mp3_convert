/*
 * MP3帧解析封装
 * Copyright FreeCode. All Rights Reserved.
 * MIT License (https://opensource.org/licenses/MIT)
 * 2025 by liuqingshuige
 */
#ifndef __EASY_MP3_PARSE_FRAME_H__
#define __EASY_MP3_PARSE_FRAME_H__

/*
 * 错误码
 */
typedef enum MpegAudioCode
{
	MPEG_AUDIO_OK = 0,
	MPEG_AUDIO_NEED_MORE = -99,
	MPEG_AUDIO_ERR = -199,
}MpegAudioCode;


/*
 * 代表返回结果的结构体
 */
typedef struct MpegAudioResult
{
	int errCode;
	int nextPos;
}MpegAudioResult;

/*
 * ID3V2.3标签头结构：位于MP3文件首部，长度不固定
 * ID3V2.3由一个标签头和若干个标签帧或者一个扩展标签头组成，
 * 至少要有一个标签帧，每一个标签帧记录一种信息，例如作曲、标题等
 */
typedef struct Mp3ID3V23Tag
{
	char header[3];   /* 必须为"ID3"，否则认为标签不存在 */
	char version;     /* 版本号，ID3V2.3就记录3 */
	char revision;    /* 副版本号此版本记录为0 */
	char flag;        /* 标志字节，只使用高三位，其它位为0 */
	char size[4];     /* 标签大小 */
	// total = (size[0]<<21) | (size[1]<<14) | (size[2]<<7) | (size[3]) + 10
}Mp3ID3V23Tag;

/*
 * 代表MPEG音频帧的信息
 */
typedef struct MpegAudioFrameInfo
{
	// MPEG Audio Frame header
	/*
	 * MPEG-1.0: 10
	 * MPEG-2.0: 20
	 * MPEG-2.5: 25
	 * invalid : other
	 */
	int mpegVersion : 6;

	/*
	 * Layer I  : 1
	 * Layer II : 2
	 * Layer III: 3
	 * invalid  : other
	 */
	int layer : 3;

	/*
	 * in kbits/s
	 */
	int bitrate : 10;

	/*
	 * in Bytes
	 */
	int paddingSize : 4;

	/*
	 * Channel mode
	 * Joint Stereo (Stereo) - 0
	 * Single channel (Mono) - 1
	 * Dual channel (Two mono channels) - 2
	 * Stereo - 3
	 */
	int channelMode : 3;

	/*
	 * Mode extension, Only used in Joint Stereo Channel mode
	 * not process
	 */
	int extensionMode : 3;

	/*
	 * Emphasis:
	 * The emphasis indication is here to tell the decoder that the file must be 
	 * de-emphasized, that means the decoder must 're-equalize' the sound after 
	 * a Dolby-like noise suppression. It is rarely used.
	 * 
	 * 0 - none
	 * 1 - 50/15 ms
	 * 2 - reserved (invalid)
	 * 3 - CCIT J.17
	 */
	int emphasis : 3;

	/*
	 * in Hz
	 */
	int samplerate : 17;

	/*
	 * Protection off: 0
	 * Protection on : 1
	 */
	int protection : 2;

	/*
	 * Copyright bit, only informative
	 */
	int copyrightBit : 2;

	/*
	 * Original bit, only informative
	 */
	int originalBit : 2;

	/*
	 * 边信息大小(Layer III), in Bytes
	 */
	int sideInfoSize : 7;

	/*
	 * Samples per frame
	 */
	int samplesPerFrame : 12;

	/*
	 * 0 - CBR
	 * 1 - CBR(INFO)
	 * 2 - VBR(XING)
	 * 3 - VBR(VBRI)
	 */
	int bitrateType : 3;

	/*
	 * 2 Bytes
	 */
	unsigned short int CRCValue;

	// XING, INFO or VBRI header
	int totalFrames;
	int totalBytes;

	/*
	 * Quality indicator
	 * From 0 - worst quality to 100 - best quality
	 */
	short int quality;

	// only for VBRI header
	/*
	 * size per table entry in bytes (max 4)
	 */
	short int entrySize : 4;

	/*
	 * Number of entries within TOC table
	 */
	short int entriesNumInTOCTable;

	/*
	 * Scale factor of TOC table entries
	 */
	short int TOCTableFactor;

	/*
	 * VBRI version
	 */
	short int VBRIVersionID;

	/*
	 * Frames per table entry
	 */
	short int framesNumPerTable;

	/*
	 * VBRI delay
	 * Delay as Big-Endian float
	 */
	unsigned char VBRIDelay[2]; 

	/*
	 * Frame size
	 */
	int frameSize;
} MpegAudioFrameInfo;


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
MpegAudioResult findMpegAudioFramePos(unsigned char *buf, int bufSize, MpegAudioFrameInfo *info, bool firstFrame);

#endif

