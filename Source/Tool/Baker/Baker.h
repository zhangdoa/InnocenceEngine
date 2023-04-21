#include "../../Engine/Common/Type.h"
#include "../../Engine/Interface/IRenderingClient.h"

using namespace Inno;
namespace Baker
{
	void Setup();
	void BakeProbeCache(const char* sceneName);
	void BakeBrickCache(const char* surfelCacheFileName);
	void BakeBrick(const char* brickCacheFileName);
	void BakeBrickFactor(const char* brickFileName);
}

class BakerRenderingClient : public IRenderingClient
{
	// Inherited via IRenderingClient
	bool Setup(ISystemConfig* systemConfig) override;
	bool Initialize() override;
	bool Render(IRenderingConfig* renderingConfig = nullptr) override;
	bool Terminate() override;

	ObjectStatus GetStatus() override;
};