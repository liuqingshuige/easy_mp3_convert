#include "easy_mp3_convert.h"


/*
 * mp3FileName：待重编码MP3文件
 * destSampleRate：重编码后的采样率，如44100，32000，16000等，最大支持48000
 * destChannel：重编码后的声道数，如1或者2，表示单声道或者双声道
 * destBitRate：重编码后的比特率，单位kbps，如32，64，128等
 */
EasyMp3Converter::EasyMp3Converter(const std::string &mp3FileName,
    int destSampleRate, int destChannel, int destBitRate)
{
    m_decodedBuf.Init(32 * 1024); // 保存MP3解码后的PCM数据
    m_resamplerBuf.Init(32 * 1024); // 保存PCM重采样后的数据

    m_decoder = EasyMp3DecoderCreate(); // MP3解码器

    m_parser = new Mp3FileParse(mp3FileName); // MP3帧解析器

    m_encoder = new EasyMp3Encoder(); // MP3编码器
    m_encoder->start(destBitRate, destSampleRate, destChannel);

    m_destBitRate = destBitRate;
    m_destChannel = destChannel;
    m_destRate = destSampleRate;
}

EasyMp3Converter::~EasyMp3Converter()
{
    if (m_decoder)
        EasyMp3DecoderDestroy(m_decoder);
    m_decoder = 0;

    if (m_parser)
        delete m_parser;
    m_parser = 0;

    if (m_encoder)
        delete m_encoder;
    m_encoder = 0;

    if (m_resampler)
        delete m_resampler;
    m_resampler = 0;
}

/* 单双声道互转
 * in_size：in_ptr数据大小，单位字节
 * out_size：out_ptr缓冲区大小，单位字节
 * return：成功返回转换后的字节数，失败-1
 */
int EasyMp3Converter::operateMonoStereo(int channel, int origin_channel, char *in_ptr, int in_size, char *out_ptr, int out_size)
{
    if (channel == origin_channel) // 声道数相同，直接返回数据
    {
        if (out_size >= in_size)
        {
            memcpy(out_ptr, in_ptr, in_size);
            return in_size;
        }
        return -1; // 缓冲区不够
    }
    else // 声道数不同
    {
        if (origin_channel == 1 && channel == 2) // 单声道转双声道
        {
            short *ptr = (short *)in_ptr;
            char *tmp = new char[in_size << 1];
            short *ptr1 = (short *)tmp;

            for (int i = 0; i < in_size >> 1; i++)
            {
                ptr1[2 * i] = ptr1[2 * i + 1] = ptr[i];
            }

            if (out_size >= (in_size << 1))
            {
                memcpy(out_ptr, ptr1, (in_size << 1));
                delete[]tmp;
                return (in_size << 1);
            }

            delete[]tmp;
            return -1; // 缓冲区不够
        }
        else if (origin_channel == 2 && channel == 1) // 双声道转单声道
        {
            short *ptr = (short *)in_ptr;
            short *ptr1 = (short *)out_ptr;

            if (out_size >= (in_size >> 1))
            {
                for (int n = 0; n < in_size >> 2; n++)
                {
                    *ptr1++ = *ptr++;
                    ptr++;
                }
                return (in_size >> 1);
            }
            return -1;
        }

        return 0; // 其他声道不支持
    }
}

/* 获取一帧/多帧重编码后的MP3数据 */
bool EasyMp3Converter::convert(std::vector<std::vector<unsigned char> > &buffer)
{
    int ret = -1, try_time = 2;
    bool res = false;
    char pcm_data[16 * 1152] = { 0 };
    int pcm_bytes = 0, mp3_bytes = 0;
    std::vector<unsigned char> mp3_data;

    buffer.clear();

    while (buffer.size() == 0 && (try_time--))
    {
        /* 获取一帧原MP3编码数据 */
        res = m_parser->GetNextFrame(mp3_data);
        if (!res)
            return false;

        /* 进行解码 */
        pcm_bytes = sizeof(pcm_data);
        mp3_bytes = mp3_data.size();

        ret = EasyMp3DecoderDecode(m_decoder, mp3_data.data(), mp3_bytes,
            (unsigned char *)pcm_data, pcm_bytes);

        if (ret != 0 || pcm_bytes <= 0)
            return false;

        int samplerate, channels, bitrate;
        EasyMp3DecoderInfo(m_decoder, samplerate, channels, bitrate);
        if (samplerate == m_destRate && channels == m_destChannel)
        {
            buffer.push_back(mp3_data);
            return true;
        }

        /* 双声道转单声道，重采样仅支持单声道，双声道会有问题 */
        if (channels == 2)
        {
            pcm_bytes = operateMonoStereo(1, 2, pcm_data, pcm_bytes, pcm_data, sizeof(pcm_data));
            channels = 1;
        }

        int samples_per_frame = (samplerate / 1000) * channels * 20; // 原PCM数据一帧的采样点数

        if ((samplerate != m_destRate) && (!m_resampler)) // 创建重采样器
        {
            m_resampler = new CResampleEx();
            res = m_resampler->resample_create(true, true, channels, samplerate, m_destRate, samples_per_frame);
        }

        m_decodedBuf.Cover((unsigned char *)pcm_data, pcm_bytes); // 保存解码后的PCM数据

        /* 进行重采样：默认位宽16bit */
        int real_size = 0;
        int frame_size = samples_per_frame * 16 / 8; // 原PCM数据一帧的字节数
        short out_ptr[8192];
        unsigned char *data = new unsigned char[frame_size];

        while (m_decodedBuf.GetLength() >= frame_size)
        {
            if (m_decodedBuf.Read(data, frame_size) != frame_size) // 获取解码后的PCM数据
                break;

            if (samplerate != m_destRate) // 需要重采样
            {
                unsigned int osize = m_resampler->resample_get_output_size(); // 重采样后的采样点数
                real_size = osize << 1; // 单位字节
                m_resampler->resample_run((short *)data, out_ptr);
            }
            else // 不需要重采样
            {
                memcpy(out_ptr, data, frame_size);
                real_size = frame_size;
            }

            ret = operateMonoStereo(m_destChannel, channels, (char *)out_ptr, real_size, pcm_data, sizeof(pcm_data)); // 单双声道转换
            /* 保存重采样后的PCM数据 */
            if (ret > 0)
                m_resamplerBuf.Cover((unsigned char *)pcm_data, ret);
        }
        delete[]data;

        /* 进行重编码 */
        int samples = m_encoder->samples();
        int encode_data_len = samples * 16 / 8;
        unsigned char *encode_data = new unsigned char[encode_data_len];

        while (m_resamplerBuf.GetLength() >= encode_data_len)
        {
            if (m_resamplerBuf.Read(encode_data, encode_data_len) != encode_data_len)
                break;

            short *pOut = 0;
            ret = m_encoder->encode((const short *)encode_data, encode_data_len / sizeof(short), &pOut);

            if (ret > 0)
            {
                const unsigned char *ptr = (const unsigned char *)pOut;
                std::vector<unsigned char> frame;

                frame.assign(ptr, ptr + ret * sizeof(short));
                buffer.push_back(frame);
            }
        }
        delete[]encode_data;
    }

    return buffer.size();
}

/*
 * destSampleRate：重编码后的采样率，如44100，32000，16000等，最大支持48000
 * destChannel：重编码后的声道数，如1或者2，表示单声道或者双声道
 * destBitRate：重编码后的比特率，单位kbps，如32，64，128等
 */
EasyMp3Converter0::EasyMp3Converter0(int destSampleRate, int destChannel, int destBitRate)
{
    m_destBitRate = destBitRate;
    m_destChannel = destChannel;
    m_destRate = destSampleRate;
    m_converter = NULL;
}

EasyMp3Converter0::~EasyMp3Converter0()
{
    if (m_converter)
        delete m_converter;
    m_converter = NULL;
}

/* 修改要重编码的MP3文件 */
bool EasyMp3Converter0::open(const std::string &mp3FileName)
{
    if (m_converter) // 删除上一次的转换器
        delete m_converter;

    m_buffer.clear(); // 清理上一次的数据
    m_converter = new EasyMp3Converter(mp3FileName, m_destRate, m_destChannel, m_destBitRate);

    bool res = m_converter->convert(m_buffer); // 同时获取编码数据
    return res;
}

/* 获取一帧重编码后的MP3数据 */
bool EasyMp3Converter0::convert(std::vector<unsigned char> &frame)
{
    if (!m_converter)
        return false;

    frame.clear();
    if (m_buffer.size() > 0) // 当前是否已经有重编码数据
    {
        frame.assign(m_buffer[0].data(), m_buffer[0].data() + m_buffer[0].size());
        m_buffer.erase(m_buffer.begin());
        return true;
    }

    bool res = m_converter->convert(m_buffer);
    if (res)
    {
        frame.assign(m_buffer[0].data(), m_buffer[0].data() + m_buffer[0].size());
        m_buffer.erase(m_buffer.begin());
        return true;
    }

    return false;
}





