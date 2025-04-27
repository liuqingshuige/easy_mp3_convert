#include "CycleBuffer.h"
#include <string.h>


CCycleBuffer::CCycleBuffer(void)
{
	m_buf = NULL;
	m_readPtr = NULL;
	m_writePtr = NULL;
	m_total = 0;
	m_cur = 0;
}

CCycleBuffer::~CCycleBuffer(void)
{
	if (m_buf)
	{
		delete []m_buf;
		m_buf = NULL;
	}

	m_readPtr = NULL;
	m_writePtr = NULL;
	m_total = 0;
	m_cur = 0;
}

bool CCycleBuffer::Init(int len)
{
    if (len <= 0)
        return false;

	m_buf = new unsigned char[len];
	if (m_buf == NULL)
		return false;

	m_readPtr = m_buf;
	m_writePtr = m_buf;
	m_total = len;
	m_cur = 0;
	return true;
}

void CCycleBuffer::Clean()
{
	m_readPtr = m_buf;
	m_writePtr = m_buf;
	m_cur = 0;
}

int CCycleBuffer::Read(unsigned char *pdata, int dlen)
{
	if (!m_buf || m_cur == 0)
		return 0;

	if (dlen > m_cur)
		dlen = m_cur;

	int len = 0;
	if (m_readPtr + dlen > (m_buf + m_total))
	{
		int rlen2 = m_buf + m_total - m_readPtr;
		len += Read(pdata, rlen2);
		pdata += rlen2;
		dlen -= rlen2;
		m_readPtr = m_buf;
	}

	memcpy(pdata, m_readPtr, dlen);
	m_readPtr += dlen;
	m_cur -= dlen;
	len += dlen;
	return len;
}

int CCycleBuffer::Write(unsigned char *pdata, int wlen)
{
	int ifree = m_total - m_cur;
	if (!m_buf || ifree == 0)
		return 0;

	if (wlen > ifree) // 多出来的被丢了?
		wlen = ifree;

	int len = 0;
	if (m_writePtr + wlen > (m_buf + m_total))
	{
		int wlen2 = m_buf + m_total - m_writePtr;
		len += Write(pdata, wlen2);
		pdata += wlen2;
		wlen -= wlen2;
		m_writePtr = m_buf;
	}

	memcpy(m_writePtr, pdata, wlen);
	m_writePtr += wlen;
	m_cur += wlen;
	len += wlen;
	return len;
}

int CCycleBuffer::Cover(unsigned char *pdata, int wlen)
{
	if (!m_buf || m_total < wlen)
		return 0;

	int ifree = m_total - m_cur;
	if (ifree < wlen)
	{
		int icover = wlen - ifree;
		m_readPtr += icover;
		if (m_readPtr > m_buf + m_total)
			m_readPtr -= m_total;
		m_cur -= icover;
	}
	return Write(pdata, wlen);
}

