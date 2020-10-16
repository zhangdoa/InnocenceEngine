#include "../../Engine/Common/InnoType.h"
#include "../../Engine/Interface/IRenderingClient.h"

namespace InnoBaker
{
	void Setup();
	void BakeProbeCache(const char* sceneName);
	void BakeBrickCache(const char* surfelCacheFileName);
	void BakeBrick(const char* brickCacheFileName);
	void BakeBrickFactor(const char* brickFileName);
}

class InnoBakerRenderingClient : public IRenderingClient
{
	// Inherited via IRenderingClient
	bool Setup(ISystemConfig* systemConfig) override;
	bool Initialize() override;
	bool Render() override;
	bool Terminate() override;

	ObjectStatus GetStatus() override;
};