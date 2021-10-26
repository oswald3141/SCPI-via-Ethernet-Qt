/*
A class for working with SCPI-devices from Qt.

The class provides functions for sending arbitrary SCPI-queries and receiving
responses via TCP/IP. It also contains methods for sending SCPI queries common
for all SCP devices (reset, clear state, etc.).

It is advised not to create objects of this class. It's better to inherit from
it and provide some higher-level interface for your specific device type
(see GeneratorDevice class in this repo).

Qt's functions waitForXXX, as the documentation states, may fail randomly on
Windows. Therefore this class doesn't use them. Instead, it imitates their behaviour
through the combination of QEventLoop and QTimer. QEventLoop blocks the control flow
until a query is successfully sent (response received) or until the one-shot timer
indicates a timeout error. See sendQuery function for further details.

The code is distributed under The MIT License
Copyright (c) 2021 Andrey Smolyakov
    (andreismolyakow 'at' gmail 'punto' com)
See LICENSE for the complete license text
*/

#ifndef SCPIDEVICE_H
#define SCPIDEVICE_H

#include <QString>
#include <QTcpSocket>
#include <QHostAddress>
#include <QEventLoop>
#include <QTimer>
#include <memory>
#include <cassert>
#include <exception>

class ScpiDevice
{
public:

    // A general device error (unable to connect, invalid IP, etc.)
    struct ScpiDeviceError : public std::exception {
        explicit ScpiDeviceError(const QString &errstr) : errorDescription(errstr.toUtf8()) {}
        const char * what() const throw() override {
            return errorDescription.constData();}
        private: QByteArray errorDescription;
    };
    
    // An error perfirming a query (timeout, unexpected OPC response, etc.)
    struct ScpiQueryError : public ScpiDeviceError {
        explicit ScpiQueryError(const QString &errstr) : ScpiDeviceError(errstr) {}
    };

    void reset();       // Reset device
    void clearState();  // Clear device's state register
    QString getIDN();   // Request identification string
    void setTimeout(unsigned int scpitimeout_ms); // Set SCPI connection timeout

protected:
    ScpiDevice(const QString &devIPaddr,unsigned int devport);
    virtual ~ScpiDevice();
    void sendCommand(const QString &command);   // Sends a command with completion request
    QString sendQuery(const QString &query);    // Sends a query and recieves a response for it
    void connectToDevice();
    void disconnectFromDevice() noexcept;
    bool isConnected() noexcept;
    

    ScpiDevice(const ScpiDevice&) = delete;

private:
    QHostAddress IPaddr = QHostAddress("0.0.0.0");
    unsigned int port = 0;
    unsigned int timeout_ms = 1000;

    std::function<void(QObject*)> qobj_deleter = [](QObject* obj) {obj->deleteLater();};
    std::unique_ptr<QTcpSocket,decltype(qobj_deleter)> socket = nullptr;

    // Are used to imitate waitForXXX behaviour
    std::unique_ptr<QTimer,decltype(qobj_deleter)> timeoutTimer = nullptr;
    std::unique_ptr<QEventLoop,decltype(qobj_deleter)> localEventloop = nullptr;

    void setIpAddr(QString devIPaddr);
    void setPort(unsigned int devport);
};

#endif // SCPIDEVICE_H
