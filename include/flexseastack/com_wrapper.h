#ifndef COMWRAPPER_H
#define COMWRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif
/*! @file com_wrapper.h
    @brief File that documents the API of the FlexSEA Plan Stack.
*/
    #include <stdint.h>
    #include <stdbool.h>

    // --------------------------
    // Serial and Setup functions
    // --------------------------

    /// \brief Initialize the FlexSEA API library environment. This must be called
    /// before any call to other functions in this library.
    /// @returns Nothing.
	void fxSetup();

    /// \brief Clean up the FlexSEA API library environment. Call this before
    ///  exiting your program.
    /// @returns Nothing.
	void fxCleanup();

    /// \brief Open a serial port to communicate with a FlexSEA device.
    /// @param portName is the name of the port to open (e.g. "COM3")
    /// @param portIdx is a user defined "handle" that refers to this port.
    /// @returns Nothing.
	void fxOpen(char* portName, int portIdx);

    /// \brief Check if a com port has been successfully opened.
    /// @param portIdx is the "handle" supplied in fxOpen()
    /// @returns 1 if the port is open, 0 otherwise
	uint8_t fxIsOpen(int portIdx);

    /// \brief Close a com port.
    /// @param portIdx is the handle to the port to close
    /// @returns Nothing.
	void fxClose(uint16_t portIdx);

    // ------------------------------------------
    // Stream configuration and reading functions
    // ------------------------------------------

    /// \brief Get the device id of all connected FlexSEA devices. The device ID is an opaque
    /// handle used by the functions in this API to specify which FlexSEA device
    /// to communicate with.
    /// @param idarray On return idarray will contain "handle" for each FlexSEA device found.
    /// Each element of the array will contain -1 for no valid device or an opaque 'handle'
    /// that refers to a device. On input it should contain enough space for the maximum number
    /// of devices supported (currently 3).
    /// @param n is the length of the array idarray.
    /// @returns Nothing. idarray is updated with device handles.
    /// @note idArray must be pre-allocated with enough space for the maximum number of
    /// FlexSEA devices supported by this API (currently 3).
	void fxGetDeviceIds(int *idarray, int n);

    /// \brief This function is called to select which FlexSEA variables are streamed from a device.
    /// @param devId is the opaque handle for the device.
    /// @param fieldIds is an array of variables to stream. Each element should contain
    /// a valid id. Variables are described at http://dephy.com/wiki/flexsea/doku.php?id=fxdevicefields
    /// @param n Specifies the length of the  array fieldIds. 
    /// @returns Returns 0 on error 1 otherwise.
	uint8_t fxSetStreamVariables(int devId, int* fieldIds, int n);

    /// \brief Start streaming data from a FlexSEA device.
    /// @param devId is the opaque handle for the device.
    /// @param frequency This is the frequency of updates. This value is in Hz and can be
    /// one of the following 1, 5, 10, 20, 33, 50, 100, 200, 300, 500, 1000.
    /// @param shouldLog If set true, the program logs all received data to a file. The name
    /// of the file is formed as follows:
    ///
    /// < FlexSEA model >_id< device ID >_< date and time >.csv
    ///
    /// for example:
    ///
    /// rigid_id3904_Tue_Nov_13_11_03_50_2018.csv
    ///
    /// The file is formatted as a CSV file. The first line of the file will be headers for
    /// all columns. Each line after that will contain the data read from the device.
    ///
    /// @param shouldAuto if set true, the device triggers itself to send read messages,
    /// otherwise the data is sent at the requested frequency.
    /// @returns Returns 0 on error. Otherwise returns 1.
	uint8_t fxStartStreaming(int devId, int frequency, bool shouldLog, int shouldAuto);

    /// \brief Stop streaming data from a FlexSEA device.
    /// @param devId is the opaque handle for the device.
    /// @returns 0 on success. Otherwise returns 1.
	uint8_t fxStopStreaming(int devId);

    /// \brief a utility function to access the most recent values received from a FlexSEA device.
    /// @param devId is the opaque handle for the device.
    /// @param fieldIds Specify the field ids of variables to read. These must have been requested
    /// in the fxSetStreamVariables. Refer to http://dephy.com/wiki/flexsea/doku.php?id=fxdevicefields
    /// for a description of the field ids.
    /// @param success An array of status codes indicating whether the returned data aray contains
    /// valid data. This array must contain the same number of elements as the fieldIds array.
    /// Each element in the success array will contain 1 if that fieldIds element contains valid
    /// data. Otherwise, the success array element will contain 0.
    /// @param n Specifies the length of the arrays fieldIds and success. The arrays must be preallocated
    /// and the size of the arrays must match.
    /// @returns Returns a pointer to an array that contains the data being read.
    /// @note This function will fail if the device is not configured to stream the requested field. Refer
    /// to fxSetStreamVariables().
    ///
    /// @note fieldIds and success arrays must be pre-allocated with enough space to hold all of the
    /// variables being streamed from the FlexSEA device.
    ///
    /// @note The returned data array is reused by each call to fxReadDevice. If you need it to persist,
    /// copy the data out of the array.
    int* fxReadDevice(int devId, int* fieldIds, uint8_t* success, int n);

    ///\brief This routine is meant to replace the fxReadDevice function. It takes an buffer for the data
    /// to be returned. 
    /// @param devId is the opaque handle for the device.
    /// @param fieldIds Specify the field ids of variables to read. These must have been requested
    /// in the fxSetStreamVariables. Refer to http://dephy.com/wiki/flexsea/doku.php?id=fxdevicefields
    /// for a description of the field ids.
    /// @param success An array of status codes indicating whether the returned data aray contains
    /// valid data. This array must contain the same number of elements as the fieldIds array.
    /// Each element in the success array will contain 1 if that fieldIds element contains valid
    /// data. Otherwise, the success array element will contain 0.
	/// @param This array will contain the data being returned. The user is reposible for allocating and
	/// disposal of the memory.
    /// @param n Specifies the length of the arrays fieldIds, success array, and bufferData array. The arrays must be preallocated
    /// and the size of the arrays must match.
    /// @returns Returns a pointer to an array that contains the data being read.
    /// @note This function will fail if the device is not configured to stream the requested field. Refer
    /// to fxSetStreamVariables().
    ///
    /// @note fieldIds and success arrays must be pre-allocated with enough space to hold all of the
    /// variables being streamed from the FlexSEA device.
    ///
    int fxReadDeviceEx(int devId, int* fieldIds, uint8_t* success, int* dataBuffer, int n);

    // -----------------
    // Control functions
    // -----------------

    /// \brief Sets the type of control mode of the FlexSEA device. The modes are open voltage, current,
    /// position, and impedance.
    /// @param devId is the opaque handle for the device.
    /// @param ctrlMode Mode description can be found at: http://dephy.com/wiki/flexsea/doku.php?id=fxdevicemodes
    /// @returns Nothing.
	void setControlMode(int devId, int ctrlMode);

    /// \brief Sets the voltage being supplied to the motor. The voltage is specified in mV (milli-volts).
    /// @param devId is the opaque handle for the device.
    /// @param mV is the voltage in milli Volts.
    /// @returns Nothing.
    /// @note The control mode must be open voltage. Refer to fxSetControlMode().
	void setMotorVoltage(int devId, int mV);

    /// \brief Sets the current for the motor. The current is specified in mA (milli-amps).
    /// @param devId is the opaque handle for the device.
    /// @param cur Is the current in milli Amos.
    /// @returns Nothing.
    /// @note The control mode must be set to current. Refer to fxSetControlMode().
	void setMotorCurrent(int devId, int cur);

    /// \brief Sets the setpoint in encoder ticks.
    /// @param devId is the opaque handle for the device.
    /// @param pos Is the position.
    /// @returns Nothing.
    /// @note The control mode must be either position or impedance.
	void setPosition(int devId, int pos);

    /// \brief Sets the gains used by PID controllers on the FlexSEA device.
    ///  @param devId is the opaque handle for the device.
    ///  @param z_k : Damping factor (used for current in current control & position in position/impedance control)
    ///  @param z_b : Damping factor (used for current in current control & position in position/impedance control)
    ///  @param i_kp : Proportional (used for for the underlying current control within the impedance controller)
    ///  @param i_ki : Integral gain (used for for the underlying current control within the impedance controller)
    ///  @returns Nothing.
    void setZGains(int devId, int z_k, int z_b, int i_kp, int i_ki);

    /// \brief Enables or disables the user FSM 2 on the FlexSEA device.
    /// @param devId is the opaque handle for the device.
    /// @param on 1 Enables the FSM, 0 disables it.
    /// @returns Nothing.
    void actPackFSM2(int devId, int on);

    /// \brief Runs the internal calibration routine on the FlexSEA device. find poles procedure
    /// on the device.
    /// @param devId is the opaque handle for the device.
    /// @param block Forces the function to block until the operation complete.
    /// @returns Nothing.
    void findPoles(int devId, int block);

    /// \brief Return the revision information for the library as a string
    /// @param None.
    /// @returns a string that includes build date and time and 'GIT describe information'
    char* fxGetRevision( void );

#ifdef __cplusplus
}
#endif

#endif // COMWRAPPER_H
