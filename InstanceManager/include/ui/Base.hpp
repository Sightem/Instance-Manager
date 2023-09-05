#pragma once
#include <GLFW/glfw3.h>

#include <cstdio>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <unordered_map>

#include "Roboto-Bold.embed"
#include "Roboto-Italic.embed"
#include "Roboto-Regular.embed"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

static std::unordered_map<std::string, ImFont*> s_Fonts;

static void ErrorCallback(int error, const char* description) {
	fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

template<typename Derived>
class AppBase {
public:
	AppBase() {
		glfwSetErrorCallback(ErrorCallback);

		if (!glfwInit())
			std::exit(1);

		const char* glsl_version = "#version 130";
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

		window = glfwCreateWindow(1280, 720, "Roblox UWP Instance Manager", nullptr, nullptr);
		if (window == NULL)
			std::exit(1);

		glfwSetWindowSize(window, 1920, 1080);
		glfwMakeContextCurrent(window);
		glfwSwapInterval(1);

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		(void) io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;// Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;    // Enable Docking
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;  // Enable Multi-Viewport / Platform Windows

		ImVec4* colors = ImGui::GetStyle().Colors;
		colors[ImGuiCol_Text] = ImGui::ColorConvertU32ToFloat4(IM_COL32(255, 255, 255, 255));
		colors[ImGuiCol_TextDisabled] = ImGui::ColorConvertU32ToFloat4(IM_COL32(128, 128, 128, 255));
		colors[ImGuiCol_WindowBg] = ImGui::ColorConvertU32ToFloat4(IM_COL32(26, 26, 26, 255));
		colors[ImGuiCol_ChildBg] = ImGui::ColorConvertU32ToFloat4(IM_COL32(0, 0, 0, 0));
		colors[ImGuiCol_PopupBg] = ImGui::ColorConvertU32ToFloat4(IM_COL32(48, 48, 48, 235));
		colors[ImGuiCol_Border] = ImGui::ColorConvertU32ToFloat4(IM_COL32(48, 48, 48, 74));
		colors[ImGuiCol_BorderShadow] = ImGui::ColorConvertU32ToFloat4(IM_COL32(0, 0, 0, 61));
		colors[ImGuiCol_FrameBg] = ImGui::ColorConvertU32ToFloat4(IM_COL32(13, 13, 13, 138));
		colors[ImGuiCol_FrameBgHovered] = ImGui::ColorConvertU32ToFloat4(IM_COL32(48, 48, 48, 138));
		colors[ImGuiCol_FrameBgActive] = ImGui::ColorConvertU32ToFloat4(IM_COL32(51, 56, 59, 255));
		colors[ImGuiCol_TitleBg] = ImGui::ColorConvertU32ToFloat4(IM_COL32(0, 0, 0, 255));
		colors[ImGuiCol_TitleBgActive] = ImGui::ColorConvertU32ToFloat4(IM_COL32(15, 15, 15, 255));
		colors[ImGuiCol_TitleBgCollapsed] = ImGui::ColorConvertU32ToFloat4(IM_COL32(0, 0, 0, 255));
		colors[ImGuiCol_MenuBarBg] = ImGui::ColorConvertU32ToFloat4(IM_COL32(36, 36, 36, 255));
		colors[ImGuiCol_ScrollbarBg] = ImGui::ColorConvertU32ToFloat4(IM_COL32(13, 13, 13, 138));
		colors[ImGuiCol_ScrollbarGrab] = ImGui::ColorConvertU32ToFloat4(IM_COL32(87, 87, 87, 138));
		colors[ImGuiCol_ScrollbarGrabHovered] = ImGui::ColorConvertU32ToFloat4(IM_COL32(102, 102, 102, 138));
		colors[ImGuiCol_ScrollbarGrabActive] = ImGui::ColorConvertU32ToFloat4(IM_COL32(143, 143, 143, 138));
		colors[ImGuiCol_CheckMark] = ImGui::ColorConvertU32ToFloat4(IM_COL32(84, 171, 219, 255));
		colors[ImGuiCol_SliderGrab] = ImGui::ColorConvertU32ToFloat4(IM_COL32(87, 87, 87, 138));
		colors[ImGuiCol_SliderGrabActive] = ImGui::ColorConvertU32ToFloat4(IM_COL32(143, 143, 143, 138));
		colors[ImGuiCol_Button] = ImGui::ColorConvertU32ToFloat4(IM_COL32(13, 13, 13, 138));
		colors[ImGuiCol_ButtonHovered] = ImGui::ColorConvertU32ToFloat4(IM_COL32(48, 48, 48, 138));
		colors[ImGuiCol_ButtonActive] = ImGui::ColorConvertU32ToFloat4(IM_COL32(51, 56, 59, 255));
		colors[ImGuiCol_Header] = ImGui::ColorConvertU32ToFloat4(IM_COL32(0, 0, 0, 133));
		colors[ImGuiCol_HeaderHovered] = ImGui::ColorConvertU32ToFloat4(IM_COL32(0, 0, 0, 92));
		colors[ImGuiCol_HeaderActive] = ImGui::ColorConvertU32ToFloat4(IM_COL32(51, 56, 59, 84));
		colors[ImGuiCol_Separator] = ImGui::ColorConvertU32ToFloat4(IM_COL32(71, 71, 71, 74));
		colors[ImGuiCol_SeparatorHovered] = ImGui::ColorConvertU32ToFloat4(IM_COL32(112, 112, 112, 74));
		colors[ImGuiCol_SeparatorActive] = ImGui::ColorConvertU32ToFloat4(IM_COL32(102, 112, 120, 255));
		colors[ImGuiCol_ResizeGrip] = ImGui::ColorConvertU32ToFloat4(IM_COL32(71, 71, 71, 74));
		colors[ImGuiCol_ResizeGripHovered] = ImGui::ColorConvertU32ToFloat4(IM_COL32(112, 112, 112, 74));
		colors[ImGuiCol_ResizeGripActive] = ImGui::ColorConvertU32ToFloat4(IM_COL32(102, 112, 120, 255));
		colors[ImGuiCol_Tab] = ImGui::ColorConvertU32ToFloat4(IM_COL32(0, 0, 0, 133));
		colors[ImGuiCol_TabHovered] = ImGui::ColorConvertU32ToFloat4(IM_COL32(36, 36, 36, 255));
		colors[ImGuiCol_TabActive] = ImGui::ColorConvertU32ToFloat4(IM_COL32(51, 51, 51, 92));
		colors[ImGuiCol_TabUnfocused] = ImGui::ColorConvertU32ToFloat4(IM_COL32(0, 0, 0, 133));
		colors[ImGuiCol_TabUnfocusedActive] = ImGui::ColorConvertU32ToFloat4(IM_COL32(36, 36, 36, 255));
		colors[ImGuiCol_DockingPreview] = ImGui::ColorConvertU32ToFloat4(IM_COL32(84, 171, 219, 255));
		colors[ImGuiCol_DockingEmptyBg] = ImGui::ColorConvertU32ToFloat4(IM_COL32(196, 196, 196, 255));
		colors[ImGuiCol_PlotLines] = ImGui::ColorConvertU32ToFloat4(IM_COL32(255, 0, 0, 255));
		colors[ImGuiCol_PlotLinesHovered] = ImGui::ColorConvertU32ToFloat4(IM_COL32(255, 0, 0, 255));
		colors[ImGuiCol_PlotHistogram] = ImGui::ColorConvertU32ToFloat4(IM_COL32(255, 0, 0, 255));
		colors[ImGuiCol_PlotHistogramHovered] = ImGui::ColorConvertU32ToFloat4(IM_COL32(255, 0, 0, 255));
		colors[ImGuiCol_TableHeaderBg] = ImGui::ColorConvertU32ToFloat4(IM_COL32(0, 0, 0, 133));
		colors[ImGuiCol_TableBorderStrong] = ImGui::ColorConvertU32ToFloat4(IM_COL32(0, 0, 0, 133));
		colors[ImGuiCol_TableBorderLight] = ImGui::ColorConvertU32ToFloat4(IM_COL32(71, 71, 71, 74));
		colors[ImGuiCol_TableRowBg] = ImGui::ColorConvertU32ToFloat4(IM_COL32(0, 0, 0, 0));
		colors[ImGuiCol_TableRowBgAlt] = ImGui::ColorConvertU32ToFloat4(IM_COL32(255, 255, 255, 15));
		colors[ImGuiCol_TextSelectedBg] = ImGui::ColorConvertU32ToFloat4(IM_COL32(51, 56, 59, 255));
		colors[ImGuiCol_DragDropTarget] = ImGui::ColorConvertU32ToFloat4(IM_COL32(84, 171, 219, 255));
		colors[ImGuiCol_NavHighlight] = ImGui::ColorConvertU32ToFloat4(IM_COL32(255, 0, 0, 255));
		colors[ImGuiCol_NavWindowingHighlight] = ImGui::ColorConvertU32ToFloat4(IM_COL32(255, 0, 0, 178));
		colors[ImGuiCol_NavWindowingDimBg] = ImGui::ColorConvertU32ToFloat4(IM_COL32(255, 0, 0, 51));
		colors[ImGuiCol_ModalWindowDimBg] = ImGui::ColorConvertU32ToFloat4(IM_COL32(255, 0, 0, 89));

		ImGuiStyle& style = ImGui::GetStyle();
		style.WindowPadding = ImVec2(10.0f, 10.0f);
		style.FramePadding = ImVec2(8.0f, 6.0f);
		style.CellPadding = ImVec2(6.00f, 6.00f);
		style.ItemSpacing = ImVec2(6.0f, 6.0f);
		style.ItemInnerSpacing = ImVec2(6.00f, 6.00f);
		style.TouchExtraPadding = ImVec2(0.00f, 0.00f);
		style.IndentSpacing = 25;
		style.ScrollbarSize = 15;
		style.GrabMinSize = 10;
		style.WindowBorderSize = 1;
		style.ChildBorderSize = 1;
		style.PopupBorderSize = 1;
		style.FrameBorderSize = 1;
		style.TabBorderSize = 1;
		style.WindowRounding = 7;
		style.ChildRounding = 6.0f;
		style.PopupRounding = 6.0f;
		style.FrameRounding = 6.0f;
		style.ScrollbarRounding = 9;
		style.GrabRounding = 3;
		style.LogSliderDeadzone = 4;
		style.TabRounding = 2;


		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		// Setup Platform/Renderer backends
		ImGui_ImplGlfw_InitForOpenGL(window, true);
		ImGui_ImplOpenGL3_Init(glsl_version);

		// Add custom fonts
		ImFontConfig fontConfig;
		fontConfig.FontDataOwnedByAtlas = false;
		ImFont* robotoFont = io.Fonts->AddFontFromMemoryTTF((void*) g_RobotoRegular, sizeof(g_RobotoRegular), 20.0f, &fontConfig);
		s_Fonts["Default"] = robotoFont;
		s_Fonts["Bold"] = io.Fonts->AddFontFromMemoryTTF((void*) g_RobotoBold, sizeof(g_RobotoBold), 20.0f, &fontConfig);
		s_Fonts["Italic"] = io.Fonts->AddFontFromMemoryTTF((void*) g_RobotoItalic, sizeof(g_RobotoItalic), 20.0f, &fontConfig);
		io.FontDefault = robotoFont;

		unsigned char* pixels;
		int width, height;
		io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

		GLuint font_texture;
		glGenTextures(1, &font_texture);
		glBindTexture(GL_TEXTURE_2D, font_texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

		io.Fonts->TexID = (void*) (intptr_t) font_texture;

		io.Fonts->ClearTexData();
	}

	virtual ~AppBase() {
		// Cleanup
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		//ImPlot::DestroyContext();
		ImGui::DestroyContext();

		glfwDestroyWindow(window);
		glfwTerminate();
	}

	void Run() {
		// Initialize the underlying app
		StartUp();
		ImGuiIO& io = ImGui::GetIO();

		while (!glfwWindowShouldClose(window)) {

			// Poll events like key presses, mouse movements etc.
			glfwPollEvents();

			// Start the Dear ImGui frame
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			{
				// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
				// because it would be confusing to have two docking targets within each others.
				ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;

				ImGuiViewport* viewport = ImGui::GetMainViewport();
				ImGui::SetNextWindowPos(viewport->Pos);
				ImGui::SetNextWindowSize(viewport->Size);
				ImGui::SetNextWindowViewport(viewport->ID);
				ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
				ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
				window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
				window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

				ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.0f, 6.0f));
				ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 3.0f);

				ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4{0.0f, 0.0f, 0.0f, 0.0f});
				ImGui::Begin("DockSpaceWindow", nullptr, window_flags);
				ImGui::PopStyleColor();// MenuBarBg
				ImGui::PopStyleVar(2);

				ImGui::PopStyleVar(2);


				// Dockspace
				ImGuiIO& io = ImGui::GetIO();
				ImGuiStyle& style = ImGui::GetStyle();
				float minWinSizeX = style.WindowMinSize.x;
				style.WindowMinSize.x = 370.0f;
				ImGui::DockSpace(ImGui::GetID("MyDockspace"));
				style.WindowMinSize.x = minWinSizeX;

				ImGui::End();
			}

			// Main loop of the underlying app
			Update();

			// Rendering
			ImGui::Render();
			int display_w, display_h;
			glfwGetFramebufferSize(window, &display_w, &display_h);
			glViewport(0, 0, display_w, display_h);
			glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
			glClear(GL_COLOR_BUFFER_BIT);
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

			// Update and Render additional Platform Windows
			// (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
			//  For this specific demo app we could also call glfwMakeContextCurrent(window) directly)
			if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
				GLFWwindow* backup_current_context = glfwGetCurrentContext();
				ImGui::UpdatePlatformWindows();
				ImGui::RenderPlatformWindowsDefault();
				glfwMakeContextCurrent(backup_current_context);
			}

			glfwSwapBuffers(window);
		}
	}

	virtual void Update() {
		static_cast<Derived*>(this)->Update();
	}

	void StartUp() {
		static_cast<Derived*>(this)->StartUp();
	}

private:
	GLFWwindow* window = nullptr;
	ImVec4 clear_color = ImVec4(0.1058f, 0.1137f, 0.1255f, 1.00f);
};
