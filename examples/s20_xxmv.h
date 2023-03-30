#pragma once
#include "main.h"
#include "media/MediaEngine.h"

static auto mefactory = ax::CreatePlatformMediaEngineFactory();

namespace XxmvTest {

	struct Scene : SceneBase {
		Scene() {
			_me = mefactory->CreateMediaEngine();
		}
		~Scene()
		{
			mefactory->DestroyMediaEngine(_me);
		}
		void Init(GameLooper* looper) override;
		int Update() override;

		xx::Mv mv;
		xx::Quad spr;
		std::vector<xx::Shared<xx::GLTexture>> texs;
		int cursor{};
		float timePool{};

		ax::MediaEngine* _me;
	};
}
