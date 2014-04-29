#ifndef __TIMEOUT_CALCULATOR_H
#define __TIMEOUT_CALCULATOR_H

#include <unordered_map>
#include "constants.h"
#include "types.h"
#include <sys/timeb.h>
#include <math.h>
#define alpha 0.85
#define defaultTimeout 20

//Uses Jacobson's algorithm to compute timeout
class TimeoutCalculator
{
private:
    std::unordered_map<seqNo_t, struct timeb> _map;
    float _srtt, _sdev;     //SRTT is in ms
    void UpdateSrtt(struct timeb startTime, struct timeb endTime)
    {
        int rttInms = (endTime.time - startTime.time) * 1000 + endTime.millitm -
                startTime.millitm;
        _srtt = alpha * _srtt + (1 - alpha) * rttInms;
        _sdev = alpha * _sdev + (1 - alpha) * abs(_srtt - rttInms);
    }
public:
    TimeoutCalculator()
    {
        _srtt = _sdev = 0;
    }
    void SentSeq(seqNo_t seqNo)
    {
        //Irrespective of whether seqNo exists or not, map current time to seqNo
        struct timeb currentTime;
        ftime(&currentTime);
        _map[seqNo] = currentTime;
    }
    void RecvAck(seqNo_t ackNo)
    {
        //TODO: Implement threads for listening on ports to improve RTT accuracy
        //Find timestamp first to ignore effects of map search.
        struct timeb currentTime;
        ftime(&currentTime);
        std::unordered_map<seqNo_t, struct timeb>::const_iterator found =
                _map.find (ackNo);
        if (found == _map.end())
            return;
        UpdateSrtt(found->second, currentTime);
        _map.erase(found);
    }
    int GetTimeoutInms()
    {
        int rto = round(_srtt + 4 * _sdev);
        rto = rto < TCP_RTO_LOWER ? TCP_RTO_LOWER : rto;
        rto = rto > TCP_RTO_UPPER ? TCP_RTO_UPPER : rto;
        return rto;
    }
    void Clear()
    {
        _srtt = _sdev = 0;
        _map.clear();
    }
};

#endif //!__TIMEOUT_CALCULATOR_H
