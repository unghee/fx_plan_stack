#include <iostream>
#include <stdio.h>
#include <algorithm>

#include "flexseaserial.h"
#include "flexsea_comm_multi.h"
#include "flexsea_multi_frame_packet_def.h"

extern "C" {
	#include "flexsea_device_spec.h"
	#include "flexsea_cmd_sysdata.h"
	#include "flexsea_sys_def.h"
	#include "flexsea_payload.h"
	#include "flexsea_comm_multi.h"
	#include "flexsea_multi_circbuff.h"
	#include "flexsea_system.h"
	#include "flexsea_dataformats.h"
}

#define LONG_ID(shortId, port) ((shortId << 6) | port)

FlexseaSerial::FlexseaSerial()
	: SerialDriver(FX_NUMPORTS)
{
	initializeDeviceSpecs();
	multiCommPeriphs = new MultiCommPeriph[FX_NUMPORTS];

	for(int i = 0; i < FX_NUMPORTS; i++){
		initMultiPeriph(this->multiCommPeriphs + i, PORT_USB, SLAVE);
	}
}

FlexseaSerial::~FlexseaSerial()
{
	delete[] multiCommPeriphs;
	multiCommPeriphs = nullptr;
}

#define CALL_MEMBER_FN(object,ptrToMember)  ((object)->*(ptrToMember))

void FlexseaSerial::sendDeviceWhoAmI(int port)
{

	uint32_t flag = 0;
	uint8_t lenFlags = 1, error;

	MultiWrapper *out = &multiCommPeriphs[port].out;
	error = CommStringGeneration::generateCommString(0, out, tx_cmd_sysdata_r, &flag, lenFlags);

	if(error)
		std::cout << "Error packing multipacket" << std::endl;
	else
	{
		unsigned int frameId = 0;
		while(out->frameMap > 0)
		{
			write(PACKET_WRAPPER_LEN, out->packed[frameId], port);
			out->frameMap &= (   ~(1 << frameId)   );
			frameId++;
		}
		out->isMultiComplete = 1;
		std::cout << "Wrote who am i message" << std::endl;
	}
}

bool FlexseaSerial::open(std::string portName, int portIdx)
{
	return tryOpen(portName, portIdx);
}

inline int updateDeviceMetadata(int port, FlexseaDevice* &serialDevice, uint8_t *buf)
{
	uint16_t i = MP_DATA1 + 1;
	uint8_t devType, devShortId, j, mapLen, devRole;
	devType = buf[i++];
	devShortId = buf[i++];
	mapLen = buf[i++];
	devRole = buf[i + mapLen * sizeof(int32_t)];

	int devId = LONG_ID(devShortId, port);

	if(serialDevice == nullptr){
		serialDevice = new FlexseaDevice(devId, devShortId, port, static_cast<FlexseaDeviceType>(devType), devRole);
	}
	assert(devType == serialDevice->_devType);

	//update bitmap (may be unchanged)
	uint32_t bitmap[FX_BITMAP_WIDTH];
	for(j=0; j < FX_BITMAP_WIDTH && j < mapLen; j++){
		bitmap[j] = REBUILD_UINT32(buf, &i);
	}

	serialDevice->setBitmap(bitmap);

	return 0;
}


inline int updateDeviceData(FlexseaDevice* serialDevice, uint8_t *buf)
{
	if(serialDevice == nullptr){
		return -1;
	}

	assert(serialDevice->isValid());
	serialDevice->updateData(buf);

	return 0;
}


int FlexseaSerial::sysDataParser(int port, MultiCommPeriph* mCP, FlexseaDevice* &serialDevice){
	uint8_t *msgBuf = mCP->in.unpacked;
	bool isMeantForPlan = msgBuf[MP_RID] / 10 == 1;
	if(!isMeantForPlan){
		std::cout << "Received message with invalid RID, probably some kind of device-side error\n";
		return -1;
	}

	uint8_t isMetaData = msgBuf[MP_DATA1];
	if(isMetaData){
		return updateDeviceMetadata(port, serialDevice, msgBuf);
	}
	else{
		return updateDeviceData(serialDevice, msgBuf);
	}
}

void FlexseaSerial::processReceivedData(uint8_t* largeRxBuffer, size_t len, int portIdx, FlexseaDevice* &serialDevice){
//    int numMessagesExpected = (totalBuffered / COMM_STR_BUF_LEN);
	// int maxMessagesExpected = (totalBuffered / COMM_STR_BUF_LEN + (totalBuffered % COMM_STR_BUF_LEN != 0));
	MultiCommPeriph* mCP = multiCommPeriphs + portIdx;
	int totalBuffered = len + circ_buff_get_size(&(mCP->circularBuff));
	int numMessagesReceived = 0;
	int maxMessagesExpected = (totalBuffered - 1) / COMM_STR_BUF_LEN + 1; //roundup int division


	uint16_t bytesToWrite, cbSpace, bytesWritten=0;
	int circ_error, successfulParse;

	while(len > 0)
	{
		cbSpace = CB_BUF_LEN - circ_buff_get_size(&(mCP->circularBuff));
		bytesToWrite = MIN(len, cbSpace);

		circ_error = circ_buff_write(&(mCP->circularBuff), (largeRxBuffer + bytesWritten), bytesToWrite);
		assert(!circ_error);
		// if(error) std::cout << "circ_buff_write error:" << error << std::endl;

		do {
			mCP->bytesReadyFlag = 1;
			mCP->in.isMultiComplete = 0;

			int convertedBytes = unpack_multi_payload_cb(&mCP->circularBuff, &mCP->in);
			
			circ_error = circ_buff_move_head(&mCP->circularBuff, convertedBytes);
			assert(!circ_error);

			if(mCP->in.isMultiComplete)
			{
				uint8_t cmd = MULTI_GET_CMD7(mCP->in.unpacked);
				int parseResult;

				if(cmd == CMD_SYSDATA)
				{
					// use sys data handling
					parseResult = sysDataParser(portIdx, mCP, serialDevice);
				}
				else if(isCmdOverloaded(cmd))
				{
					// use user added Rx function
					MultiPacketInfo info;
					info.xid = mCP->in.unpacked[MP_XID];
					info.rid = mCP->in.unpacked[MP_RID];
					info.portIn = portIdx;
					info.portOut = portIdx;

					callRx(cmd, &info, mCP->in.unpacked + MP_DATA1, mCP->in.unpackedIdx);
				}
				else
				{
					assert(serialDevice);
					mCP->in.unpacked[MP_XID] = serialDevice->getRole();

					parseResult = parseReadyMultiString(mCP);
				}

				numMessagesReceived++;
				(void) parseResult;
			}

			successfulParse = convertedBytes > 0 && !circ_error;
		} while(successfulParse && numMessagesReceived < maxMessagesExpected);

		len -= bytesToWrite;
		bytesWritten += bytesToWrite;

		if(CB_BUF_LEN == circ_buff_get_size(&(mCP->circularBuff)) && len)
		{
			std::cout << "circ buffer is full with non valid frames; clearing..." << std::endl;
			// erase all the bytes except the ones we just wrote
			circ_buff_move_head(&(mCP->circularBuff), CB_BUF_LEN - bytesToWrite);
		}
	}	
}

void FlexseaSerial::readAndProcessData(int portIdx, FlexseaDevice* &serialDevice){
	size_t i, bytesToRead;
	long int numBytes;
	uint8_t largeRxBuffer[MAX_SERIAL_RX_LEN];

	numBytes = bytesAvailable(portIdx);
	while(numBytes > 0){
		bytesToRead = numBytes > MAX_SERIAL_RX_LEN ? MAX_SERIAL_RX_LEN : numBytes;
		numBytes -= bytesToRead;
		readPort(portIdx, largeRxBuffer, bytesToRead);
		processReceivedData(largeRxBuffer, bytesToRead, portIdx, serialDevice);
	}
}
