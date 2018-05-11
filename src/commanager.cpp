#include "flexseastack/commanager.h"

#include <chrono>
#include <thread>
#include <iostream>

#ifdef TEST_CODE

extern "C" {
    #include "flexseastack/flexsea-system/inc/flexsea_cmd_sysdata.h"
    #include "flexseastack/flexsea-comm/inc/flexsea_comm_multi.h"
}
#include "flexseastack/flexsea-system/inc/flexsea_system.h"

#else

extern "C" {
    #include "flexseastack/flexsea-system/inc/flexsea_device_spec.h"
    #include "flexseastack/flexsea-system/inc/flexsea_cmd_sysdata.h"
}
    #include "flexseastack/flexsea-comm/inc/flexsea.h"
    #include "flexseastack/flexsea-comm/inc/flexsea_comm_multi.h"
    #include "flexseastack/flexsea-system/inc/flexsea_system.h"
    #include "flexseastack/flexsea_board.h"
    #include "flexseastack/flexsea-projects/Rigid/inc/cmd-Rigid.h"
#endif

CommManager::CommManager() : PeriodicTask()
{
	// Because this class is used in a thread, init is called after the class
	// has been passed to the thread. This avoid allocating heap in the
	// "creator thread" instead of the "SerialDriver thread".
	// see https://wiki.qt.io/QThreads_general_usage
}

CommManager::~CommManager(){}

void CommManager::init(FlexseaSerial* serialDriver)
{
	//this needs to be in order from smallest to largest
	int timerFreqsInHz[NUM_TIMER_FREQS] = {1, 5, 10, 20, 33, 50, 100, 200, 300, 500, 1000};
	for(int i = 0; i < NUM_TIMER_FREQS; i++)
	{
		timerFrequencies[i] = timerFreqsInHz[i];
		timerIntervals[i] = 1000.0f / timerFreqsInHz[i];
		autoStreamLists[i] = std::vector<CmdSlaveRecord>();
		streamLists[i] = std::vector<CmdSlaveRecord>();
	}

    streamCount = 0;
    fxSerial = serialDriver;
}

int CommManager::getIndexOfFrequency(int freq)
{
	int indexOfFreq = -1;
	for(int i = 0; i < NUM_TIMER_FREQS; i++)
	{
		if(freq == timerFrequencies[i])
		{
			indexOfFreq = i;
			i = NUM_TIMER_FREQS;
		}
	}
	return indexOfFreq;
}

std::vector<int> CommManager::getStreamingFrequencies() const
{
    std::vector<int> r;
    r.resize(NUM_TIMER_FREQS);
    int *d = r.data();
    memcpy(d, timerFrequencies, sizeof(int) * NUM_TIMER_FREQS);
    return r;
}

bool CommManager::startStreaming(int devId, int freq, bool shouldLog, bool shouldAuto)
{
	int indexOfFreq = getIndexOfFrequency(freq);

    if(indexOfFreq < 0 || indexOfFreq >= NUM_TIMER_FREQS)
    {
        std::cout << "Invalid frequency" << std::endl;
        return false;
    }

    if(!fxSerial->getDevice(devId).isValid())
    {
        std::cout << "Invalid device id" << std::endl;
        return false;
    }

    CmdSlaveRecord record(CMD_SYSDATA, devId, shouldLog);

#ifndef TEST_CODE
    if(shouldAuto) {
        streamLists[indexOfFreq].push_back(record);
        std::cout << "Started streaming cmd: " << CMD_SYSDATA << ", for slave id: " << devId << " at frequency: " << freq << std::endl;



    } else {
        autoStreamLists[indexOfFreq].push_back(record);
        std::cout << "Started autostreaming cmd: " << CMD_SYSDATA << ", for slave id: " << devId << " at frequency: " << freq << std::endl;
    }
#else
    streamLists[indexOfFreq].push_back(record);
    std::cout << "[Fake] Started " << (shouldLog ? " logged " : "") << (shouldAuto ? "auto" : "") << "streaming cmd: " << CMD_SYSDATA << ", for slave id: " << devId << " at frequency: " << freq << std::endl;
#endif

    // increase stream count only for regular streaming
    bool doNotify;
    {
        std::lock_guard<std::mutex> l(conditionMutex);
        streamCount += shouldAuto ? 0 : 1;
        doNotify = streamCount == 1;
    }
    // if stream count is exactly one, then we need to wake our sleeping worker thread
    if(doNotify)
        wakeCV.notify_all();

    return true;
}

bool CommManager::stopStreaming(int devId)
{
    std::vector<CmdSlaveRecord>* listArray[2] = {autoStreamLists, streamLists};

    bool found = false;

    for(int listIndex = 0; listIndex < 2; listIndex++)
    {
        std::vector<CmdSlaveRecord> *l = listArray[listIndex];

        for(int indexOfFreq = 0; indexOfFreq < NUM_TIMER_FREQS; indexOfFreq++)
        {
            for(unsigned int i = 0; i < (l)[indexOfFreq].size(); /* no increment */)
            {
                CmdSlaveRecord record = (l)[indexOfFreq].at(i);
                if(record.slaveIndex == devId)
                {
                    (l)[indexOfFreq].erase( (l)[indexOfFreq].begin() + i);

                    if(listIndex == 0)
                    {
                        //packAndSendStopStreaming(cmd, devId);
                    }
                    else
                    {
                        std::lock_guard<std::mutex> l(conditionMutex);
                        streamCount--;
                    }

                    std::cout << "Stopped " << (listIndex == 0 ? "autostreaming" : "streaming")
                              << " cmd: " << record.cmdType
                              << ", for slave id: " << devId
                              << " at frequency: " << timerFrequencies[indexOfFreq] << std::endl;
                    found = true;
                }
                else    //only increment i if we aren't erasing from the vector
                    i++;
            }
        }
    }

    return found;
}

void CommManager::periodicTask()
{
    const float TOLERANCE = 0.0001;
    int i;
    if(outgoingBuffer.size())
    {
        Message m = outgoingBuffer.front();
        outgoingBuffer.pop();
        fxSerial->write(m.numBytes, m.dataPacket.get());
    }
    else
    {
        for(i = 0; i < NUM_TIMER_FREQS; i++)
        {
            if(!streamLists[i].size()) continue;

            //received clocks comes in at 5ms/clock
            msSinceLast[i] += taskPeriod;

            float timerInterval = timerIntervals[i];
            if((msSinceLast[i] + TOLERANCE) > timerInterval)
            {
                sendCommands(i);

                while((msSinceLast[i] + TOLERANCE) > timerInterval)
                    msSinceLast[i] -= timerInterval;
            }
        }
    }
}

bool CommManager::wakeFromLongSleep()
{
    return (this->outgoingBuffer.size() || this->streamCount > 0);
}
bool CommManager::goToLongSleep()
{
    return streamCount == 0;
}

void CommManager::tryPackAndSend(int cmd, int slaveId)
{
    const FlexseaDevice &d = fxSerial->getDevice(slaveId);
    (void)cmd;
#ifndef TEST_CODE
    if(d.port < 0 || d.port >= FX_NUMPORTS || d.type == FX_NONE) return;

    uint16_t numb = 0;
    uint8_t info[2] = {PORT_USB, PORT_USB};
    pack(P_AND_S_DEFAULT, slaveId, info, &numb, comm_str_usb);

    fxSerial->write(numb, comm_str_usb, d.port);
#else
    (void)d;
    printf("Writing to \"serial\": ");
    fwrite(comm_str_usb, COMM_PERIPH_ARR_LEN, 1, stdout);
    fflush(stdout);
#endif
}

bool CommManager::enqueueCommand(uint8_t numb, uint8_t* dataPacket, int portIdx)
{
	//If we are over a max size, clear the queue
	const unsigned int MAX_Q_SIZE = 200;

	if(outgoingBuffer.size() > MAX_Q_SIZE)
	{
        std::cout << "ComManager::enqueueCommand, queue is above max size (" << MAX_Q_SIZE  << "), clearing queue..." << std::endl;
		while(outgoingBuffer.size())
			outgoingBuffer.pop();
	}

    outgoingBuffer.push(Message(numb, dataPacket, portIdx));

    bool doNotify;
    {
        std::lock_guard<std::mutex> lk(conditionMutex);
        doNotify = streamCount < 1;
    }
    if(doNotify)
        wakeCV.notify_all();

    return true;
}

void CommManager::sendCommands(int index)
{
	if(index < 0 || index >= NUM_TIMER_FREQS) return;

	for(unsigned int i = 0; i < streamLists[index].size(); i++)
	{
		CmdSlaveRecord record = streamLists[index].at(i);
		switch(record.cmdType)
        {
#ifndef TEST_CODE
            case CMD_READ_ALL_RIGID:
                sendCommandRigid(record.slaveIndex);
                break;

#endif //TEST_CODE

        case CMD_SYSDATA:
            sendSysDataRead(record.slaveIndex);
            break;

			default:
                std::cout<< "Unsupported command was given: " << record.cmdType << std::endl;
                //stopStreaming(record.cmdType, record.slaveIndex, timerFrequencies[index]);
				break;
		}
	}
}

#ifndef TEST_CODE
void CommManager::sendCommandRigid(uint8_t slaveId)
{
    static int index = 0;
    index++;
    index %= 3;

    tx_cmd_rigid_r(TX_N_DEFAULT, (uint8_t)(index));
    tryPackAndSend(CMD_READ_ALL_RIGID, slaveId);
}

#endif // TEST_CODE

void CommManager::sendSysDataRead(uint8_t slaveId)
{
    const FlexseaDevice &d = fxSerial->getDevice(slaveId);

    if(d.id != slaveId) return;

    MultiCommPeriph *cp = fxSerial->portPeriphs + d.port;
    MultiWrapper *out = &cp->out;

    const uint8_t RESERVEDBYTES = 4;
    uint8_t cmdCode, cmdType;
    out->unpackedIdx = 0;

    uint32_t bitmap[3];
    d.getBitmap(bitmap);
    uint32_t flag = bitmap[0];
    uint8_t lenFlags = 1, error;

    // NOTE: in a response, we need to reserve bytes for XID, RID, CMD, and TIMESTAMP
    tx_cmd_sysdata_r(out->unpacked + RESERVEDBYTES, &cmdCode, &cmdType, &out->unpackedIdx, &flag, lenFlags);
    out->unpacked[0] = FLEXSEA_PLAN_1;
    out->unpacked[1] = FLEXSEA_MANAGE_1;
    out->unpacked[2] = 0;
    out->unpacked[3] = (cmdCode << 1) | 0x1;
    out->unpackedIdx += RESERVEDBYTES;

    out->currentMultiPacket++;
    out->currentMultiPacket%=4;
    error = packMultiPacket(out);

    if(error)
        std::cout << "Error packing multipacket" << std::endl;
    else
    {
        unsigned int frameId = 0;
        while(out->frameMap > 0)
        {
            fxSerial->write(PACKET_WRAPPER_LEN, out->packed[frameId], d);
            out->frameMap &= (   ~(1 << frameId)   );
            frameId++;
        }
        out->isMultiComplete = 1;
    }
}

