#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <opencv2/opencv.hpp>

#include <GLFW/glfw3.h>
#include <iostream>

struct Node {
    std::string title;
    int id;
    ImVec2 position;
    char inputText[128] = "";
    std::string savedValue;
    cv::Mat imageMat;                   // OpenCV image
    GLuint textureID = 0;               // OpenGL texture

    Node(const std::string& t, int id, ImVec2 pos)
        : title(t), id(id), position(pos), savedValue("") {}
};

std::vector<Node> nodes;

int main() {
    // Init GLFW
    if (!glfwInit()) return -1;

    // Setup OpenGL (3.2 Core Profile, required on macOS)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Required on macOS

    GLFWwindow* window = glfwCreateWindow(800, 600, "ImGui Button Window", nullptr, nullptr);
    if (!window) return -1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Init ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 150 core");

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Set position and size for the side panel (left side of screen)
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(200, ImGui::GetIO().DisplaySize.y), ImGuiCond_Always);

        // Begin the window as a fixed, borderless panel
        ImGui::Begin("Add Nodes", nullptr,
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoBringToFrontOnFocus);

        // Add your buttons
        ImGui::Text("Add Nodes");
        ImGui::Separator();

        static int nodeCounter = 0;

        if (ImGui::Button("Add Blur Node")) {
            nodes.emplace_back("Blur Node", nodeCounter++, ImVec2(250, 50 + nodes.size() * 80));
        }

        if (ImGui::Button("Add Brightness Node")) {
            nodes.emplace_back("Brightness Node", nodeCounter++, ImVec2(250, 50 + nodes.size() * 80));
        }

        if (ImGui::Button("Add Load Image Node")) {
            nodes.emplace_back("Load Image Node", nodeCounter++, ImVec2(250, 50 + nodes.size() * 80));
        }        

        ImGui::End();

        int nodeToDelete = -1;  // -1 means no deletion requested

        for (Node& node : nodes) {
            std::string windowTitle = node.title + "##" + std::to_string(node.id);
            ImGui::SetNextWindowPos(node.position, ImGuiCond_FirstUseEver);
            ImGui::Begin(windowTitle.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        
            if (node.title == "Load Image Node") {
                ImGui::InputText("Image Path", node.inputText, IM_ARRAYSIZE(node.inputText),
                                 ImGuiInputTextFlags_EnterReturnsTrue);
        
                if (ImGui::IsItemDeactivatedAfterEdit()) {
                    node.savedValue = node.inputText;
        
                    // Load image with OpenCV
                    node.imageMat = cv::imread(node.savedValue);
                    if (!node.imageMat.empty()) {
                        // Convert BGR to RGB
                        cv::cvtColor(node.imageMat, node.imageMat, cv::COLOR_BGR2RGB);
                        cv::flip(node.imageMat, node.imageMat, 0);
        
                        // Generate OpenGL texture
                        if (node.textureID) glDeleteTextures(1, &node.textureID);

                        glGenTextures(1, &node.textureID);
                        glBindTexture(GL_TEXTURE_2D, node.textureID);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

                        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

                        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
                                    node.imageMat.cols, node.imageMat.rows,
                                    0, GL_RGB, GL_UNSIGNED_BYTE, node.imageMat.data);

                        glBindTexture(GL_TEXTURE_2D, 0); // Unbind

                    }
                }
        
                // Show texture if available
                if (node.textureID) {
                    float maxDisplayWidth = 200.0f;
                    float scale = maxDisplayWidth / node.imageMat.cols;
                    ImVec2 displaySize = ImVec2(
                        maxDisplayWidth,
                        node.imageMat.rows * scale
                    );
                    ImGui::Image((ImTextureID)node.textureID, displaySize,
                    ImVec2(0.0f, 1.0f),  // Top-left
                    ImVec2(1.0f, 0.0f)); // Bottom-right;
                }
            }
        
            node.position = ImGui::GetWindowPos();
            ImGui::End();
        }
        

        // for (int i = 0; i < nodes.size(); ++i) {
        //     Node& node = nodes[i];

        //     std::string windowTitle = node.title + "##" + std::to_string(node.id);

        //     ImGui::SetNextWindowPos(node.position, ImGuiCond_FirstUseEver);
        //     ImGui::Begin(windowTitle.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        //     if (ImGui::InputText("Input", node.inputText, IM_ARRAYSIZE(node.inputText), ImGuiInputTextFlags_EnterReturnsTrue)) {
        //         node.savedValue = node.inputText;
        //         std::cout << "Saved: " << node.savedValue << std::endl;
        //     }

        //     // 🗑️ Remove button
        //     if (ImGui::Button("Remove Node")) {
        //         nodeToDelete = i;  // mark this node for deletion
        //     }

        //     node.position = ImGui::GetWindowPos();

        //     ImGui::End();
        // }

        if (nodeToDelete != -1) {
            nodes.erase(nodes.begin() + nodeToDelete);
        }

        // Render
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
