/*
See generatordevice.h for the description.

The code is distributed under The MIT License
Copyright (c) 2021 Andrey Smolyakov
    (andreismolyakow 'at' gmail 'punto' com)
See LICENSE for the complete license text
*/

#include "generatordevice.h"

GeneratorDevice::GeneratorDevice
    (const QString &genIPaddr, unsigned int genport) : 
        ScpiDevice(genIPaddr, genport) {
            
    connectToDevice();

    try {
        IDstring = getIDN();
    }  catch (ScpiDeviceError &err) {
        disconnectFromDevice();
        throw;
    }

    isKeysight = IDstring.contains("Agilent",Qt::CaseInsensitive) || 
                        IDstring.contains("Keysight",Qt::CaseInsensitive);
    isRandS = IDstring.contains("Rohde&Schwarz",Qt::CaseInsensitive);

    if ( !(isKeysight ||isRandS )) {
        disconnectFromDevice();
        throw ScpiDeviceError(
            QString("The generatorn isn't supported. Try to activate SCPI \
                interpreter compatible with on of the Rohde & Schwarz or \
                    Keysight/Agilent generators. Usually such an option is \
                        available in the generator settings.") );
    }
}

GeneratorDevice::~GeneratorDevice() {
    disconnectFromDevice();
}

void GeneratorDevice::setFreq(double freq_Hz) {
    sendCommand(QString(":FREQ %1Hz").arg(freq_Hz,0,'f',0));
}

void GeneratorDevice::setPow(double pow_dbm) {
    sendCommand(QString(":POW %1dbm").arg(pow_dbm,0,'f',2));
}

void GeneratorDevice::ALCoff() {
    sendCommand(":POW:ALC OFF");
}

void GeneratorDevice::setPW(double pw_us) {
    if (isRandS)
        sendCommand(QString(":PULM:WIDT %1uS").arg(pw_us,0,'f',2));
    else if ( isKeysight )
        sendCommand(QString(":PULM:INT:PWID %1uS").arg(pw_us,0,'f',2));
}

void GeneratorDevice::setPRI(double pri_us) {
    if (isRandS)
        sendCommand(QString(":PULM:PER %1uS").arg(pri_us,0,'f',2));
    else if ( isKeysight )
        sendCommand(QString(":PULM:INT:PER %1uS").arg(pri_us,0,'f',2));
}

void GeneratorDevice::activatePmod() {
    sendCommand(":PULM:STAT ON");
}

void GeneratorDevice::deactivateRFpow() {
    sendCommand(":OUTP:STAT OFF");
}

void GeneratorDevice::activateRFpow() {
    sendCommand(":OUTP:STAT ON");
}

void GeneratorDevice::checkForStaticErrors() {
    if( !isRandS )
        return;

    QString static_errors = sendQuery("SYST:SERR?");
    QStringList err_codes_descrs = static_errors.split(",");
    if ( err_codes_descrs.at(0).toInt() != 0 )
        throw ScpiDeviceError(static_errors);
}

void GeneratorDevice::checkForErrors() {
    QString errors = "";
    try {
        errors = sendQuery("SYST:ERR?");
    }  catch (ScpiDeviceError &err) {
        throw ScpiQueryError(QString
            ("Unable to request the errors list (%1)").arg(err.what()));
    }

    QStringList err_codes_descrs = errors.split(",");
    if ( err_codes_descrs.at(0).toInt() != 0 )
        throw ScpiDeviceError(errors);
}

void GeneratorDevice::turnOffGui() {
    sendCommand("SYST:DISP:UPD OFF");
}

void GeneratorDevice::turnOnGui() {
    sendCommand("SYST:DISP:UPD ON");
}
