/*
 * Copyright 2011-2019 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx#license-bsd-2-clause
 */

#include "common.h"
#include "bgfx_utils.h"
#include "imgui/imgui.h"

#include <stdio.h>

namespace
{

struct PosColorVertex
{
	float m_x;
	float m_y;
	float m_z;
	uint32_t m_abgr;

	static void init()
	{
		ms_layout
			.begin()
			.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Color0,   4, bgfx::AttribType::Uint8, true)
			.end();
	};

	static bgfx::VertexLayout ms_layout;
};

bgfx::VertexLayout PosColorVertex::ms_layout;

constexpr uint16_t kNumTrisAcross = 15;
constexpr uint16_t kNumTrisDown = 30;
constexpr uint16_t kNumTriangles = kNumTrisAcross * kNumTrisDown;
constexpr uint16_t kNumVerts = 3 * kNumTriangles;

static PosColorVertex s_triVerts[kNumTrisDown][kNumTrisAcross * 3];
static uint16_t s_triIndices[kNumVerts];

class ExampleCubes : public entry::AppI
{
public:
	ExampleCubes(const char* _name, const char* _description, const char* _url)
		: entry::AppI(_name, _description, _url)
		, m_pt(0)
		//, m_singleThread(false)
		, m_makeRef(true)
	{
	}

	void init(int32_t _argc, const char* const* _argv, uint32_t _width, uint32_t _height) override
	{
		Args args(_argc, _argv);

		m_width  = _width;
		m_height = _height;
		m_debug  = BGFX_DEBUG_NONE;
		m_reset  = BGFX_RESET_VSYNC;

		bgfx::Init init;
		init.type     = args.m_type;
		init.vendorId = args.m_pciId;
		init.resolution.width  = m_width;
		init.resolution.height = m_height;
		init.resolution.reset  = m_reset;
		bgfx::init(init);

		// Enable debug text.
		bgfx::setDebug(m_debug);

		// Set view 0 clear state.
		bgfx::setViewClear(0
			, BGFX_CLEAR_COLOR|BGFX_CLEAR_DEPTH
			, 0x303030ff
			, 1.0f
			, 0
			);

		// Create vertex stream declaration.
		PosColorVertex::init();

		fillTriangles();
		// We fill the index buffer only once, here:
		for (uint16_t i=0; i<BX_COUNTOF(s_triIndices); ++i) {
			s_triIndices[i] = i;
		}

		// Create static vertex buffer.
		m_vbh = bgfx::createDynamicVertexBuffer(
			kNumVerts
			, PosColorVertex::ms_layout
			);

		// Create static index buffer for triangle list rendering.
		m_ibh = bgfx::createIndexBuffer(
			// Static data can be passed with bgfx::makeRef
			bgfx::makeRef(s_triIndices, sizeof(s_triIndices) )
			);

		// Create program from shaders.
		m_program = loadProgram("vs_cubes", "fs_cubes");

		m_timeOffset = bx::getHPCounter();

		imguiCreate();
	}

	virtual int shutdown() override
	{
		imguiDestroy();

		// Cleanup.
		bgfx::destroy(m_ibh);
		bgfx::destroy(m_vbh);
		bgfx::destroy(m_program);

		// Shutdown bgfx.
		bgfx::shutdown();

		return 0;
	}

	void slowSpin(int count) {
		// Waste some time.
		for (int n=0; n<count; ++n) {
			printf("%s", "");
		}
	}

	void fillTriangles() {
		static bool s_doShift = true;
		s_doShift = !s_doShift;

		const float width = 1.2f;
		const float height = 1.2f;

		float startX = -18.0f + (s_doShift ? 20.0f : 0.0f);
		float startY = -17.0f;
		float baseX = startX;
		float baseY = startY;
		for (int r=0; r<kNumTrisDown; ++r) {
			for (int c=0; c<kNumTrisAcross; ++c) {
				PosColorVertex &v0 = s_triVerts[r][3*c+0];
				PosColorVertex &v1 = s_triVerts[r][3*c+1];
				PosColorVertex &v2 = s_triVerts[r][3*c+2];

				v0.m_x = baseX + 0.0f;
				v0.m_y = baseY + 0.0f;
				v0.m_z = 0.0f;
				v0.m_abgr = 0xff00ffff;

				slowSpin(20 * r * r * r);

				v1.m_x = baseX + 1.0f;
				v1.m_y = baseY + 0.0f;
				v1.m_z = 0.0f;
				v1.m_abgr = 0xff00ffff;

				slowSpin(20 * r * r * r);

				v2.m_x = baseX + 0.0f;
				v2.m_y = baseY + 1.0f;
				v2.m_z = 0.0f;
				v2.m_abgr = 0xff00ffff;

				slowSpin(20 * r * r * r);

				baseX += width;
			}
			baseY += height;
			baseX = startX;
		}
	}

	bool update() override
	{
		if (!entry::processEvents(m_width, m_height, m_debug, m_reset, &m_mouseState) )
		{
			imguiBeginFrame(m_mouseState.m_mx
				,  m_mouseState.m_my
				, (m_mouseState.m_buttons[entry::MouseButton::Left  ] ? IMGUI_MBUT_LEFT   : 0)
				| (m_mouseState.m_buttons[entry::MouseButton::Right ] ? IMGUI_MBUT_RIGHT  : 0)
				| (m_mouseState.m_buttons[entry::MouseButton::Middle] ? IMGUI_MBUT_MIDDLE : 0)
				,  m_mouseState.m_mz
				, uint16_t(m_width)
				, uint16_t(m_height)
				);

			showExampleDialog(this);

			ImGui::SetNextWindowPos(
				  ImVec2(m_width - m_width / 5.0f - 10.0f, 10.0f)
				, ImGuiCond_FirstUseEver
				);
			ImGui::SetNextWindowSize(
				  ImVec2(m_width / 5.0f, m_height / 3.5f)
				, ImGuiCond_FirstUseEver
				);
			ImGui::Begin("Settings"
				, NULL
				, 0
				);

			//ImGui::Checkbox("Single-threaded\n(requires restart)", &m_singleThread);
			ImGui::Checkbox("Use makeRef\n(else: copy)", &m_makeRef);

			ImGui::End();

			imguiEndFrame();

			const bx::Vec3 at  = { 0.0f, 0.0f,   0.0f };
			const bx::Vec3 eye = { 0.0f, 0.0f, -35.0f };

			// Set view and projection matrix for view 0.
			{
				float view[16];
				bx::mtxLookAt(view, eye, at);

				float proj[16];
				bx::mtxProj(proj, 60.0f, float(m_width)/float(m_height), 0.1f, 100.0f, bgfx::getCaps()->homogeneousDepth);
				bgfx::setViewTransform(0, view, proj);

				// Set view 0 default viewport.
				bgfx::setViewRect(0, 0, 0, uint16_t(m_width), uint16_t(m_height) );
			}

			// This dummy draw call is here to make sure that view 0 is cleared
			// if no other draw calls are submitted to view 0.
			bgfx::touch(0);

			uint64_t state = 0
				| BGFX_STATE_WRITE_R
				| BGFX_STATE_WRITE_G
				| BGFX_STATE_WRITE_B
				| BGFX_STATE_WRITE_A
				| BGFX_STATE_WRITE_Z
				| BGFX_STATE_DEPTH_TEST_LESS
				| BGFX_STATE_CULL_CW
				| BGFX_STATE_MSAA
				// render as triangle list
				;

			// Update the triangles
			fillTriangles();
			bgfx::update(
				m_vbh
				,0
				,m_makeRef ?
					bgfx::makeRef(s_triVerts, sizeof(s_triVerts))
					: bgfx::copy(s_triVerts, sizeof(s_triVerts))
				);

			// Submit the triangles

			// Set vertex and index buffer.
			bgfx::setVertexBuffer(0, m_vbh);
			bgfx::setIndexBuffer(m_ibh);

			// Set render states.
			bgfx::setState(state);

			// Submit primitive for rendering to view 0.
			bgfx::submit(0, m_program);

			// Advance to next frame. Rendering thread will be kicked to
			// process submitted rendering primitives.
			bgfx::frame();

			return true;
		}

		return false;
	}

	entry::MouseState m_mouseState;

	uint32_t m_width;
	uint32_t m_height;
	uint32_t m_debug;
	uint32_t m_reset;
	bgfx::DynamicVertexBufferHandle m_vbh;
	bgfx::IndexBufferHandle m_ibh;
	bgfx::ProgramHandle m_program;
	int64_t m_timeOffset;
	int32_t m_pt;

	//bool m_singleThread; // TODO: implement this. For now, the makeRef flag is enough.
	bool m_makeRef;
};

} // namespace

ENTRY_IMPLEMENT_MAIN(
	  ExampleCubes
	, "x01-threads-vs-makeref"
	, "Demonstrate race condition with shared vertex buffer between main and rendering thread."
	, "http://zombo.com"
	);
