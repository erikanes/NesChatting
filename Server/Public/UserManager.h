#pragma once

#include <unordered_map>
#include <string>
#include "Common_Define.h"
#include "ErrorCode.h"
#include "User.h"

class UserManager
{
public:
	UserManager() = default;
	~UserManager();

	void Init(const INT32 iMaxUserCount);

	void IncreaseUserCount();
	void DecreaseUserCount();

	ERROR_CODE AddUser(_char* userID, _int iClientIndex);
	INT32 FindUserIndexByID(_char* userID);

	void DeleteUserInfo(User* pUser);
	User* GetUserByConnIndex(INT32 iClientIndex);

#pragma region Getter
	INT32 GetCurrentUserCount() const { return m_iCurrentUserCount; }
	INT32 GetMaxUserCount() const { return m_iMaxUserCount; }
#pragma endregion

#pragma region Setter
#pragma endregion

private:
	INT32 m_iMaxUserCount = 0;
	INT32 m_iCurrentUserCount = 0;

	std::vector<User*> m_userObjPool;
	std::unordered_map<std::string, _int> m_userIDDictionary;
};

