#ifndef __FREE_MP3_FILE_PARSE_H__
#define __FREE_MP3_FILE_PARSE_H__

#include <string>
#include <vector>
using namespace std;

// 完成MP3文件的逐帧提取

class Mp3FileParse
{
public:
    Mp3FileParse(const std::string &filename);
    ~Mp3FileParse();

    bool GetNextFrame(std::vector<unsigned char> &mp3data);

private:
    void *m_file;
    int m_nextPos;
};

#endif

