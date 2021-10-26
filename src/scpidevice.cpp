/*
See scpidevice.h for the description.

The code is distributed under The MIT License
Copyright (c) 2021 Andrey Smolyakov
    (andreismolyakow 'at' gmail 'punto' com)
See LICENSE for the complete license text
*/

#include "scpidevice.h"

ScpiDevice::ScpiDevice(const QString &devIPaddr, unsigned int devport) {
    setIpAddr(devIPaddr);
    setPort(devport);

    socket = std::unique_ptr<QTcpSocket,decltype(qobj_deleter)> (new QTcpSocket(nullptr), qobj_deleter);
    timeoutTimer = std::unique_ptr<QTimer,decltype(qobj_deleter)> (new QTimer(nullptr),qobj_deleter);
    localEventloop = std::unique_ptr<QEventLoop,decltype(qobj_deleter)> (new QEventLoop(nullptr), qobj_deleter);

    // We don't need a cyclic timer
    timeoutTimer->setSingleShot(true);
}

ScpiDevice::~ScpiDevice() {
    disconnectFromDevice();
}

void ScpiDevice::connectToDevice() {

    disconnectFromDevice();

    QObject::connect( socket.get(), &QTcpSocket::connected, localEventloop.get(), &QEventLoop::quit );
    QObject::connect( timeoutTimer.get(), &QTimer::timeout, localEventloop.get(), &QEventLoop::quit);

    socket->connectToHost(IPaddr,port);
    timeoutTimer->start(timeout_ms);
    localEventloop->exec();

    if( !timeoutTimer->isActive() )
        throw ScpiDeviceError("Error connecting to a device. Timeout expired.");
    return;
}

void ScpiDevice::reset() {
    try {
        sendCommand("*RST;");
    }  catch (ScpiQueryError &err) {
        throw ScpiQueryError(QString("Unable to reset the device: %1").arg(err.what()));
    }
}

void ScpiDevice::clearState() {
    try {
        sendCommand("*CLS;");
    }  catch (ScpiQueryError &err) {
        throw ScpiQueryError(QString("Unable to clear the device's state register: %1").arg(err.what()));
    }
}

void ScpiDevice::disconnectFromDevice() noexcept {
    if (!socket->isValid())
        return;

    if (socket->state() == QAbstractSocket::UnconnectedState)
        return;

    socket->disconnectFromHost();
}

bool ScpiDevice::isConnected() noexcept {
    if (socket->isValid() && socket->state() == QTcpSocket::ConnectedState)
        return true;
    else
        return false;
}

QString ScpiDevice::getIDN() {
    try {
        return sendQuery("*IDN?");
    }  catch (ScpiQueryError &err) {
        throw ScpiQueryError(QString("Unable to get an ID string: %1").arg(err.what()));
    }
}

void ScpiDevice::sendCommand(const QString &command) {
    /* The command gets extended with the request of its completion (OPerationComplete?)
    and sended as a query (it's fine). A device will return 1 onlu after the command
    will be actually executed. */
    QString status = sendQuery(QString("%1; *OPC?").arg(command));
    if( status.toInt() != 1 )
        throw ScpiQueryError("The device has returned an unexpected OPC code (not 1).");
}

QString ScpiDevice::sendQuery(const QString &query) {

    if (!socket->isValid() || socket->state() != QTcpSocket::ConnectedState)
        throw ScpiDeviceError("Incorrect socket state (invalid or not connected).");

    // Extend a query with the terminating symbol
    QString query_eol = QString("%1\n").arg(query);

    QObject::connect( socket.get(), &QTcpSocket::bytesWritten, localEventloop.get(), &QEventLoop::quit );
    QObject::connect(timeoutTimer.get(), &QTimer::timeout, localEventloop.get(), &QEventLoop::quit);

    // Send a query, wait for the sending completion
    socket->write(query_eol.toLocal8Bit());
    timeoutTimer->start(timeout_ms);
    localEventloop->exec();

    if(!timeoutTimer->isActive())
        throw ScpiQueryError("Sending timeout expired.");


    QObject::disconnect( socket.get(), &QTcpSocket::bytesWritten, localEventloop.get(), &QEventLoop::quit );
    QObject::connect( socket.get(), &QTcpSocket::readyRead, localEventloop.get(), &QEventLoop::quit );

    // Wait for the device ready to send a response
    timeoutTimer->start(timeout_ms);
    localEventloop->exec();

    if(!timeoutTimer->isActive())
        throw ScpiQueryError("Receiving timeout expired.");

    // Принимаем ответ
    return QString(socket->readAll());
}

void ScpiDevice::setIpAddr(QString devIPaddr) {
    if (!IPaddr.setAddress(devIPaddr))
        throw ScpiDeviceError("Invalid IP address.");
}

void ScpiDevice::setPort(unsigned int devport) {
    if (devport == 0 || devport > 65535)
        throw ScpiDeviceError("Invalid TCP port.");
    else
        port = devport;
}

void ScpiDevice::setTimeout(unsigned int scpitimeout) {
    // It's risky to set a big timeout. The program may hang, waiting for it.
    // Usually 2-3 seconds are more than enough.
    if (scpitimeout> 100000)
        throw ScpiDeviceError("The SCPI timeout is too large.");
    else
        timeout_ms = scpitimeout;
}
