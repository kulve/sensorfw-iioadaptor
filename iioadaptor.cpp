/**
   @file iioadaptor.cpp
   @brief IioAdaptor based on SysfsAdaptor

   <p>
   Copyright (C) 2009-2010 Nokia Corporation
   Copyright (C) 2012 Tuomas Kulve
   Copyright (C) 2012 Srdjan Markovic

   @author Tuomas Kulve <tuomas@kulve.fi>

   Sensord is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License
   version 2.1 as published by the Free Software Foundation.

   Sensord is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with Sensord.  If not, see <http://www.gnu.org/licenses/>.
   </p>
*/
#include <errno.h>

#include <logging.h>
#include <config.h>
#include <datatypes/utils.h>

#include "iioadaptor.h"
#include <sensord/sysfsadaptor.h>
#include <sensord/deviceadaptorringbuffer.h>
#include <QTextStream>
#include <QDir>

#include <sensord/deviceadaptor.h>
#include "datatypes/orientationdata.h"

IioAdaptor::IioAdaptor(const QString& id) :
        SysfsAdaptor(id, SysfsAdaptor::IntervalMode, true)
{
    int i;

    sensordLogD() << "Creating IioAdaptor with id: " << id;

    dev_accl_ = sensorExists(IIO_ACCELEROMETER);
    dev_gyro_ = -1;/*sensorExists(IIO_GYROSCOPE);*/
    dev_magn_ = -1;/*sensorExists(IIO_MAGNETOMETER);*/

    if (dev_accl_ != -1) {
        iioAcclBuffer_ = new DeviceAdaptorRingBuffer<TimedXyzData>(1);
        QString desc = "Industrial I/O accelerometer (" +  devices_[dev_accl_].name +")";
        sensordLogD() << "Accelerometer found";
        setAdaptedSensor("accelerometer", desc, iioAcclBuffer_);
    }

    if (dev_gyro_ != -1) {
        iioGyroBuffer_ = new DeviceAdaptorRingBuffer<TimedXyzData>(1);
        QString desc = "Industrial I/O gyroscope (" + devices_[dev_gyro_].name +")";
        sensordLogD() << "Gyroscope found";
        setAdaptedSensor("gyroscope", desc, iioGyroBuffer_);
    }

    if (dev_magn_ != -1) {
        iioMagnBuffer_ = new DeviceAdaptorRingBuffer<TimedXyzData>(1);
        QString desc = "Industrial I/O magnetometer (" + devices_[dev_magn_].name +")";
        sensordLogD() << "Magnetometer found";
        setAdaptedSensor("magnetometer", desc, iioMagnBuffer_);
    }

    // Disable and then enable devices to make sure they allow changing settings
    for (i = 0; i < IIO_MAX_DEVICES; ++i) {
        if (dev_accl_ == i || dev_gyro_ == i || dev_magn_ == i) {
            deviceEnable(i, false);
            deviceEnable(i, true);
            addDevice(i);
        }
    }
    
    //introduceAvailableDataRange(DataRange(0, 65535, 1));
}


IioAdaptor::~IioAdaptor()
{
    if (iioAcclBuffer_)
        delete iioAcclBuffer_;
    if (iioGyroBuffer_)
        delete iioGyroBuffer_;
    if (iioMagnBuffer_)
        delete iioMagnBuffer_;
}

int IioAdaptor::addDevice(int device) {
    
    QDir dir(deviceGetPath(device));
    if (!dir.exists()) {
        sensordLogW() << "Directory ??? doesn't exists";
        return 1;
    }

    QStringList filters;
    filters << "*_raw";
    dir.setNameFilters(filters);

    QFileInfoList list = dir.entryInfoList();
    for (int i = 0; i < list.size(); ++i) {
        QFileInfo fileInfo = list.at(i);
	sensordLogT() << "adding device " << fileInfo.filePath() << " as channel" << device*IIO_MAX_DEVICE_CHANNELS+i;
        addPath(fileInfo.filePath(),device*IIO_MAX_DEVICE_CHANNELS+i);
    }
    
    return 0;
}

int IioAdaptor::sensorExists(int sensor)
{
    int i;

    for (i = 0; i < IIO_MAX_DEVICES; ++i) {
        QString path = deviceGetPath(i) + QString("/");
        switch (sensor) {
        case IIO_ACCELEROMETER:
            path.append(IIO_ACCELEROMETER_ENABLE);
            break;
        case IIO_GYROSCOPE:
            path.append(IIO_GYROSCOPE_ENABLE);
            break;
        case IIO_MAGNETOMETER:
            path.append(IIO_MAGNETOMETER_ENABLE);
            break;
        default:
            return -1;
        }

        if (QFile::exists(path))
            return i;
    }

    return -1;
}

QString IioAdaptor::deviceGetPath(int device)
{
    QString pathDevice = QString(IIO_SYSFS_BASE) + "/iio:device" + QString::number(device);

    return pathDevice;
}

bool IioAdaptor::deviceEnable(int device, int enable)
{
    QString pathEnable = deviceGetPath(device) + "/buffer/enable";
    QString pathLength = deviceGetPath(device) + "/buffer/length";

    if (enable) {
        // FIXME: should enable sensors for this device? Assuming enabled already
        devices_[device].name = deviceGetName(device);
        devices_[device].channels = scanElementsEnable(device, enable);
        sysfsWriteInt(pathLength, IIO_BUFFER_LEN);
        sysfsWriteInt(pathEnable, enable);
    } else {
        sysfsWriteInt(pathEnable, enable);
        scanElementsEnable(device, enable);
        // FIXME: should disable sensors for this device?
    }

    return true;
}

QString IioAdaptor::deviceGetName(int device)
{
    QString pathDeviceName = deviceGetPath(device) + "/name";

    return sysfsReadString(pathDeviceName);
}

bool IioAdaptor::sysfsWriteInt(QString filename, int val)
{

    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        sensordLogW() << "Failed to open " << filename;
        return false;
    }

    QTextStream out(&file);
    out << val << "\n";

    file.close();

	return true;
}

QString IioAdaptor::sysfsReadString(QString filename)
{

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        sensordLogW() << "Failed to open " << filename;
        return QString();
    }

    QTextStream in(&file);
    QString line = in.readLine();

    if (line.endsWith("\n")) {
        line.chop(1);
    }

    file.close();

	return line;
}

int IioAdaptor::sysfsReadInt(QString filename)
{
    QString string = sysfsReadString(filename);

    bool ok;
    int value = string.toInt(&ok);

    if (!ok) {
        sensordLogW() << "Failed to parse '" << string << "' to int from file " << filename;
    }

	return value;
}

// Return the number of channels
int IioAdaptor::scanElementsEnable(int device, int enable)
{
    QString elementsPath = deviceGetPath(device) + "/scan_elements";

    QDir dir(elementsPath);
    if (!dir.exists()) {
        sensordLogW() << "Directory " << elementsPath << " doesn't exists";
        return 0;
    }

    // Find all the *_en file and write 0/1 to it
    QStringList filters;
    filters << "*_en";
    dir.setNameFilters(filters);

    QFileInfoList list = dir.entryInfoList();
    for (int i = 0; i < list.size(); ++i) {
        QFileInfo fileInfo = list.at(i);

        if (enable) {
            QString base = fileInfo.filePath();
            // Remove the _en
            base.chop(3);

            int index = sysfsReadInt(base + "_index");
            int bytes = deviceChannelParseBytes(base + "_type");

            devices_[device].channel_bytes[index] = bytes;
        }

        sysfsWriteInt(fileInfo.filePath(), enable);
    }

    return list.size();
}


int IioAdaptor::deviceChannelParseBytes(QString filename)
{
    QString type = sysfsReadString(filename);

    if (type.compare("le:s16/16>>0") == 0) {
        return 2;
    } else if (type.compare("le:s32/32>>0") == 0) {
        return 4;
    } else if (type.compare("le:s64/64>>0") == 0) {
        return 8;
    } else {
        sensordLogW() << "ERROR: invalid type from file " << filename << ": " << type;
    }

    return 0;
}


void IioAdaptor::processSample(int fileId, int fd)
{
    char buf[256];
    int readBytes;
    int result;
    int channel = fileId%IIO_MAX_DEVICE_CHANNELS;
    int device = (fileId-channel)/IIO_MAX_DEVICE_CHANNELS;

    readBytes = read(fd, buf, sizeof(buf));

    if (readBytes <= 0) {
        sensordLogW() << "read():" << strerror(errno);
        return;
    }
    result=atoi(buf);
    sensordLogT() << "Read " << result << " from device " << device << ", channel " << channel;

    // FIXME: shouldn't hardcode
    // FIXME: Find a mapping between channels and actual devices (0, 1 and 2 are probably wrong)
    if (device == 0) {
        TimedXyzData* accl = iioAcclBuffer_->nextSlot();
        switch(channel){
	  case 0:
	    accl->x_=result;
	    break;
	    
	  case 1:
            accl->y_ = result;
	    break;
	    
	  case 2:
            accl->z_ = result;
	    break;
	    
	  default:
	    return;
	}
        iioAcclBuffer_->wakeUpReaders();
    }
}

bool IioAdaptor::setInterval(const unsigned int value, const int sessionId)
{
    if (mode() == SysfsAdaptor::IntervalMode)
        return SysfsAdaptor::setInterval(value, sessionId);

    sensordLogD() << "Ignoring setInterval for " << value;

    return true;
}

unsigned int IioAdaptor::interval() const
{
    int value = 100;
    sensordLogD() << "Returning dummy value in interval(): " << value;

    return value;
}


/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:4
   c-basic-offset:4
   End:
*/
