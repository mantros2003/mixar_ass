#define IMGUI_DEFINE_MATH_OPERATORS

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imnodes.h"
#include "imgui_internal.h"
#include <GLFW/glfw3.h>
#include <vector>
#include <string>
#include <optional>
#include <map>
#include <iostream>

enum class OperationType {
    Blur,
    Brightness,
    LoadImage
};

struct Node {
    int id;
    OperationType type;
    std::string name;
    ImVec2 position;
    int inputSlotId;
    int outputSlotId;
    float width = 150.0f; // Default resizable width

    std::optional<float> value; // For sliders/input fields like brightness
    std::optional<std::string> imagePath; // For LoadImage
};

struct Link {
    int id;
    int fromSlot;
    int toSlot;
};

std::vector<Node> nodes;
std::vector<Link> links;
int nodeCounter = 0;
int slotCounter = 1000;
int linkCounter = 0;

void AddNode(OperationType type, const std::string& name, ImVec2 pos) {
    Node node;
    node.id = nodeCounter++;
    node.type = type;
    node.name = name;
    node.position = pos;
    node.inputSlotId = slotCounter++;
    node.outputSlotId = slotCounter++;
    if (type == OperationType::Brightness || type == OperationType::Blur) {
        node.value = 0.0f;
    } else if (type == OperationType::LoadImage) {
        node.imagePath = "";
    }    
    nodes.push_back(node);
}

void RenderNodes() {
    ImGui::SetNextWindowPos(ImVec2(200, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize - ImVec2(200, 0), ImGuiCond_Always);

    ImGui::Begin("Node Editor Area", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
    ImNodes::BeginNodeEditor();

    for (Node& node : nodes) {
        ImNodes::BeginNode(node.id);

        ImNodes::BeginNodeTitleBar();
        ImGui::TextUnformatted(node.name.c_str());
        ImNodes::EndNodeTitleBar();

        ImGui::PushItemWidth(node.width);

        if (node.type != OperationType::LoadImage) {
            ImNodes::BeginInputAttribute(node.inputSlotId);
            ImGui::Text("Input");
            ImNodes::EndInputAttribute();
        }

        if (node.value.has_value()) {
            static std::map<int, std::string> inputBuffers;
            char buf[32];
            snprintf(buf, sizeof(buf), "##val%d", node.id);

            if (inputBuffers.find(node.id) == inputBuffers.end())
                inputBuffers[node.id] = std::to_string(*node.value);

            char input[32];
            strncpy(input, inputBuffers[node.id].c_str(), sizeof(input));
            input[sizeof(input) - 1] = '\0';

            if (ImGui::InputText(buf, input, IM_ARRAYSIZE(input), ImGuiInputTextFlags_EnterReturnsTrue)) {
                try {
                    *node.value = std::stof(input);
                    inputBuffers[node.id] = input;
                } catch (...) {
                    // handle parse error (optional)
                }
            }
        }

        if (node.imagePath.has_value()) {
            static std::map<int, std::string> pathBuffers;
            char buf[64];
            snprintf(buf, sizeof(buf), "##path%d", node.id);
        
            if (pathBuffers.find(node.id) == pathBuffers.end())
                pathBuffers[node.id] = *node.imagePath;
        
            char input[256];
            strncpy(input, pathBuffers[node.id].c_str(), sizeof(input));
            input[sizeof(input) - 1] = '\0';
        
            if (ImGui::InputText(buf, input, IM_ARRAYSIZE(input), ImGuiInputTextFlags_EnterReturnsTrue)) {
                *node.imagePath = input;
                pathBuffers[node.id] = input;
            }
        }

        ImNodes::BeginOutputAttribute(node.outputSlotId);
        ImGui::Text("Output");
        ImNodes::EndOutputAttribute();

        // Resizer grip
        ImVec2 grip_pos = ImGui::GetCursorScreenPos();
        ImGui::SetCursorScreenPos(ImVec2(grip_pos.x + node.width, grip_pos.y));
        ImGui::PushID(node.id); // Unique ID for each grip
        ImGui::Button(">");     // Visual resizer
        if (ImGui::IsItemActive()) {
            node.width += ImGui::GetIO().MouseDelta.x;
            node.width = ImClamp(node.width, 100.0f, 300.0f); // Clamp to a reasonable range
        }
        ImGui::PopID();

        ImNodes::EndNode();
    }

    for (const auto& link : links) {
        ImNodes::Link(link.id, link.fromSlot, link.toSlot);
    }

    ImNodes::EndNodeEditor();

    int startAttr, endAttr;
    if (ImNodes::IsLinkCreated(&startAttr, &endAttr)) {
        links.push_back({linkCounter++, startAttr, endAttr});
    }

    ImGui::End();
}

void ShowSidePanel() {
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(200, ImGui::GetIO().DisplaySize.y), ImGuiCond_Always);
    ImGui::Begin("Add Nodes", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    if (ImGui::Button("Add Blur Node")) {
        AddNode(OperationType::Blur, "Blur Node", ImVec2(250, 100));
    }
    if (ImGui::Button("Add Brightness Node")) {
        AddNode(OperationType::Brightness, "Brightness Node", ImVec2(250, 200));
    }
    if (ImGui::Button("Add Load Image Node")) {
        AddNode(OperationType::LoadImage, "Load Image", ImVec2(250, 300));
    }

    ImGui::End();
}

void RenderUI() {
    ShowSidePanel();
    RenderNodes();
}

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Node Editor", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImNodes::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        RenderUI();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImNodes::DestroyContext();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}