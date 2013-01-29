/**
   @file iioadaptorplugin.cpp
   @brief Plugin for IioAdaptor

   <p>
   Copyright (C) 2009-2010 Nokia Corporation
   Copyright (C) 2012 Tuomas Kulve

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

#include "iioadaptorplugin.h"
#include "iioadaptor.h"
#include <sensormanager.h>

void IioAdaptorPlugin::Register(class Loader&)
{
    sensordLogD() << "registering iioadaptor";
    SensorManager& sm = SensorManager::instance();
    sm.registerDeviceAdaptor<IioAdaptor>("accelerometeradaptor");
}

Q_EXPORT_PLUGIN2(iioadaptor, IioAdaptorPlugin)
