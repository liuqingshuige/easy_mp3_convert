#include "easy_mp3_convert.h"
#include <time.h>
#include <fstream>
using namespace std;

#define DEST_SAMPLERATE 44100
#define DEST_CHANNELS 2
#define DEST_BITRATE 128 // 码率，单位kbps

// 返回开机以来的时间(ms)
unsigned long GetTickCount(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (ts.tv_sec*1000+ts.tv_nsec/1000000);
}


int main(int argc, char **argv)
{
    run_init_log(4, 0);
    if (argc < 2)
    {
        LOG("usage: %s <mp3-file1> [<mp3-file2> ...]\n", argv[0]);
        return -1;
    }

    char filename[64] = {0};
	std::vector<unsigned char> frame;

    snprintf(filename, sizeof(filename), "%d_%d_16.mp3", DEST_SAMPLERATE, DEST_CHANNELS);
    std::ofstream outfile(filename, std::ios::binary);

    if (!outfile.is_open())
    {
        LOG("can not open file: %s\n", filename);
        return -1;
    }

    for (int i=0; i<argc-1; i++)
    {
        EasyMp3Converter0 *converter = new EasyMp3Converter0(DEST_SAMPLERATE, DEST_CHANNELS, DEST_BITRATE);
		converter->open(argv[i+1]);

        LOG("converting %d --> %s\n", i+1, argv[i+1]);
        unsigned long stick = GetTickCount();
        while (converter->convert(frame))
        {
			LOG("frame: %d, sz: %d\n", frame.size(), frame.size());
            outfile.write((const char *)frame.data(), frame.size());
        }
        LOG("converted %s cost time: %lu ms\n", argv[i+1], GetTickCount()-stick);
        delete converter;
    }

    return 0;
}





