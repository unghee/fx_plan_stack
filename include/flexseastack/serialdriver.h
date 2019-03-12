#ifndef SERIALDRIVER_H
#define SERIALDRIVER_H

#include <serial/serial.h>

#include <vector>
#include <string>
#include <mutex>

/// /brief class that handles thread-safe management of n serial ports
///
/// SerialDriver wraps the libserialc library (serial/serial.h)
/// and uses mutexes to enforce thread safety
class SerialDriver
{
public:
    explicit SerialDriver(int n);
    virtual ~SerialDriver();

    /// \brief returns a list of serial ports available on your computer
    virtual std::vector<std::string> getAvailablePorts() const;

    /// \brief returns 0 if the port is closed, 1 if the port is open
    virtual int isOpen(uint16_t portIdx) const;

    /// \brief closes the corresponding port (if it is open)
    /// throws std::out_of_range for invalid portIdx
    virtual void tryClose(uint16_t portIdx);
    virtual void close(uint16_t portIdx) {tryClose(portIdx);}

    /// \brief writes to the corresponding port
    /// @param serial_tx_data is an array of data to write
    /// @param bytes_to_send is the length of the array serial_tx_data
    /// throws std::out_of_range for invalid portIdx
    virtual void write(uint8_t bytes_to_send, uint8_t *serial_tx_data, uint16_t portIdx);

    /// \brief tries to force the serials rx/tx lines to push through any buffered data
    /// throws std::out_of_range for invalid portIdx
    virtual void flush(uint16_t portIdx);

    /// \brief empties the serials rx/tx lines (buffered data will not get pushed through)
    /// throws std::out_of_range for invalid portIdx
    virtual void clear(uint16_t portIdx);

    /// \brief returns the currently opened port's name (if the port is open)
    /// throws std::out_of_range for invalid portIdx
    std::string getPortName(uint16_t portIdx) const;

    /// \brief returns the current port state (an enum)
    /// throws std::out_of_range for invalid portIdx
    virtual serial::state_t getPortState(int portIdx) const;

protected:
    int numPortsOpen() const {return openPorts;}

    /// \brief closes the corresponding port (if it is open)
    /// throws std::out_of_range for invalid portIdx
    virtual bool tryOpen(const std::string &portName, uint16_t portIdx);

    /// \brief returns the number of bytes available for reading from the corresponding port (if it is open)
    /// throws std::out_of_range for invalid portIdx
    size_t bytesAvailable(int portIdx) const;

    /// \brief reads "nb" bytes from port at "portIdx" into output buffer "buf"
    /// throws std::out_of_range for invalid portIdx
    size_t readPort(int portIdx, uint8_t *buf, uint16_t nb);

private:
    const int _NUMPORTS;
    serial::Serial *ports;
    std::mutex *serialMutexes;
    bool *isPortOpen;

    mutable std::mutex _portCountMutex;
    mutable uint16_t openPorts;

    friend class TestSerial;
};

#endif // SERIALDRIVER_H
