
#include "flexseastack/commanager.h"
#include "flexseastack/flexsea-projects/ActPack/inc/cmd-ActPack.h"
#include "flexseastack/flexsea-comm/inc/flexsea_comm_def.h"

#include <thread>
#include <iostream>
#include <unordered_map>
#include <tuple>
#include <utility>
#include <exception>

template <int... indexes, typename ... types>
auto get_tuple(std::tuple<types...> &mytup )
{
    
    return std::tie(
        (std::get<indexes>(mytup))...
    );
    
}


extern "C" 
{
        #include "flexseastack/com_wrapper.h"
        #include "flexseastack/flexsea_config.h"

        CommManager commManager;
        std::thread *commThread = nullptr;

        typedef std::tuple<uint8_t, int32_t, uint8_t, int16_t, int16_t, int16_t, int16_t, uint8_t> CtrlParams;
        std::unordered_map<int, CtrlParams> ctrlsMap; 

        void fxSetup()
        {
                initFlexSEAStack_minimalist(FLEXSEA_PLAN_1);
                commManager.taskPeriod = 2;
                commThread = new std::thread(&CommManager::runPeriodicTask, &commManager);
        }

        void fxCleanup()
        {
                commManager.quitPeriodicTask();
                if(commThread)
                {
                        commThread->join();
                        delete commThread;
                        commThread = nullptr;
                }
        }

        // open serial port named portName at portIdx [0-3], 
        void fxOpen(char* portName, int portIdx) 
        {       
                std::string pn = portName;
                std::cout << pn << std::endl;
                commManager.open(pn, portIdx);    
        }

        uint8_t fxIsOpen(int portIdx)
        {
                return commManager.isOpen(portIdx);
        }

        // close port at portIdx
        void fxClose(uint16_t portIdx)
        {       commManager.close(portIdx);    }

        // get the ids of all connected FlexSEA devices
        // idarray should contain enough space for the function to read into it
        // n should provide the length of the input idarray
        // n is written with the new length of the array
        void fxGetDeviceIds(int *idarray, int n)
        {
                std::vector<int> ids = commManager.getDeviceIds();
                if(ids.size() == 0)
                        commManager.sendDeviceWhoAmI(0);

                int i;
                for(i = 0; i < n && i < ids.size(); ++i)
                        idarray[i] = ids[i];
                while(i < n)
                        idarray[i++] = -1;
        }

        CtrlParams defaultCtrlParams()
        {
                return {CTRL_NONE, 0, 0, 0, 0, 0, 0, KEEP};
        }

        // start streaming data from device with id: devId, with given configuration
        uint8_t fxStartStreaming(int devId, int freq, bool shouldLog, int shouldAuto)
        {
                if(!commManager.haveDevice(devId)) return 0;
                if(!ctrlsMap.count(devId))
                        ctrlsMap.insert({devId, CtrlParams()});

                // cmd func periodically writes 
                auto cmdFunc = [devId] (uint8_t* buf, uint8_t* cmdCode, uint8_t* cmdType, uint16_t* len) {

                        if(!ctrlsMap.count(devId)) 
                        {
                            std::cout << "Something wrong, no ctrls map for selected device\n";
                            return;
                        }

                        auto ctrls = ctrlsMap.at(devId);

                        tx_cmd_actpack_rw(buf, cmdCode, cmdType, len,
                                0,                  std::get<0>(ctrls), std::get<1>(ctrls), 
                                std::get<2>(ctrls), std::get<3>(ctrls), std::get<4>(ctrls), 
                                std::get<5>(ctrls), std::get<6>(ctrls), std::get<7>(ctrls)
                                );

                        std::get<2>(ctrlsMap.at(devId)) = KEEP;

                        return;
                };

                // stream reading and commands at same rate
                commManager.startStreaming(devId, freq, shouldLog, shouldAuto);
                commManager.startStreaming(devId, freq, false, cmdFunc);    
        }

        // stop streaming data from device with id: devId
        uint8_t fxStopStreaming(int devId)
        {   return commManager.stopStreaming(devId);    }

        uint8_t fxSetStreamVariables(int devId, int* fieldIds, int n)
        {
                std::vector<int> m;
                m.reserve(n);
                int i;
                for(i=0;i<n;i++)
                        m.push_back(fieldIds[i]);

                commManager.writeDeviceMap(devId, m);
        }

        const int MAX_L = 100;
        int devData[MAX_L];

        int* fxReadDevice(int devId, int* fieldIds, int n)
        {
                auto dev = commManager.getDevicePtr(devId);

                if(dev && dev->hasData())
                {
                        dev->getData( dev->dataCount()-1, devData, MAX_L );
                }

                return &devData[0];
        }

        // -- control functions 
        void setControlMode(int devId, int ctrlMode)
        {
                if(!ctrlsMap.count(devId)) return;
                std::get<0> ( ctrlsMap.at(devId) ) = ctrlMode;
        }

        void setMotorVoltage(int devId, int mV)
        {
                if(!ctrlsMap.count(devId)) return;
                std::get<1> ( ctrlsMap.at(devId) ) = mV;
        }

        void setMotorCurrent(int devId, int cur)
        {
                if(!ctrlsMap.count(devId)) return;
                std::get<1> ( ctrlsMap.at(devId) ) = cur;
        }

        void setPosition( int devId, int pos )
       {
                if(!ctrlsMap.count(devId)) return;
                std::get<1> ( ctrlsMap.at(devId) ) = pos;
        }

        void setZGains(int devId, int z_k, int z_b, int i_kp, int i_ki)
        {
                if(!ctrlsMap.count(devId)) return;
                
                // straight magic right here
                get_tuple<2,3,4,5,6>( ctrlsMap.at(devId) ) = std::make_tuple(CHANGE, z_k, z_b, i_kp, i_ki);
                // std::get<2> (  ) = CHANGE;
                // std::get<3> ( ctrlsMap.at(devId) ) = z_k;
                // std::get<4> ( ctrlsMap.at(devId) ) = z_b;
                // std::get<5> ( ctrlsMap.at(devId) ) = i_kp;
                // std::get<6> ( ctrlsMap.at(devId) ) = i_ki;  
        }


        void actPackFSM2(int devId, uint8_t on)
        {
            get_tuple<0,7>( ctrlsMap.at(devId) ) = std::make_tuple(CTRL_NONE, on ? SYS_NORMAL : SYS_DISABLE_FSM2);
                // std::get<0> ( c ) = CTRL_NONE;
                // std::get<7> ( ctrlsMap.at(devId) ) = on ? SYS_NORMAL : SYS_DISABLE_FSM2;
        }

        void findPoles(int devId, uint8_t block)
        {

        }

        void writeUser(int devId, int index, int value)
        {

        }

        void readUser(int devId, int index, int value)
        {
                
        }

}