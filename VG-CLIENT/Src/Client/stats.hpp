#include "../common.hpp"
#pragma once
#include "bnet_auth.hpp"
#include "offsets.hpp"

namespace s4 {
    enum class ScenarioInstallState {
        NoOwnership = 0x0,
        Available = 0x1,
        Installing = 0x2,
        Installed = 0x3,
    };
}

enum voiceCommunicationSetting
{
	TALK_TO_EVERYONE = 0x0,
	TALK_TO_FRIENDS_ONLY = 0x1,
	TALK_TO_NOBODY_I_HATE_EVERYONE = 0x2,
	NUM_VOICECOMMUNICATIONSETTINGS = 0x3,
};

enum class StatsSource {
	STATS_ONLINE = 0x0,
	STATS_OFFLINE = 0x1,
	STATS_COUNT = 0x2
};

struct XenonUserData
{
	int signinState;
	char gamertag[36];
	unsigned int gamertagSuffix;
	int hashOfGamertag;
	char platformGamertag[36];
	char fullGamertag[64];
	uint64_t xuid;
	char xuidString[21];
	unsigned __int64 platformId;
	char platformIdString[21];
	bool isGuest;
	enum voiceCommunicationSetting voiceCommunicationSetting;
	int lastOnlinePlayPrivilegeCheckTime;
	unsigned __int8 userPrivileges;
	unsigned __int8 privilegeCheckStates;
	bool isReputationSet;

	void AssignUsername(const char* username);
	void AssignXUID(uint64_t xuid);
	uint32_t HashGamerTag(const char* str);
	void Auth();
};

inline uint32_t XenonUserData::HashGamerTag(const char* str) {  
	uint32_t hash = 0;

	if (str) {
		while (*str) {
			hash = hash * 30 + (unsigned char)(*str);
			++str;
		}
	}

	return hash;
}

inline void XenonUserData::AssignUsername(const char* username) {
	XenonUserData* profile = this;
	strncpy_s(profile->gamertag, sizeof(profile->gamertag), username, _TRUNCATE);
	strncpy_s(profile->platformGamertag, sizeof(profile->platformGamertag), username, _TRUNCATE);
	strncpy_s(profile->fullGamertag, sizeof(profile->fullGamertag), username, _TRUNCATE);
	this->hashOfGamertag = HashGamerTag(username);
 
}

inline void XenonUserData::AssignXUID(uint64_t XUID)
{
	XenonUserData* profile = this;
	std::string xuid_string = std::to_string(XUID);
	profile->xuid = XUID;
	strncpy_s(profile->xuidString, xuid_string.c_str(), sizeof(profile->xuidString));
	profile->platformId = XUID;
	strncpy_s(profile->platformIdString, xuid_string.c_str(), sizeof(profile->platformIdString));

}

inline XenonUserData* unkw_Get_UserData(int controllerIndex)
{
	auto func = reinterpret_cast<XenonUserData * (*)(int controllerIndex)>(s4::offsets::Resolve(s4::offsets::LocalUserData_GetByController));  
	return func(controllerIndex);
}


inline void Assign_UsernameAndXUID(const char* username, uint64_t XUID)
{
	XenonUserData* userData = unkw_Get_UserData(0);
    if (!userData) throw std::runtime_error("Local user data is not ready");

	userData->AssignUsername(username);  
	userData->AssignXUID(XUID);  
	userData->signinState = static_cast<int>(s4::CoDAuthState::AUTHSTATE_SUCCESS);
}

inline void SignIn()
{
	XenonUserData* userData = unkw_Get_UserData(0);
    if (!userData) throw std::runtime_error("Local user data is not ready");
	userData->signinState = static_cast<int>(s4::CoDAuthState::AUTHSTATE_SUCCESS);
}

struct lol
{
	char paddingp[51083];
};

 
struct CommonDataBlob
{
	unsigned __int8 checksum[4];
	unsigned __int8 data[4892];
};

 
struct MPDataBlob
{
	unsigned __int8 checksum[4];
	unsigned __int8 data[15908];
};

 
struct RankedLoadoutsBlob
{
	unsigned __int8 checksum[4];
	unsigned __int8 data[4892];
};

 
struct PrivateLoadoutsBlob
{
	unsigned __int8 checksum[4];
	unsigned __int8 data[4892];
};

 
struct CPDataBlob
{
	unsigned __int8 checksum[4];
	unsigned __int8 data[14684];
};

 
struct NonGameDataBlob
{
	unsigned __int8 checksum[4];
	unsigned __int8 data[20804];
};

 
struct StatsBlobs
{
	CommonDataBlob commonData;
	MPDataBlob mpData;
	RankedLoadoutsBlob rankedLoadoutsData;
	PrivateLoadoutsBlob privateLoadoutsData;
	CPDataBlob cpData;
	NonGameDataBlob nonGameData;
};

 
struct PlayerStatsObfuscation
{
	bool isObfuscated;
	unsigned int randomInt;
};

 
struct StatsBlobFlags
{
	bool isDirty;
	bool isFirstUpload;
	bool isStatsReset;
	bool isPrestigeReset;
	bool isFailedUpload;
	bool isDoubleXPUsed;
	PlayerStatsObfuscation obfuscation;
};

 
enum ResetStatsReason
{
	RESET_STATS_NO_RESET = 0x0,
	RESET_STATS_REASON_NOT_FOUND = 0x1,
	RESET_STATS_REASON_PIPELINE = 0x2,
	RESET_STATS_REASON_GUEST = 0x3,
	RESET_STATS_REASON_CORRUPT = 0x4,
	RESET_STATS_REASON_BAD_FILE_VERSION = 0x5,
	RESET_STATS_REASON_BAD_PLAYERDATA_VERSION = 0x6,
	RESET_STATS_REASON_ONLINE_STATS_RESET = 0x7,
};

struct PlayerStats
{
	lol ___u0;
	unsigned __int8 validatedStatsData;
	StatsBlobFlags blobFlags[6];
	ResetStatsReason resetStatus;
	bool statsFetched;
	bool onlineStatWritePending;
	bool allowSavingProgress;
};

struct msg_t
{
	int overflowed;
	int readOnly;
	unsigned __int8* data;
	unsigned __int8* splitData;
	int maxsize;
	int cursize;
	int splitSize;
	int readcount;
	int bit;
	int lastEntityRef;
	int targetLocalNetID;
	unsigned int compressionFlags;
};


struct XUID {
	unsigned __int64 m_id;
	void Deserialize(const msg_t* msg);
	void Serialize(const msg_t* msg);
	XUID* FromHexString(const char* xuidString);
	XUID* FromMsg(const msg_t* msg);
	XUID* FromString(const char* xuidString);
	XUID* FromUInt64(unsigned __int64 id);
	XUID* FromUniversalId(unsigned __int64 id);

	unsigned __int64 GetUniversalId();

	bool IsNull();
	bool IsValid();

	XUID* NullXUID();
	XUID* RandomXUID();  

	const char* ToDevString();
	const char* ToHexString();
	const char* ToString();

	bool operator !=(const XUID* xuid);
	XUID* operator =(const XUID* xuid);
	bool operator ==(const XUID* xuid);

};

inline void LiveStorage_StatsInit(int controller, bool clear, bool freshStart, StatsSource statssrc)
{
	auto func = reinterpret_cast<void* (*)(int controller, bool clear, bool freshStart, StatsSource statssrc)>(s4::offsets::Resolve(s4::offsets::LiveStorage_StatsInit.rva));  
	func(controller, clear, freshStart, statssrc);
}

