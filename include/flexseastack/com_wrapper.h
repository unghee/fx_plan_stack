#ifndef COMWRAPPER_H
#define COMWRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

    #include <stdint.h>

	// setup the entire stack
	void fxSetup();
	void fxCleanup();

	// open serial port named portName at portIdx [0-3], 
	void fxOpen(char* portName, int portIdx);
	
	uint8_t fxIsOpen(int portIdx);
	// uint8_t waitOpeN(int portIdx, int timeout);

	// close port at portIdx
	void fxClose(uint16_t portIdx);

	// get the ids of all connected FlexSEA devices
	// n is the length of the array idarray
	// idarray should contain enough space for the function to read into it
	void fxGetDeviceIds(int *idarray, int n);

	// start streaming data from device with id: devId, with given configuration
	uint8_t fxStartStreaming(int devId, int freq, bool shouldLog, int shouldAuto);
	// stop streaming data from device with id: devId
	uint8_t fxStopStreaming(int devId);
	// set which variables are streamed from device with dev id
	// fieldIds specify the variable ids, n specifies the number of fieldIds
	uint8_t fxSetStreamVariables(int devId, int* fieldIds, int n);

    int* fxReadDevice(int devId, int* fieldIds, uint8_t* success, int n);

	// CONTROL functions
	void setControlMode(int devId, int ctrlMode);
	void setMotorVoltage(int devId, int mV);
	void setMotorCurrent(int devId, int cur);
	void setPosition(int devId, int pos);
	void setZGains(int devId, int z_k, int z_b, int i_kp, int i_ki);	
    void actPackFSM2(int devId, int on);
    void findPoles(int devId, int block);



#ifdef __cplusplus
}
#endif

#endif // COMWRAPPER_H
