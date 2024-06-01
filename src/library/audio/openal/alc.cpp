/*
    Copyright 2015-2023 Clément Gallet <clement.gallet@ens-lyon.org>

    This file is part of libTAS.

    libTAS is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    libTAS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with libTAS.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "alc.h"
#include "alsoft.h"
#include "efx.h"
#include "alext.h"

#include "audio/AudioContext.h"
#include "logging.h"
#include "global.h"

namespace libtas {

DEFINE_ORIG_POINTER(alcOpenDevice)
DEFINE_ORIG_POINTER(alcCloseDevice)

DEFINE_ORIG_POINTER(alcCreateContext)
DEFINE_ORIG_POINTER(alcMakeContextCurrent)
DEFINE_ORIG_POINTER(alcProcessContext)
DEFINE_ORIG_POINTER(alcSuspendContext)
DEFINE_ORIG_POINTER(alcDestroyContext)
DEFINE_ORIG_POINTER(alcGetCurrentContext)
DEFINE_ORIG_POINTER(alcGetContextsDevice)

DEFINE_ORIG_POINTER(alcGetError)

DEFINE_ORIG_POINTER(alcIsExtensionPresent)
DEFINE_ORIG_POINTER(alcGetProcAddress)
DEFINE_ORIG_POINTER(alcGetEnumValue)

DEFINE_ORIG_POINTER(alcGetString)
DEFINE_ORIG_POINTER(alcGetIntegerv)

static ALCdevice dummyDevice = 0;
static ALCcontext dummyContext = -1;
static ALCcontext currentContext = -1;

static ALCenum alcError = ALC_NO_ERROR;
#define ALCSETERROR(error) if(alcError==ALC_NO_ERROR) alcError = error

/* Override */ ALCenum alcGetError(ALCdevice *device)
{
    LOGTRACE(LCF_SOUND);
    CHECK_USE_ALSOFT_FUNCTION(alcGetError, device)

    ALCenum err = alcError;
    alcError = ALC_NO_ERROR;
    return err;
}

/* Override */ ALCdevice* alcOpenDevice(const ALCchar* devicename)
{
    LOGTRACE(LCF_SOUND);
    CHECK_USE_ALSOFT_FUNCTION(alcOpenDevice, devicename)

    Global::game_info.audio |= GameInfo::OPENAL;
    Global::game_info.tosend = true;
    return &dummyDevice;
}

/* Override */ ALCboolean alcCloseDevice(ALCdevice* deviceHandle)
{
    LOGTRACE(LCF_SOUND);
    CHECK_USE_ALSOFT_FUNCTION(alcCloseDevice, deviceHandle)

    return ALC_TRUE;
}

/* Override */ ALCcontext* alcCreateContext(ALCdevice *device, const ALCint* attrlist)
{
    LOGTRACE(LCF_SOUND);
    CHECK_USE_ALSOFT_FUNCTION(alcCreateContext, device, attrlist)

    if (Global::shared_config.audio_disabled)
        return nullptr;

    if (dummyContext != -1) {
        LOG(LL_DEBUG, LCF_SOUND | LCF_TODO, "We don't support multiple openAL contexts yet");
        return NULL;
    }
    if (attrlist) {
        for (int attr = 0; attrlist[attr] != 0; attr+=2) {
            LOG(LL_DEBUG, LCF_SOUND, "Attribute %d is %d", attrlist[attr], attrlist[attr+1]);
        }
    }

    dummyContext = 0;
    return &dummyContext;
}

/* Override */ ALCboolean alcMakeContextCurrent(ALCcontext *context)
{
    LOGTRACE(LCF_SOUND);
    CHECK_USE_ALSOFT_FUNCTION(alcMakeContextCurrent, context)

    if (context == NULL) {
        currentContext = -1;
        return ALC_TRUE;
    }

    if (*context != 0) {
        ALCSETERROR(ALC_INVALID_CONTEXT);
        return ALC_FALSE;
    }
    currentContext = *context;
    return ALC_TRUE;
}

/* Override */ void alcProcessContext(ALCcontext *context)
{
    LOGTRACE(LCF_SOUND);
    CHECK_USE_ALSOFT_FUNCTION(alcProcessContext, context)

    if (context == NULL)
        ALCSETERROR(ALC_INVALID_CONTEXT);
    if (*context != dummyContext)
        ALCSETERROR(ALC_INVALID_CONTEXT);
}

/* Override */ void alcSuspendContext(ALCcontext *context)
{
    LOGTRACE(LCF_SOUND);
    CHECK_USE_ALSOFT_FUNCTION(alcSuspendContext, context)

    if (context == NULL)
        ALCSETERROR(ALC_INVALID_CONTEXT);
    if (*context != dummyContext)
        ALCSETERROR(ALC_INVALID_CONTEXT);
}

/* Override */ void alcDestroyContext(ALCcontext *context)
{
    LOGTRACE(LCF_SOUND);
    CHECK_USE_ALSOFT_FUNCTION(alcDestroyContext, context)

    if (context == NULL)
        ALCSETERROR(ALC_INVALID_CONTEXT);
    if (*context == dummyContext) {
        if (*context == currentContext) {
            ALCSETERROR(ALC_INVALID_VALUE);
        }
        else {
            dummyContext = -1;
        }
    }
    else
        ALCSETERROR(ALC_INVALID_CONTEXT);
}

/* Override */ ALCcontext* alcGetCurrentContext(void)
{
    LOGTRACE(LCF_SOUND);

    /* Can't use CHECK_USE_ALSOFT_FUNCTION :(
     * "ISO C++11 requires at least one argument for the "..." in a variadic macro"
     */
    if (Global::shared_config.openal_soft && check_al_soft_available()) {
        LINK_NAMESPACE_ALSOFT(alcGetCurrentContext);
        return orig::alcGetCurrentContext();
    }

    if (currentContext == -1)
        return NULL;
    else
        return &dummyContext;

}

/* Override */ ALCdevice* alcGetContextsDevice(ALCcontext *context)
{
    LOGTRACE(LCF_SOUND);
    CHECK_USE_ALSOFT_FUNCTION(alcGetContextsDevice, context)

    return &dummyDevice;
}

/* Override */ ALCboolean alcIsExtensionPresent(ALCdevice *device, const ALCchar *extname)
{
    LOGTRACE(LCF_SOUND);
    CHECK_USE_ALSOFT_FUNCTION(alcIsExtensionPresent, device, extname)

    if (extname == NULL) {
        ALCSETERROR(ALC_INVALID_VALUE);
        return ALC_FALSE;
    }

    LOG(LL_DEBUG, LCF_SOUND, "Extension asked is %s", extname);

    if (strcmp(extname, "ALC_ENUMERATION_EXT") == 0) {
        return ALC_TRUE;
    }

    if (strcmp(extname, "ALC_ENUMERATE_ALL_EXT") == 0) {
        return ALC_TRUE;
    }

    if (strcmp(extname, "ALC_EXT_CAPTURE") == 0) {
        return ALC_TRUE;
    }

    if (strcmp(extname, "ALC_SOFT_HRTF") == 0) {
        return ALC_FALSE;
    }

    LOG(LL_ERROR, LCF_SOUND, "Extension %s not supported, but we will still return yes because some games crash if we return no", extname);
    return ALC_TRUE;
}

/* Little helper macro */
#define CHECK_RETURN_MY_FUNCTION(name) \
    if (strcmp(funcname, #name) == 0) { \
        return reinterpret_cast<void*>(my##name); \
    }

/* Override */ void* alcGetProcAddress(ALCdevice *device, const ALCchar *funcname)
{
    LOGTRACE(LCF_SOUND);
    CHECK_USE_ALSOFT_FUNCTION(alcGetProcAddress, device, funcname)

    if (funcname == NULL) {
        ALCSETERROR(ALC_INVALID_VALUE);
        return NULL;
    }

    CHECK_RETURN_MY_FUNCTION(alcSetThreadContext)
    CHECK_RETURN_MY_FUNCTION(alcGetThreadContext)
    CHECK_RETURN_MY_FUNCTION(alcLoopbackOpenDeviceSOFT)
    CHECK_RETURN_MY_FUNCTION(alcIsRenderFormatSupportedSOFT)
    CHECK_RETURN_MY_FUNCTION(alcRenderSamplesSOFT)
    CHECK_RETURN_MY_FUNCTION(alcGetStringiSOFT)
    CHECK_RETURN_MY_FUNCTION(alcResetDeviceSOFT)
    CHECK_RETURN_MY_FUNCTION(alcDevicePauseSOFT)
    CHECK_RETURN_MY_FUNCTION(alcDeviceResumeSOFT)
    
    LOG(LL_ERROR, LCF_SOUND, "Requesting function %s", funcname);

    return NULL;
}

/* Override */ ALCenum alcGetEnumValue(ALCdevice *device, const ALCchar *enumname)
{
    LOGTRACE(LCF_SOUND);
    CHECK_USE_ALSOFT_FUNCTION(alcGetEnumValue, device, enumname)

    if (enumname == NULL) {
        ALCSETERROR(ALC_INVALID_VALUE);
        return 0;
    }

    LOG(LL_ERROR, LCF_SOUND, "Requesting enum %s", enumname);
    return 0;
}

static const ALCchar* alcNoErrorStr = "No error";
static const ALCchar* alcInvalidDeviceStr = "Invalid device";
static const ALCchar* alcInvalidContextStr = "Invalid context";
static const ALCchar* alcInvalidEnumStr = "Invalid enum";
static const ALCchar* alcInvalidValueStr = "Invalid value";
static const ALCchar* alcOutOfMemoryStr = "Out of memory";
static const ALCchar* alcExtensionsStr = "ALC_ENUMERATION_EXT ALC_ENUMERATE_ALL_EXT ALC_EXT_CAPTURE"; // extensions strings separated by space
static const ALCchar* alcDeviceListStr = "libTAS device\0"; // must be double null-terminated
static const ALCchar* alcDeviceStr = "libTAS device";
static const ALCchar* alcCaptureListStr = "\0"; // must be double null-terminated
static const ALCchar* alcDefault = "";

/* Override */ const ALCchar* alcGetString(ALCdevice *device, ALCenum param)
{
    LOG(LL_TRACE, LCF_SOUND, "%s call with param %d", __func__, param);
    CHECK_USE_ALSOFT_FUNCTION(alcGetString, device, param)

    switch (param) {
        case ALC_DEFAULT_DEVICE_SPECIFIER:
            LOG(LL_DEBUG, LCF_SOUND, "Request default device");
            return alcDeviceStr;

        case ALC_DEVICE_SPECIFIER:
            if (device == NULL) {
                LOG(LL_DEBUG, LCF_SOUND, "Request list of available devices");
                return alcDeviceListStr;
            }
            else {
                LOG(LL_DEBUG, LCF_SOUND, "Request current device");
                return alcDeviceStr;
            }

        case ALC_DEFAULT_ALL_DEVICES_SPECIFIER:
            LOG(LL_DEBUG, LCF_SOUND, "Request default all device");
            return alcDeviceStr;

        case ALC_ALL_DEVICES_SPECIFIER:
            if (device == NULL) {
                LOG(LL_DEBUG, LCF_SOUND, "Request list of available all devices");
                return alcDeviceListStr;
            }
            else {
                LOG(LL_DEBUG, LCF_SOUND, "Request current device");
                return alcDeviceStr;
            }

        case ALC_EXTENSIONS:
            LOG(LL_DEBUG, LCF_SOUND, "Request list of supported extensions");
            if (device == NULL) {
                ALCSETERROR(ALC_INVALID_DEVICE);
                return NULL;
            }
            return alcExtensionsStr;

        case ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER:
            LOG(LL_DEBUG, LCF_SOUND, "Request default capture device");
            return NULL;
        case ALC_CAPTURE_DEVICE_SPECIFIER:
            if (device == NULL) {
                LOG(LL_DEBUG, LCF_SOUND, "Request list of available capture devices");
                return alcCaptureListStr;
            }
            else {
                LOG(LL_DEBUG, LCF_SOUND, "Request current capture device");
                return NULL;
            }

        /* Request error strings */
        case ALC_NO_ERROR:
            return alcNoErrorStr;
        case ALC_INVALID_DEVICE:
            return alcInvalidDeviceStr;
        case ALC_INVALID_CONTEXT:
            return alcInvalidContextStr;
        case ALC_INVALID_ENUM:
            return alcInvalidEnumStr;
        case ALC_INVALID_VALUE:
            return alcInvalidValueStr;
        case ALC_OUT_OF_MEMORY:
            return alcOutOfMemoryStr;
        default:
            return alcDefault;
    }
}

/* Override */ void alcGetIntegerv(ALCdevice *device, ALCenum param, ALCsizei size, ALCint *values)
{
    LOGTRACE(LCF_SOUND);
    CHECK_USE_ALSOFT_FUNCTION(alcGetIntegerv, device, param, size, values)

    if (values == NULL)
        return;
    if (size == 0)
        return;

    switch (param) {
        case ALC_FREQUENCY:
            LOG(LL_DEBUG, LCF_SOUND | LCF_TODO, "Request frequency");
            values[0] = 44100;
            return;
        case ALC_REFRESH:
            LOG(LL_DEBUG, LCF_SOUND | LCF_TODO, "Request refresh");
            values[0] = 60;
            return;
        case ALC_SYNC:
            LOG(LL_DEBUG, LCF_SOUND | LCF_TODO, "Request sync");
            values[0] = 0;
            return;
        case ALC_MONO_SOURCES:
            LOG(LL_DEBUG, LCF_SOUND | LCF_TODO, "Request mono sources");
            values[0] = 255;
            return;
        case ALC_STEREO_SOURCES:
            LOG(LL_DEBUG, LCF_SOUND | LCF_TODO, "Request stereo sources");
            values[0] = 255;
            return;
        case ALC_ATTRIBUTES_SIZE:
            LOG(LL_DEBUG, LCF_SOUND | LCF_TODO, "Request attributes size");
            //if (device == NULL)
            values[0] = 13;
            return;
        case ALC_ALL_ATTRIBUTES:
            LOG(LL_DEBUG, LCF_SOUND | LCF_TODO, "Request all attributes");
            values[0] = ALC_FREQUENCY;
            values[1] = 44100;
            values[2] = ALC_REFRESH;
            values[3] = 60;
            values[4] = ALC_SYNC;
            values[5] = 0;
            values[6] = ALC_MONO_SOURCES;
            values[7] = 255;
            values[8] = ALC_STEREO_SOURCES;
            values[9] = 255;
            values[10] = ALC_MAX_AUXILIARY_SENDS;
            values[11] = 2;
            values[12] = 0;
            return;
        case ALC_MAJOR_VERSION:
            LOG(LL_DEBUG, LCF_SOUND, "Request major version");
            values[0] = 1;
            return;
        case ALC_MINOR_VERSION:
            LOG(LL_DEBUG, LCF_SOUND, "Request minor version");
            values[0] = 1;
            return;
        case ALC_CAPTURE_SAMPLES:
            LOG(LL_DEBUG, LCF_SOUND | LCF_TODO, "Request capture samples");
            values[0] = 0;
            return;
        case ALC_MAX_AUXILIARY_SENDS:
            LOG(LL_DEBUG, LCF_SOUND | LCF_TODO, "Request max auxiliary sends");
            values[0] = 2;
            return;
        case ALC_NUM_HRTF_SPECIFIERS_SOFT:
            LOG(LL_DEBUG, LCF_SOUND | LCF_TODO, "Request number of HRTFs");
            values[0] = 0;
            return;
        case ALC_HRTF_SOFT:
            LOG(LL_DEBUG, LCF_SOUND | LCF_TODO, "Request HRTF state");
            values[0] = ALC_FALSE;
            return;
        case ALC_HRTF_STATUS_SOFT:
            LOG(LL_DEBUG, LCF_SOUND | LCF_TODO, "Request HRTF status");
            values[0] = ALC_HRTF_DISABLED_SOFT;
            return;
        default:
            LOG(LL_DEBUG, LCF_SOUND | LCF_TODO, "Unknown param %d", param);
            values[0] = 2;
            return;
    }
}

}
