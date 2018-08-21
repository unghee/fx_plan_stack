#ifndef COMWRAPPER_H
#define COMWRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

    #include <stdint.h>

    // -----------------------
    // Serial and Setup functions
    // -----------------------

    /// \brief Setup the wrapper's stack environment
	void fxSetup();

    /// \brief Clean up the wrapper's stack environment
	void fxCleanup();

    /// \brief Open a serial port named portName at portIdx [0-3]
	void fxOpen(char* portName, int portIdx);

    /// \brief Check if portIdx [0-3] is open (1) or closed (0)
	uint8_t fxIsOpen(int portIdx);

    /// \brief close port at portIdx
	void fxClose(uint16_t portIdx);

    // -----------------------
    // Stream configuration and reading functions
    // -----------------------

    /// \brief get the ids of all connected FlexSEA devices.
    /// n is the length of the array idarray
    /// idarray should contain enough space for the function to read into it
	void fxGetDeviceIds(int *idarray, int n);

    /// \brief start streaming data from device with id=devId, with given configuration
    /// @param freq : must match one of the those allowed in CommManager (set in constructor).
    /// @param shouldLog : if set true, the program logs all received data to a .csv file.
    /// @param shouldAuto : if set true, the device triggers itself to send read messages,
    /// otherwise the Plan stack sends read requests at intervals
	uint8_t fxStartStreaming(int devId, int freq, bool shouldLog, int shouldAuto);

    /// \brief stop streaming data from device with id=devId
	uint8_t fxStopStreaming(int devId);

    /// \brief set which variables are streamed from device with id=devId
    /// @param fieldIds : specify the ids of variables to stream
    /// @param n : specifies the length of the  array fieldIds
    uint8_t fxSetStreamVariables(int devId, int* fieldIds, int n);

    /// \brief a utility function to access the most recent values received by device with id=devId
    /// @param fieldIds :   specify the ids of variables to read.
    ///                     (device must have been configured to stream these data with fxSetStreamVariables)
    /// @param success :    output variable, which tells you whether the read succeeded
    ///                     (will fail if the device is not configured to stream this field)
    /// @param n : specifies the length of the array fieldIds and the output array success (lengths must match)
    int* fxReadDevice(int devId, int* fieldIds, uint8_t* success, int n);

    // -----------------------
    // Control functions
    // -----------------------

    /// \brief sets the type of control (open voltage / current / position / impedance)
	void setControlMode(int devId, int ctrlMode);

    /// \brief sets the setpoint in mV (control type must be open voltage)
	void setMotorVoltage(int devId, int mV);

    /// \brief sets the setpoint in mA (control type must be current)
	void setMotorCurrent(int devId, int cur);

    /// \brief sets the setpoint in encoder ticks (control type must be position or impedance)
	void setPosition(int devId, int pos);

    /// \brief sets the gains used by PID controllers on device with id=devId
    ///  @param KP1 : proportional gain 1 ( used for current in current control & position in position/impedance control )
    ///  @param KI1 : integral gain 1     ( used for current in current control & position in position/impedance control )
    ///  @param KP2 : proportional gain 2 ( used for for the underlying current control within the impedance controller )
    ///  @param KI2 : integral gain 2     ( used for for the underlying current control within the impedance controller )
    void setZGains(int devId, int KP1, int KI1, int KP2, int KI2);

    /// \brief enables or disables user fsm 2 on the device
    void actPackFSM2(int devId, int on);

    /// \brief runs the find poles procedure on the device
    void findPoles(int devId, int block);

#ifdef __cplusplus
}
#endif

#endif // COMWRAPPER_H
