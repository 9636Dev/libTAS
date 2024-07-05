/*
    Copyright 2015-2024 Clément Gallet <clement.gallet@ens-lyon.org>

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

#include "isteamuser.h"

#include "logging.h"

namespace libtas {

char steamuserdir[2048] = "/NOTVALID";

void SteamSetUserDataFolder(std::string path)
{
    LOGTRACE(LCF_STEAM);
    strncpy(steamuserdir, path.c_str(), sizeof(steamuserdir)-1);
}

HSteamUser ISteamUser::GetHSteamUser()
{
    LOGTRACE(LCF_STEAM);
	return 1;
}

bool ISteamUser::BLoggedOn()
{
    LOGTRACE(LCF_STEAM);
    return false; // Required for N++ to pass the splash screen
}

CSteamID ISteamUser::GetSteamID()
{
    LOGTRACE(LCF_STEAM);
    return 1;
}

int ISteamUser::InitiateGameConnection( void *pAuthBlob, int cbMaxAuthBlob, CSteamID steamIDGameServer, unsigned int unIPServer, uint16_t usPortServer, bool bSecure )
{
    LOGTRACE(LCF_STEAM);

    if(!pAuthBlob || cbMaxAuthBlob < 1)
        return 0;

    *(char *)pAuthBlob = 0;
    return 1;
}

void ISteamUser::TerminateGameConnection( unsigned int unIPServer, uint16_t usPortServer )
{
    LOGTRACE(LCF_STEAM);
}

void ISteamUser::TrackAppUsageEvent( CGameID gameID, int eAppUsageEvent, const char *pchExtraInfo )
{
    LOGTRACE(LCF_STEAM);
}

bool ISteamUser::GetUserDataFolder( char *pchBuffer, int cubBuffer )
{
    LOGTRACE(LCF_STEAM);
    strncpy(pchBuffer, steamuserdir, cubBuffer-1);
    LOG(LL_DEBUG, LCF_STEAM, "user data folder = \"%s\".", steamuserdir);
    return true;
}

void ISteamUser::StartVoiceRecording()
{
    LOGTRACE(LCF_STEAM);
}

void ISteamUser::StopVoiceRecording()
{
    LOGTRACE(LCF_STEAM);
}

EVoiceResult ISteamUser::GetAvailableVoice( unsigned int *pcbCompressed, unsigned int *pcbUncompressed_Deprecated, unsigned int nUncompressedVoiceDesiredSampleRate_Deprecated)
{
    LOGTRACE(LCF_STEAM);
    return 3; //k_EVoiceResultNoData
}

EVoiceResult ISteamUser::GetVoice( bool bWantCompressed, void *pDestBuffer, unsigned int cbDestBufferSize, unsigned int *nBytesWritten, bool bWantUncompressed_Deprecated, void *pUncompressedDestBuffer_Deprecated, unsigned int cbUncompressedDestBufferSize_Deprecated, unsigned int *nUncompressBytesWritten_Deprecated, unsigned int nUncompressedVoiceDesiredSampleRate_Deprecated)
{
    LOGTRACE(LCF_STEAM);
    return 3; //k_EVoiceResultNoData
}

EVoiceResult ISteamUser::DecompressVoice( const void *pCompressed, unsigned int cbCompressed, void *pDestBuffer, unsigned int cbDestBufferSize, unsigned int *nBytesWritten, unsigned int nDesiredSampleRate )
{
    LOGTRACE(LCF_STEAM);
    return 3; //k_EVoiceResultNoData
}

unsigned int ISteamUser::GetVoiceOptimalSampleRate()
{
    LOGTRACE(LCF_STEAM);
    return 44100;
}

HAuthTicket ISteamUser::GetAuthSessionTicket( void *pTicket, int cbMaxTicket, unsigned int *pcbTicket )
{
    LOGTRACE(LCF_STEAM);
    *pcbTicket = 8;
    return 1;
}

EBeginAuthSessionResult ISteamUser::BeginAuthSession( const void *pAuthTicket, int cbAuthTicket, CSteamID steamID )
{
    LOGTRACE(LCF_STEAM);
    return 0; //k_EBeginAuthSessionResultOK;
}

void ISteamUser::EndAuthSession( CSteamID steamID )
{
    LOGTRACE(LCF_STEAM);
}

void ISteamUser::CancelAuthTicket( HAuthTicket hAuthTicket )
{
    LOGTRACE(LCF_STEAM);
}

}
