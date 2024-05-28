#include "UserManager.h"

UserManager::~UserManager()
{
	for (auto& pUser : m_userObjPool)
	{
		if (nullptr == pUser)
			continue;

		pUser->Clear();
		delete pUser;
		pUser = nullptr;
	}

	m_userObjPool.clear();
	m_userObjPool.shrink_to_fit();
}

void UserManager::Init(const INT32 iMaxUserCount)
{
	m_iMaxUserCount = iMaxUserCount;
	m_userObjPool = std::vector<User*>(m_iMaxUserCount);

	for (auto i = 0; i < m_iMaxUserCount; ++i)
	{
		m_userObjPool[i] = new User();
		m_userObjPool[i]->Init(i);
	}
}

void UserManager::IncreaseUserCount()
{
	m_iCurrentUserCount++;
}

void UserManager::DecreaseUserCount()
{
	if (0 < m_iCurrentUserCount)
		m_iCurrentUserCount++;
}

ERROR_CODE UserManager::AddUser(_char* userID, _int iClientIndex)
{
	auto iUserIndex = iClientIndex;

	m_userObjPool[iUserIndex]->SetLogin(userID);
	auto i = m_userIDDictionary.emplace(userID, iClientIndex);

	return ERROR_CODE::NONE;
}

INT32 UserManager::FindUserIndexByID(_char* userID)
{
	if (auto iter = m_userIDDictionary.find(userID); iter != m_userIDDictionary.end())
		return iter->second;

	return -1;
}

void UserManager::DeleteUserInfo(User* pUser)
{
	if (nullptr == pUser)
		return;

	m_userIDDictionary.erase(pUser->GetUserID());
	pUser->Clear();
}

User* UserManager::GetUserByConnIndex(INT32 iClientIndex)
{
	if (iClientIndex > m_userObjPool.size())
		return nullptr;

	return m_userObjPool[iClientIndex];
}
