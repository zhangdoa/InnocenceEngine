#pragma once
#include "../interface/ISystem.h"
#include "../component/AssetSystemSingletonComponent.h"

class TextureDataSystem : public ISystem
{
public:
	TextureDataSystem() {};
	~TextureDataSystem() {};

	void setup() override;
	void initialize() override;
	void update() override;
	void shutdown() override;

	const objectStatus& getStatus() const override;

private:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
};
