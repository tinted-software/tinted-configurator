#include <cstddef>
#include <string>
#include <vector>
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include "imgui_node_editor.h"
#include <SDL3/SDL.h>
#include <stdio.h>

struct LinkInfo {
  ax::NodeEditor::PinId input_id;
  ax::NodeEditor::PinId output_id;
};

int main(int, char **) {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMEPAD) != 0) {
    printf("Error: SDL_Init(): %s\n", SDL_GetError());
    return -1;
  }

  SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");

  Uint32 window_flags =
      SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN;
  SDL_Window *window =
      SDL_CreateWindow("Tinted Configurator", 1280, 720, window_flags);
  if (window == nullptr) {
    printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
    return -1;
  }
  SDL_Renderer *renderer = SDL_CreateRenderer(window, nullptr);
  SDL_SetRenderVSync(renderer, 1);
  if (renderer == nullptr) {
    SDL_Log("Error: SDL_CreateRenderer(): %s\n", SDL_GetError());
    return -1;
  }
  SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
  SDL_ShowWindow(window);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

  ImGui::StyleColorsDark();

  ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
  ImGui_ImplSDLRenderer3_Init(renderer);

  ax::NodeEditor::Config config;
  config.SettingsFile = "node_editor.json";
  ax::NodeEditor::EditorContext *node_editor =
      ax::NodeEditor::CreateEditor(&config);

  ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

  bool done = false;

  std::vector<std::string> available_components = {
      "Chromium", "Linux Base System", "Systemd", "Wayland Window System",
      "Genesis Desktop"};

  std::vector<LinkInfo> m_links;
  size_t m_next_link_id = 100;

  while (!done) {

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      ImGui_ImplSDL3_ProcessEvent(&event);
      if (event.type == SDL_EVENT_QUIT)
        done = true;
      if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
          event.window.windowID == SDL_GetWindowID(window))
        done = true;
    }

    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Tinted Configurator");

    if (ImGui::Button("Add a component")) {
      ImGui::OpenPopup("Add A New Component");
    }

    if (ImGui::BeginPopupModal("Add A New Component", nullptr)) {
      ImGuiTextFilter filter;
      ImGui::Text("Search:");
      filter.Draw("##searchbar");

      ImGui::BeginChild("listbox child");

      for (size_t i = 0; i < available_components.size(); i++) {
        auto available_component = available_components.at(i);

        if (filter.PassFilter(available_component.c_str())) {
          if (ImGui::Selectable(available_component.c_str())) {
            ImGui::CloseCurrentPopup();
          }
        }
      }

      ImGui::EndChild();

      if (ImGui::Button("Close"))
        ImGui::CloseCurrentPopup();

      ImGui::EndPopup();
    }

    ax::NodeEditor::SetCurrentEditor(node_editor);
    ax::NodeEditor::Begin("Node Graph", ImVec2(0.0, 0.0f));
    int unique_identifier = 1;

    ax::NodeEditor::BeginNode(unique_identifier++);
    ax::NodeEditor::BeginPin(unique_identifier++,
                             ax::NodeEditor::PinKind::Input);
    ImGui::Text("Physical Disk 1");
    ax::NodeEditor::EndPin();
    ax::NodeEditor::EndNode();

    ax::NodeEditor::BeginNode(unique_identifier++);
    ImGui::Text("Linux Base System");
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::BeginItemTooltip()) {
      ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
      ImGui::TextWrapped(
          "Includes the core functionality to boot a minimal Linux system.");
      ImGui::PopTextWrapPos();
      ImGui::EndTooltip();
    }
    ax::NodeEditor::BeginPin(unique_identifier++,
                             ax::NodeEditor::PinKind::Output);
    ImGui::Text("Output ->");
    ax::NodeEditor::EndPin();
    ax::NodeEditor::EndNode();

    {
      std::vector<LinkInfo>::iterator link;

      for (link = m_links.begin(); link < m_links.end(); link++) {
        std::size_t index = std::distance(std::begin(m_links), link);

        ax::NodeEditor::Link(index, link->input_id, link->output_id);
      }
    }

    if (ax::NodeEditor::BeginCreate()) {
      ax::NodeEditor::PinId inputPinId, outputPinId;
      if (ax::NodeEditor::QueryNewLink(&inputPinId, &outputPinId)) {

        if (inputPinId && outputPinId) {

          if (ax::NodeEditor::AcceptNewItem()) {

            m_links.push_back({inputPinId, outputPinId});

            ax::NodeEditor::Link(m_links.size() - 1, m_links.back().input_id,
                                 m_links.back().output_id);
          }
        }
      }
    }
    ax::NodeEditor::EndCreate();

    if (ax::NodeEditor::BeginDelete()) {

      ax::NodeEditor::LinkId deleted_link_id;
      while (ax::NodeEditor::QueryDeletedLink(&deleted_link_id)) {

        if (ax::NodeEditor::AcceptDeletedItem()) {
          std::vector<LinkInfo>::iterator link;

          for (link = m_links.begin(); link < m_links.end(); link++) {
            std::size_t index = std::distance(std::begin(m_links), link);

            if (index == deleted_link_id.Get()) {
              m_links.erase(link);
              break;
            }
          }
        }
      }
    }
    ax::NodeEditor::EndDelete();

    ax::NodeEditor::End();
    ax::NodeEditor::SetCurrentEditor(nullptr);
    ImGui::End();

    ImGui::Render();

    SDL_SetRenderDrawColor(
        renderer, (Uint8)(clear_color.x * 255), (Uint8)(clear_color.y * 255),
        (Uint8)(clear_color.z * 255), (Uint8)(clear_color.w * 255));
    SDL_RenderClear(renderer);
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
    SDL_RenderPresent(renderer);
  }

  ax::NodeEditor::DestroyEditor(node_editor);

  ImGui_ImplSDLRenderer3_Shutdown();
  ImGui_ImplSDL3_Shutdown();
  ImGui::DestroyContext();

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
