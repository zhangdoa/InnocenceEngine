#pragma once

#ifndef _I_BASE_OBJECT_H_
#define _I_BASE_OBJECT_H_

enum class objectStatus
{
	STANDBY,
	ALIVE,
	SHUTDOWN,
};

typedef unsigned long int GameObjectID;

class IBaseObject
{
public:
	IBaseObject();
	virtual ~IBaseObject();

	const objectStatus& getStatus() const;
	const GameObjectID& getGameObjectID() const;
	const std::string& getClassName() const;

	virtual void initialize() = 0;
	virtual void update() = 0;
	virtual void shutdown() = 0;

protected:
	void setStatus(objectStatus objectStatus);

private:
	objectStatus m_ObjectStatus = objectStatus::SHUTDOWN;
	GameObjectID m_gameObjectID = 0;
	std::string m_className = {};
};

#endif // !_I_BASE_OBJECT_H_