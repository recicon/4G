#include "TimeModule.h"

const uint8_t c_szMonthDay[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

uint8_t IsLeap(uint16_t wYear)
{
    if ((wYear % 4 == 0 && wYear % 100 != 0) || (wYear % 400 == 0))
        return 1;
    return 0;
}

uint32_t DATE_DateToUtc(tTimeParam tTime)
{
    uint8_t i;
    uint32_t l_dwSencondCnt = 0;

    if (tTime.year > 136)
        tTime.year -= 136;

    for (i = 0; i < tTime.year; i++)
    {
        l_dwSencondCnt += 31536000;
        if (IsLeap(i + 2000))
            l_dwSencondCnt += 86400;
    }

    if (tTime.month > 0)
        tTime.month -= 1;
    for (i = 0; i < tTime.month; i++)
    {
        l_dwSencondCnt += (uint32_t)c_szMonthDay[i] * 86400;
        if (IsLeap(2000 + tTime.year) && i == 1)
            l_dwSencondCnt += 86400;
    }

    if (tTime.day > 0)
        l_dwSencondCnt += (uint32_t)(tTime.day - 1) * 86400;
    l_dwSencondCnt += (uint32_t)tTime.hour * 3600;
    l_dwSencondCnt += (uint32_t)tTime.minute * 60;
    l_dwSencondCnt += tTime.second;

    return l_dwSencondCnt;
}

uint8_t DATE_UtcToDate(uint32_t dwSencondCount, tTimeParam *tTime)
{
    uint32_t l_dwDayCont, l_dwTemp;
    uint16_t l_wTempCnt;
    uint32_t l_dwSencondCnt = DATE_DateToUtc(*tTime) + dwSencondCount;

    l_dwDayCont = l_dwSencondCnt / 86400;
    if (l_dwDayCont)
    {
        l_wTempCnt = 0;
        while (l_dwDayCont >= 365)
        {
            if (IsLeap(l_wTempCnt + 2000))
            {
                if (l_dwDayCont >= 366)
                    l_dwDayCont -= 366;
                else
                    break;
            }
            else
                l_dwDayCont -= 365;
            l_wTempCnt++;
        }
        (*tTime).year = l_wTempCnt;

        l_wTempCnt = 0;
        while (l_dwDayCont >= 28)
        {
            if (IsLeap((*tTime).year + 2000) && l_wTempCnt == 1)
            {
                if (l_dwDayCont >= 29)
                    l_dwDayCont -= 29;
                else
                    break;
            }
            else
            {
                if (l_dwDayCont >= c_szMonthDay[l_wTempCnt])
                    l_dwDayCont -= c_szMonthDay[l_wTempCnt];
                else
                    break;
            }
            l_wTempCnt++;
        }
        (*tTime).month = l_wTempCnt + 1;
        (*tTime).day = l_dwDayCont + 1;
    }
    else
    {
        (*tTime).year = 0;
        (*tTime).month = 1;
        (*tTime).day = 1;
    }

    l_dwTemp = l_dwSencondCnt % 86400;
    (*tTime).hour = l_dwTemp / 3600;
    (*tTime).minute = (l_dwTemp % 3600) / 60;
    (*tTime).second = (l_dwTemp % 3600) % 60;

    return 0;
}
