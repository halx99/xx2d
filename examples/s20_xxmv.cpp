#include "main.h"
#include "s20_xxmv.h"

namespace xx {
	struct Shader_NV12toRgba : Shader {

		static const size_t index = 6;	// index at sm->shaders

		GLint uCxy = -1, uStrideHeight = -1, uTexY = -1, uTexUV = -1, aPos = -1, aTexCoord = -1;
		GLVertexArrays va;
		GLBuffer vb, ib;

		static void Init()
		{
			if (!engine.sm.shaders[index]) {
				auto s = Make<Shader_NV12toRgba>();
				engine.sm.shaders[index] = s;
				s->Init(&engine.sm);
			}
		}
		void Init(ShaderManager* sm_) override
		{
			sm = sm_;

			v = LoadGLVertexShader({ R"(#version 300 es
precision highp float;
uniform vec2 uCxy;	// screen center coordinate
uniform vec2 uStrideHeight;

in vec2 aPos;
in vec2 aTexCoord;	// pixel pos

out vec2 vTexCoord;	// 0 ~ 1

void main() {
	gl_Position = vec4(aPos * uCxy, 0, 1);
	vTexCoord = aTexCoord / uStrideHeight;
})"sv });

			f = LoadGLFragmentShader({ R"(#version 300 es
precision highp float;

uniform sampler2D uTexY;
uniform sampler2D uTexUV;

in vec2 vTexCoord;

out vec4 oColor;

const mat3 YUVtoRGBCoeff = mat3(
    1.16438356, 1.16438356,  1.16438356,
    0.00000000, -0.213237017, 2.11241937,
    1.79265225, -0.533004045, 0.00000000
);

const vec3 YUVOffset8bits = vec3(0.0627451017, 0.501960814, 0.501960814);

vec3 trasnformYUV(vec3 YUV)
{
    YUV -= YUVOffset8bits;
    return YUVtoRGBCoeff * YUV;
}

void main() {
	vec3 YUV;
    
    YUV.x = texture2D(uTexY, vTexCoord).x; // Y
    YUV.yz = texture2D(uTexUV, vTexCoord).xy; // CbCr
	
    /* Convert YUV to RGB */
    vec4 OutColor;
    OutColor.xyz = trasnformYUV(YUV);
    OutColor.w = 1.0;

    oColor = OutColor;
})"sv });

			p = LinkGLProgram(v, f);

			uCxy = glGetUniformLocation(p, "uCxy");
			uStrideHeight = glGetUniformLocation(p, "uStrideHeight");
			uTexY = glGetUniformLocation(p, "uTexY");
			uTexUV = glGetUniformLocation(p, "uTexUV");

			aPos = glGetAttribLocation(p, "aPos");
			aTexCoord = glGetAttribLocation(p, "aTexCoord");
			CheckGLError();

			glGenVertexArrays(1, &va.Ref());
			glBindVertexArray(va);
			glGenBuffers(1, (GLuint*)&vb);
			glGenBuffers(1, (GLuint*)&ib);

			glBindBuffer(GL_ARRAY_BUFFER, vb);
			glVertexAttribPointer(aPos, 2, GL_FLOAT, GL_FALSE, sizeof(XYUV), 0);
			glEnableVertexAttribArray(aPos);
			glVertexAttribPointer(aTexCoord, 2, GL_UNSIGNED_SHORT, GL_FALSE, sizeof(XYUV), (GLvoid*)offsetof(XYUV, u));
			glEnableVertexAttribArray(aTexCoord);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib);
			GLushort idxs[6] = { 0, 1, 2, 0, 2, 3 };
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idxs), idxs, GL_STATIC_DRAW);

			glBindVertexArray(0);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			glBindBuffer(GL_ARRAY_BUFFER, 0);

			CheckGLError();
		}
		void Begin() override
		{
			if (sm->cursor != index) {
				// here can check shader type for combine batch
				sm->shaders[sm->cursor]->End();
				sm->cursor = index;
			}
		}
		void End() override
		{

		}

		void Draw(uint8_t const* const& yData, uint8_t const* const& uvData, uint32_t const& yaStride, uint32_t const& uvStride, uint32_t const& w, uint32_t const& h, XY const& pos)
		{
			glUseProgram(p);
			glUniform2f(uCxy, 2 / engine.w, 2 / engine.h);
			glUniform2f(uStrideHeight, yaStride, h);
			glUniform1i(uTexY, 0);
			glUniform1i(uTexUV, 1);

			glBindVertexArray(va);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib);

			XYUV xyuv[4];

			auto w2 = w / 2;
			auto h2 = h / 2;

			xyuv[0].x = pos.x - w2;
			xyuv[0].y = pos.y - h2;
			xyuv[0].u = 0;
			xyuv[0].v = 0;

			xyuv[1].x = pos.x - w2;
			xyuv[1].y = pos.y + h2;
			xyuv[1].u = 0;
			xyuv[1].v = h;

			xyuv[2].x = pos.x + w2;
			xyuv[2].y = pos.y + h2;
			xyuv[2].u = w;
			xyuv[2].v = h;

			xyuv[3].x = pos.x + w2;
			xyuv[3].y = pos.y - h2;
			xyuv[3].u = w;
			xyuv[3].v = 0;

			auto texY = LoadGLTexture_core(0);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, yaStride, h, 0, GL_RED, GL_UNSIGNED_BYTE, yData);
			CheckGLError();

			auto texUV = LoadGLTexture_core(1);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RG, uvStride, h / 2, 0, GL_RG, GL_UNSIGNED_BYTE, uvData);
			CheckGLError();

			glBindBuffer(GL_ARRAY_BUFFER, vb);
			glBufferData(GL_ARRAY_BUFFER, sizeof(xyuv), xyuv, GL_STREAM_DRAW);

			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, {});
			CheckGLError();

			sm->drawVerts += 6;
			sm->drawCall += 1;
		}
	};
}

namespace XxmvTest {
	void Scene::Init(GameLooper* looper) {
		this->looper = looper;
		std::cout << "XxmvTest::Scene::Init" << std::endl;

		xx::Shader_NV12toRgba::Init();

		auto [d, f] = xx::engine.LoadFileData("res/st_k100.xxmv");

		int r = mv.Load(d);
		assert(!r);

		auto&& shader = xx::engine.sm.GetShader<xx::Shader_NV12toRgba>();


		//auto fullPath = xx::engine.GetFullPath(R"(E:\dev\xx2d\_bak\res\st_k200.webm)");
		_me->SetAutoPlay(true);
		_me->Open(R"(D:\dev\axmol\tests\cpp-tests\Content\hvc1_1920x1080.mp4)");

		//ax::MEVideoTextueSample sample;
		//while (!_me->GetLastVideoSample(sample)) {
		//	std::this_thread::sleep_for(std::chrono::milliseconds(50));
		//}

		unsigned frameCount = 0;

		ax::MEVideoTextueSample sample;
		//while (!_me->GetLastVideoSample(sample))
		while (frameCount < 120)
		{
			if (_me->GetLastVideoSample(sample)) {
				printf("got video frame\n");

				//auto c = GL_RGBA;
				//auto t = xx::LoadGLTexture_core();
				//glTexImage2D(GL_TEXTURE_2D, 0, c, sample._bufferDim.x, sample._bufferDim.y, 0, GL_BGRA, GL_UNSIGNED_BYTE, sample._buffer.data());
				//glBindTexture(GL_TEXTURE_2D, 0);
				//auto tex = xx::Make<xx::GLTexture>(t, sample._bufferDim.x, sample._bufferDim.y, "");

				/*spr.SetTexture(tex);*/
				auto vidoeDim = sample._videoDim;
				auto tex = xx::FrameBuffer().Init().Draw({ (uint32_t)sample._videoDim.x, (uint32_t)sample._videoDim.y }, true, xx::RGBA8{}, [&]() {
					shader.Draw(sample._buffer.data(), sample._buffer.data() + sample._yuvDesc.YDataLen, sample._yuvDesc.YPitch, sample._yuvDesc.UVPitch >> 1, vidoeDim.x, vidoeDim.y, {});
					});

				texs.push_back(tex);

				++frameCount;
			}
			else
				std::this_thread::sleep_for(std::chrono::milliseconds(17));
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
#if 1
		timePool += xx::engine.delta;
		while (timePool >= 1.f / 60) {
			timePool -= 1.f / 60;

			if (++cursor == texs.size()) {
				cursor = 0;
			}
			spr.SetTexture(texs[cursor]);
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
