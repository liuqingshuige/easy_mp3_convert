#ifndef __CYCLE_BUFFER_H__
#define __CYCLE_BUFFER_H__

class CCycleBuffer
{
public:
	CCycleBuffer(void);
	~CCycleBuffer(void);

	bool Init(int len);
	void Clean();
	int Read(unsigned char *pdata, int len);
	int Write(unsigned char *pdata, int len);
	int GetLength() {return m_cur;}
	int GetFreeLength() {return m_total - m_cur;}
	int Cover(unsigned char *pdata,int len);

protected:
	unsigned char *m_buf;
	unsigned char *m_readPtr;
	unsigned char *m_writePtr;
	int m_total;
	int m_cur;
};

#endif

