#include "commanager.h"
#include "cmd-ActPack.h"
#include "flexsea_system.h"
#include "flexsea_comm_def.h"
#include "revision.h"

#include <thread>
#include <iostream>
#include <unordered_map>
#include <tuple>
#include <utility>
#include <exception>
#include <algorithm>
#include <cstring>
#include <string>

using namespace std::chrono_literals;

template <int... indexes, typename ... types>
auto get_tuple(std::tuple<types...> &mytup )
{
	return std::tie(
		(std::get<indexes>(mytup))...
	);
}

extern "C"
{
	#include "com_wrapper.h"
	#include "flexsea_config.h"
	#include "flexsea_cmd_calibration.h"

	CommManager* commManager;
	// static std::thread *commThread = nullptr;

	typedef std::tuple<uint8_t, int32_t, uint8_t, int16_t, int16_t, int16_t, int16_t, uint8_t> CtrlParams;
	static std::unordered_map<int, CtrlParams> ctrlsMap;

	CommManager* fxGetManager(void)
	{
		return commManager;
	}

	void fxSetup()
	{
		initFlexSEAStack_minimalist(FLEXSEA_PLAN_1);
		commManager = new CommManager();
	}

	void fxCleanup()
	{
		delete commManager;
	}

	// open serial port named portName at portIdx [0-3],
	void fxOpen(char* portName, int portIdx)
	{
		commManager->loadAndGetDevice(portIdx);
		// commManager.open(std::string(portName), portIdx);
	}

	uint8_t fxIsOpen(int portIdx)
	{
		return commManger->isOpen(portIdx);
	}

	// close port at portIdx
	void fxClose(uint16_t portIdx)
	{
		return commManager->closeDevice(portIdx);
		// commManager.close(portIdx);
	}

	// get the ids of all connected FlexSEA devices
	// idarray should contain enough space for the function to read into it
	// n should provide the length of the input idarray
	// n is written with the new length of the array
	void fxGetDeviceIds(int *idarray, int n)
	{
		std::vector<int> ids = commManager->getDeviceIds();

		size_t i;
		for(i = 0; i < n && i < ids.size(); ++i){
				idarray[i] = ids[i];
		}

		//fill rest of array with -1
		std::fill_n(std::begin(idarray) + i, std::end(idarray), -1);
	}

	static CtrlParams defaultCtrlParams()
	{
		return std::make_tuple(CTRL_NONE, 0, 0, 0, 0, 0, 0, KEEP);
	}

	void sendCommandMessage(uint8_t* buf, uint8_t* cmdCode, uint8_t* cmdType, uint16_t* len, int devId)
	{
		if(!ctrlsMap.count(devId))
		{
			std::cout << "Something wrong, no ctrls map for selected device\n";
			return;
		}

		auto ctrls = ctrlsMap.at(devId);

		tx_cmd_actpack_rw(buf, cmdCode, cmdType, len,
				0,					std::get<0>(ctrls), std::get<1>(ctrls),
				std::get<2>(ctrls), std::get<3>(ctrls), std::get<4>(ctrls),
				std::get<5>(ctrls), std::get<6>(ctrls), std::get<7>(ctrls)
				);

		std::get<2>(ctrlsMap.at(devId)) = KEEP;
	}

	// start streaming data from device with id: devId, with given configuration
	uint8_t fxStartStreaming(int devId, int freq, bool shouldLog, int shouldAuto)
	{
		if(!commManager->isValidDevId(devId)) return 0;
		if(!ctrlsMap.count(devId))
				ctrlsMap.insert({devId, defaultCtrlParams()});

		// stream reading and commands at same rate
		commManager->startStreaming(devId, freq, shouldLog, shouldAuto);
		return 1;
	}

	// stop streaming data from device with id: devId
	uint8_t fxStopStreaming(int devId)
	{
		return commManager->stopStreaming(devId);
	}

	uint8_t fxSetStreamVariables(int devId, int* fieldIds, int n)
	{
		std::vector<int> m;
		m.reserve(n);
		int i;
		for(i = 0; i < n; i++)
		{
			m.push_back(fieldIds[i]);
		}

		return !commManager->writeDeviceMap(devId, m);
	}

	const int MAX_L = 100;
	int devData[MAX_L];
	int devDataPriv[MAX_L];

	int* fxReadDevice(int devId, int* fieldIds, uint8_t* success, int n)
	{
		auto dev = commManager.getDevicePtr(devId);
		memset(success, 0, n);
		if(!dev)
		{
			std::cout << "Device does not exist" << std::endl;
			return &devData[0];
		}
		if(!dev->hasData())
		{
			std::cout << "Device does not have data" << std::endl;
			return &devData[0];
		}
		if(dev->getDataPtr( dev->dataCount()-1, (FX_DataPtr)devDataPriv, MAX_L ) == 0)
		{
			std::cout << "Failed to read device data" << std::endl;
			return &devData[0];
		}

		auto activeIds = dev->getActiveFieldIds();

		
		for(int i = 0; i < n; i++)
		{
			auto it = std::find(activeIds.begin(), activeIds.end(), fieldIds[i]);
			if(it != activeIds.end())
			{
				devData[i] = devDataPriv[1 + fieldIds[i]];
				success[i] = 1;
			}
			else
			{
				std::cout << "Requested field not found" << std::endl;
				devData[i] = 0;
			}
		}
		fflush(stdout);
		return &devData[0];
	}
	int fxReadDeviceEx(int devId, int* fieldIds, uint8_t* success, int* dataBuffer, int n)
	{
		// Initialize return values (ensure theya re all set to false)
		memset(success, 0, n);
		int returnCount = 0;

		// Check input parameters
		if(!dataBuffer)
		{
			std::cout << "Invalid Input buffer or size" << std::endl;
			return returnCount;
		}
		auto dev = commManager.getDevicePtr(devId);
		if(!dev)
		{
			std::cout << "Device does not exist" << std::endl;
			return returnCount;
		}
		if(!dev->hasData())
		{
			std::cout << "Device does not have data" << std::endl;
			return returnCount;
		}
		if(dev->getDataPtr( dev->dataCount()-1, (FX_DataPtr)devDataPriv, MAX_L ) == 0)
		{
			std::cout << "Failed to read device data" << std::endl;
			return returnCount;
		}

		// We know we have data and a place to put it
		auto activeIds = dev->getActiveFieldIds();
		for(int i = 0; i < n; i++)
		{
			auto it = std::find(activeIds.begin(), activeIds.end(), fieldIds[i]);
			if(it != activeIds.end())
			{
				dataBuffer[i] = devDataPriv[1 + fieldIds[i]];
				success[i] = 1;
			}
			else
			{
				std::cout << "Requested field not found ex" << std::endl;
				dataBuffer[i] = 0;
			}

			// We have increased the number of elements being returned
			returnCount++;
		}

		fflush(stdout);
		return returnCount;
	}

	// -- control functions
	void setControlMode(int devId, int ctrlMode)
	{
		if(!ctrlsMap.count(devId)) return;
		std::get<0> ( ctrlsMap.at(devId) ) = ctrlMode;
		commManager->enqueueCommand(devId, sendCommandMessage, devId);
	}

	void setMotorVoltage(int devId, int mV)
	{
		if(!ctrlsMap.count(devId)) return;
		std::get<1> ( ctrlsMap.at(devId) ) = mV;
		commManager->enqueueCommand(devId, sendCommandMessage, devId);
	}
	
	void readUser(int devId)
	{
		commManager->enqueueCommand(devId, tx_cmd_data_user_r, 0);
	}
	
	
	void writeUser(int devId, int index, int val)
	{
		user_data_1.w[index] = val;
		commManager->enqueueCommand(devId, tx_cmd_data_user_w, index);
	}

	int* getUserRead()
	{
		return &user_data_1.r[0];
	}
	
	int* getUserWrite()
	{
		return &user_data_1.w[0];
	}
	
	void setMotorCurrent(int devId, int cur)
	{
		if(!ctrlsMap.count(devId)) return;
		std::get<1> ( ctrlsMap.at(devId) ) = cur;
		commManager->enqueueCommand(devId, sendCommandMessage, devId);
	}

	void setPosition( int devId, int pos )
	{
		if(!ctrlsMap.count(devId)) return;
		std::get<1> ( ctrlsMap.at(devId) ) = pos;
		commManager->enqueueCommand(devId, sendCommandMessage, devId);
	}

	void setGains(int devId, int g0, int g1, int g2, int g3)
	{
		if(!ctrlsMap.count(devId)) return;
		get_tuple<2,3,4,5,6>( ctrlsMap.at(devId) ) = std::make_tuple(CHANGE, g0, g1, g2, g3);
		commManager->enqueueCommand(devId, sendCommandMessage, devId);
	}

	void actPackFSM2(int devId, int on)
	{
		get_tuple<0,7>( ctrlsMap.at(devId) ) = std::make_tuple(CTRL_NONE, on ? SYS_NORMAL : SYS_DISABLE_FSM2);
		commManager->enqueueCommand(devId, sendCommandMessage, devId);
	}

	void findPoles(int devId, int block)
	{
		if(!commManager.haveDevice(devId)) return;
		commManager->enqueueCommand(devId, tx_cmd_calibration_mode_rw, CALIBRATION_FIND_POLES);

		if(block)
		{
			int waitlength = 60;
			for(int i=0;i<waitlength;i++)
			{
				std::cout << "Waited for " << i << " / " << waitlength << " seconds..." << std::endl;
				std::this_thread::sleep_for(1s);
			}
		}
	}

	const char* fxGetRevision( LIB_REVISION_E whichLib )
	{
		std::string ptr;
		std::string seperator = " @ ";

		if( whichLib == FX_PLAN_STACK_E )
		{
			std::string d = FX_PLAN_STACK_DATE;
			std::string g = FX_PLAN_STACK_GIT_INFO;
			ptr = g.substr( g.find(':') + 1);
			ptr += seperator;
			ptr += d.substr( d.find(':') + 1);
		}
		else
		{
			std::string d = SERIAL_LIB_DATE;
			std::string g = SERIAL_LIB_GIT_INFO;
			ptr = g.substr( g.find(':') + 1);
			ptr += seperator;
			ptr += d.substr( d.find(':') + 1);
		}

		return ptr.c_str();
	}
}
