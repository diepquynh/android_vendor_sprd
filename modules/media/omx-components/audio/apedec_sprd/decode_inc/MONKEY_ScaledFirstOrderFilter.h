#ifndef APE_SCALEDFIRSTORDERFILTER_H
#define APE_SCALEDFIRSTORDERFILTER_H

#define MULTIPLY 31
#define SHIFT 5

typedef struct _CScaledFirstOrderFilter {
	int m_nLastValue;
} CScaledFirstOrderFilter;

static __inline void MONKEY_CScaledFirstOrderFilter_Flush(CScaledFirstOrderFilter * CCScaledFirstOrderFilter)
{
    CCScaledFirstOrderFilter->m_nLastValue = 0;
}

static __inline int MONKEY_CScaledFirstOrderFilter_Compress(CScaledFirstOrderFilter * CCScaledFirstOrderFilter, const int nInput)
{
    int nRetVal = nInput - ((CCScaledFirstOrderFilter->m_nLastValue * MULTIPLY) >> SHIFT);
    CCScaledFirstOrderFilter->m_nLastValue = nInput;
    return nRetVal;
}

static __inline int MONKEY_CScaledFirstOrderFilter_Decompress(CScaledFirstOrderFilter * CCScaledFirstOrderFilter, const int nInput)
{
    CCScaledFirstOrderFilter->m_nLastValue = nInput + ((CCScaledFirstOrderFilter->m_nLastValue * MULTIPLY) >> SHIFT);
    return CCScaledFirstOrderFilter->m_nLastValue;
}

#endif // #ifndef APE_SCALEDFIRSTORDERFILTER_H
