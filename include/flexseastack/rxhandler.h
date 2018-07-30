#ifndef RXHANDLER_H
#define RXHANDLER_H

#include <unordered_map>
#include <mutex>
#include <functional>

#define LOCK_MUTEX(m) std::lock_guard<std::mutex> lk##m (m)

struct _MultiPacketInfo_s;
typedef _MultiPacketInfo_s MultiPacketInfo;

typedef std::function<void(MultiPacketInfo*, uint8_t*, uint16_t)> RxHandler;

class RxHandlerManager
{

public:

    void addRxHandler(int cmdCode, RxHandler func)
    {
        LOCK_MUTEX(_rxFuncMapMutex);

        if(_rxFuncMap.count(cmdCode))
            _rxFuncMap.at(cmdCode) = func;
        else
            _rxFuncMap.emplace(cmdCode, func);
    }

    void removeRxHandler(int cmdCode)
    {
        LOCK_MUTEX(_rxFuncMapMutex);
        _rxFuncMap.erase(cmdCode);
    }

protected:
    bool isCmdOverloaded (int cmdCode)
    {
        LOCK_MUTEX(_rxFuncMapMutex);
        return _rxFuncMap.count(cmdCode);
    }

    void callRx(int cmdCode, MultiPacketInfo* info, uint8_t* input, uint16_t inputLen)
    {
        LOCK_MUTEX(_rxFuncMapMutex);
        if(_rxFuncMap.count(cmdCode))
            _rxFuncMap.at(cmdCode)(info, input, inputLen);
    }

private:

    std::mutex _rxFuncMapMutex;
    std::unordered_map<int, RxHandler> _rxFuncMap;

};

#endif // RXHANDLER_H
