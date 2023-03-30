#include "main.h"
#include "s20_xxmv.h"

namespace XxmvTest {

	void Scene::Init(GameLooper* looper) {
		this->looper = looper;
		std::cout << "XxmvTest::Scene::Init" << std::endl;

		xx::Shader_Yuva2Rgba::Init();

		auto [d, f] = xx::engine.LoadFileData("res/st_k100.xxmv");

		int r = mv.Load(d);
		assert(!r);

		auto&& shader = xx::engine.sm.GetShader<xx::Shader_Yuva2Rgba>();


		auto fullPath = xx::engine.GetFullPath(R"(E:\dev\xx2d\_bak\res\st_k200.webm)");
		_me->SetAutoPlay(true);
		_me->Open(fullPath/*R"(D:\dev\axmol\tests\cpp-tests\Content\SampleVideo5.mp4)"*/);

		//ax::MEVideoTextueSample sample;
		//while (!_me->GetLastVideoSample(sample)) {
		//	std::this_thread::sleep_for(std::chrono::milliseconds(50));
		//}

		ax::MEVideoTextueSample sample;
		while (!_me->GetLastVideoSample(sample)) std::this_thread::sleep_for(std::chrono::milliseconds(50));

		{
			printf("got video frame");

			auto c = GL_RGBA;
			auto t = xx::LoadGLTexture_core();
			glTexImage2D(GL_TEXTURE_2D, 0, c, sample._bufferDim.x, sample._bufferDim.y, 0, c, GL_UNSIGNED_BYTE, sample._buffer.data());
			glBindTexture(GL_TEXTURE_2D, 0);
			auto tex = xx::Make<xx::GLTexture>(t, sample._bufferDim.x, sample._bufferDim.y, "");

			spr.SetTexture(tex);
		}

		// wait on sample ready
#if 0
		auto secs = xx::NowEpochSeconds();
		mv.ForeachFrame([&](int const& frameIndex, uint32_t const& w, uint32_t const& h
		, uint8_t const* const& yData, uint8_t const* const& uData, uint8_t const* const& vData, uint8_t const* const& aData, uint32_t const& yaStride, uint32_t const& uvStride)->int {

			auto tex = xx::FrameBuffer().Init().Draw({ w, h }, true, xx::RGBA8{}, [&]() {
				shader.Draw(yData, uData, vData, aData, yaStride, uvStride, w, h, {});
			});

			texs.push_back(tex);

			return 0;
		});
#endif

		// xx::CoutN("convert res/st_k100.xxmv all frames to texs. elapsed secs = ", xx::NowEpochSeconds(secs));

		//spr.SetTexture(texs[cursor]);
	}

	int Scene::Update() {
#if 0
		timePool += xx::engine.delta;
		while (timePool >= 1.f / 60) {
			timePool -= 1.f / 60;

			if (++cursor == texs.size()) {
				cursor = 0;
			}
			//spr.SetTexture(texs[cursor]);
		}
		spr.Draw();
#endif
		/*ax::MEVideoTextueSample sample;
		if (_me->GetLastVideoSample(sample)) {
			printf("got video frame");

			auto c = GL_BGRA;
			auto t = xx::LoadGLTexture_core();
			glTexImage2D(GL_TEXTURE_2D, 0, c, sample._bufferDim.x, sample._bufferDim.y, 0, c, GL_UNSIGNED_BYTE, sample._buffer.data());
			glBindTexture(GL_TEXTURE_2D, 0);
			auto tex = xx::Make<xx::GLTexture>(t, sample._bufferDim.x, sample._bufferDim.y, "");

			spr.SetTexture(tex);
		}*/
		spr.Draw();
		return 0;
	}
}
