#include "../../Engine/Common/InnoType.h"
#include "../../Engine/Core/IRenderingClient.h"

namespace InnoBaker
{
	void Setup();
	void BakeProbeCache(const std::string & sceneName);
	void BakeBrickCache(const std::string& surfelCacheFileName);
	void BakeBrick(const std::string& brickCacheFileName);
	void BakeBrickFactor(const std::string& brickFileName);
}

class InnoBakerRenderingClient : public IRenderingClient
{
	// Inherited via IRenderingClient
	virtual bool Setup() override;
	virtual bool Initialize() override;
	virtual bool Render() override;
	virtual bool Terminate() override;
};