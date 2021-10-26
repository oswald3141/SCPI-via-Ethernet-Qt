/*
A class for controlling a microwave signal generator with SCPI commands
sent through TCP/IP.
Tested with Keysight E8267D, R&S SMB100A, R&S SMBV100A and some other
Keysight (Agilent) anf R&S generators.

The code is distributed under The MIT License
Copyright (c) 2021 Andrey Smolyakov
    (andreismolyakow 'at' gmail 'punto' com)
See LICENSE for the complete license text
*/

#ifndef GENERATORDEVICE_H
#define GENERATORDEVICE_H

#include <QTcpSocket>
#include <QThread>

#include "scpidevice.h"

class GeneratorDevice : public ScpiDevice
{
public:
    GeneratorDevice(const QString &genIPaddr, unsigned int genport);
    ~GeneratorDevice();
    GeneratorDevice(const GeneratorDevice&) = delete;

    // Set carrier frequency
    void setFreq(double freq_Hz);
    // Set output power
    void setPow(double pow_dbm);
    // Turn off Automatic Level Control
    // (ALC usually fails on short pulses,
    // see generator's documentation)
    void ALCoff();
    // Set pulse width
    void setPW(double pw_us);
    // Set pulse repetition interval
    void setPRI(double pri_us);
    // Activate pulse modulator
    void activatePmod();
    // Activate output RF power
    void activateRFpow();
    // Deactivate RF power
    void deactivateRFpow();
    // Request static errors list
    void checkForStaticErrors();
    // Request regular errors list
    void checkForErrors();
    // Turn on/off generator's GUI
    // Tunring GUI off may improve perfomance when executing long
    // sequencies of commands
    void turnOffGui();
    void turnOnGui();

private:
    QString IDstring;     // the string returned by IDN?

    bool isKeysight = false;
    bool isRandS = false;
};

#endif // GENERATORDEVICE_H
