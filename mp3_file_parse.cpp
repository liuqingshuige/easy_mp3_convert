#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "easy_mp3_parse_frame.h"
#include "mp3_file_parse.h"
#include "print_log.h"


Mp3FileParse::Mp3FileParse(const std::string &filename)
{
    FILE *fp = fopen(filename.c_str(), "rb");
    m_file = fp;
    m_nextPos = 0;
}

Mp3FileParse::~Mp3FileParse()
{
    FILE *fp = (FILE *)m_file;
    if (fp)
        fclose(fp);
    m_file = NULL;
}

/*
 * 获取一帧MP3数据，保存在mp3data中
 * return：成功返回true，失败返回false
 */
bool Mp3FileParse::GetNextFrame(std::vector<unsigned char> &mp3data)
{
    FILE *mp3File = (FILE *)m_file;
    if (!mp3File)
        return false;

    MpegAudioFrameInfo info;
    MpegAudioResult ret = {0, 0};
    int readLen = 0;
    char mp3Buff[4608];

    fseek(mp3File, 0, SEEK_SET); // 回到文件头
    fseek(mp3File, m_nextPos, SEEK_SET); // 下一帧的位置

    readLen = fread(mp3Buff, 1, sizeof(mp3Buff), mp3File);

    ret = findMpegAudioFramePos((unsigned char *)mp3Buff, readLen, &info, (m_nextPos == 0) ? true : false);
    if (ret.errCode == MPEG_AUDIO_OK)
    {
        if (m_nextPos == 0) // 第一帧
            memmove(mp3Buff, mp3Buff + (ret.nextPos - info.frameSize), info.frameSize);

        mp3data.clear();
        mp3data.assign(mp3Buff, mp3Buff + info.frameSize);

        m_nextPos += ret.nextPos;

        return true;
    }
    else if (ret.errCode == MPEG_AUDIO_NEED_MORE) // 需要更多数据
    {
        if (feof(mp3File))
        {
            return false;
        }

        char *newBuf = new char[ret.nextPos];

        fseek(mp3File, 0, SEEK_SET); // 回到文件头
        fseek(mp3File, m_nextPos, SEEK_SET); // 下一帧的位置

        readLen = fread(newBuf, 1, ret.nextPos, mp3File);

        if (readLen != ret.nextPos)
        {
            delete []newBuf;
            return false;
        }

        ret = findMpegAudioFramePos((unsigned char *)newBuf, readLen, &info, (m_nextPos == 0) ? true : false);

        if (ret.errCode == MPEG_AUDIO_OK)
        {
            if (m_nextPos == 0) // 第一帧
                memmove(newBuf, newBuf + (ret.nextPos - info.frameSize), info.frameSize);

            mp3data.clear();
            mp3data.assign(newBuf, newBuf + info.frameSize);

            m_nextPos += ret.nextPos;

            delete []newBuf;

            return true;
        }
        delete []newBuf;
    }

    return false;
}




