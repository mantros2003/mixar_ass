#define IMGUI_DEFINE_MATH_OPERATORS

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imnodes.h"
#include "imgui_internal.h"
#include "ImageProcessor.h"
#include "_Node.h"
#include "utils.cpp"
#include <GLFW/glfw3.h>
#include <opencv2/opencv.hpp>
#include <vector>
#include <string>
#include <optional>
#include <map>
#include <iostream>

std::vector<Node> nodes;
std::vector<Link> links;
int nodeCounter = 0;
int slotCounter = 1000;
int linkCounter = 0;

void displayImage(Node& node) {
    // Calculate display size, maintaining aspect ratio within node width
    float aspectRatio = (float)node.imageHeight / (float)node.imageWidth;
    float displayWidth = node.width - ImGui::GetStyle().FramePadding.x * 2; // Use node width minus padding
    float displayHeight = displayWidth * aspectRatio;

    // Center the image horizontally
    float spaceX = (node.width - displayWidth) / 2.0f;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + spaceX);

    ImGui::Image((ImTextureID)(uintptr_t)node.textureId, ImVec2(displayWidth, displayHeight));
}

void AddNode(OperationType type, const std::string& name, ImVec2 pos) {
    Node node;
    node.id = nodeCounter++;
    node.type = type;
    node.name = name;
    node.position = pos;
    node.width = 150; // Default width

    // Assign slots based on type
    if (type == OperationType::LoadImage) {
        node.outputSlotId = slotCounter++; // Load has output only
    } else if (type == OperationType::ProcessDisplay) {
        node.inputSlotId = slotCounter++; // ProcessDisplay has input only
        node.width = 200; // Maybe make it wider by default
    } else { // Processing nodes (Blur, Brightness)
        node.inputSlotId = slotCounter++;
        node.outputSlotId = slotCounter++;
        node.value = 0.0f; // Default value for processing nodes
    }

    nodes.push_back(node);
}

void handleNodeConnection(int startAttr, int endAttr) {
    // --- Check if the target input slot already has a connection ---
    bool targetIsInput = false;
    bool alreadyConnected = false;
    int targetNodeId = -1; // To identify which node the target attribute belongs to

    // Identify if endAttr is an input slot of a processing node
    // and if it's already connected.
    for (const Node& node : nodes) {
        // Check if the end attribute belongs to this node's input slot
        if (node.inputSlotId == endAttr) {
            targetNodeId = node.id;
            // Check if it's a type that should only have one input
            if (node.type == OperationType::Brightness || node.type == OperationType::Blur) {
                 targetIsInput = true;
                 break; // Found the node and it's a relevant type
            }
        }
    }

    // If the target attribute is an input slot for a processing node,
    // check if any existing link already connects to it.
    if (targetIsInput) {
        for (const auto& link : links) {
            if (link.toSlot == endAttr) {
                alreadyConnected = true;
                break;
            }
        }
    }

    // --- Add the link only if the target is not an already connected input slot ---
    //    (or if the target is an output slot, which can have multiple connections originating from it)
    if (!alreadyConnected || !targetIsInput) {
        // If the target isn't an input OR if it is an input but isn't connected yet, add the link.
        // Also handles the case where the link starts from an input (which shouldn't happen ideally)
        // or ends at an output (which is allowed).
        // You might want to add further checks to ensure startAttr is an output and endAttr is an input.

        links.push_back({linkCounter++, startAttr, endAttr});
    } else {
        std::cout << "Node input already connected. Cannot add new link." << std::endl;
    }
}

// --- Helper function to create/update OpenGL texture from cv::Mat ---
//     Place this function somewhere accessible, e.g., before RenderLoadImageNode
bool CreateOrUpdateTexture(const cv::Mat& image, GLuint& textureId) {
    if (image.empty()) {
        return false;
    }

    // If texture already exists, delete it first
    if (textureId != 0) {
        glDeleteTextures(1, &textureId);
        textureId = 0;
    }

    // Create new texture
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);

    // Set texture parameters (you might want different settings)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // Avoid border artifacts
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Convert cv::Mat BGR to RGBA for OpenGL
    cv::Mat imageRGBA;
    if (image.channels() == 3) {
        cv::cvtColor(image, imageRGBA, cv::COLOR_BGR2RGBA);
    } else if (image.channels() == 1) {
        cv::cvtColor(image, imageRGBA, cv::COLOR_GRAY2RGBA);
    } else if (image.channels() == 4) {
        // Assuming it's BGRA, convert to RGBA
         cv::cvtColor(image, imageRGBA, cv::COLOR_BGRA2RGBA);
    }
     else {
        // Handle other cases or return false if unsupported format
        glBindTexture(GL_TEXTURE_2D, 0); // Unbind
        glDeleteTextures(1, &textureId); // Clean up allocated texture ID
        textureId = 0;
        return false;
    }


    // Upload image data to texture
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0); // Ensure correct alignment
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageRGBA.cols, imageRGBA.rows, 0,
                   GL_RGBA, GL_UNSIGNED_BYTE, imageRGBA.data);

    glBindTexture(GL_TEXTURE_2D, 0); // Unbind texture

    return true;
}

// --- Helper Function for Load Image Nodes ---
void RenderLoadImageNode(Node& node) {
    // Note: pathBuffers should ideally be managed differently if nodes can be deleted,
    // but keeping static for simplicity based on previous code.
    static std::map<int, std::string> pathBuffers;

    // --- Input Path Text Field ---
    char buf[64];
    snprintf(buf, sizeof(buf), "##path%d", node.id);

    std::string currentPath = node.imagePath.value_or("");
    if (pathBuffers.find(node.id) == pathBuffers.end()) {
        pathBuffers[node.id] = currentPath;
    }

    char inputText[256];
    strncpy(inputText, pathBuffers[node.id].c_str(), sizeof(inputText));
    inputText[sizeof(inputText) - 1] = '\0';

    bool pathChanged = false;
    if (ImGui::InputText(buf, inputText, IM_ARRAYSIZE(inputText), ImGuiInputTextFlags_EnterReturnsTrue)) {
        node.imagePath = inputText;
        pathBuffers[node.id] = inputText;
        currentPath = inputText; // Update currentPath after change
        pathChanged = true;
    }

    // --- Output Attribute (LoadImage only has output) ---
    ImNodes::BeginOutputAttribute(node.outputSlotId);
    // Align text to the right for output node
    ImGui::Indent(node.width - ImGui::CalcTextSize("Output").x - ImGui::GetStyle().FramePadding.x * 2); // Adjust padding
    ImGui::Text("Output");
    ImGui::Unindent(); // Match Indent
    ImNodes::EndOutputAttribute();


    // --- Load Image and Update Texture if Path Changed ---
    // Check if path changed OR if it's different from the last successfully loaded path
    if (pathChanged) {
        // --- This is the block that now runs only when pathChanged is true ---
        if (node.imagePath.has_value() && !node.imagePath.value().empty()) {
            // Attempt to load the image
            node.loadedCvImage = ImageProcessor::loadImage(node.imagePath.value()); // loadImage returns cv::Mat [cite: 3]

            if (node.loadedCvImage.has_value() && !node.loadedCvImage.value().empty()) {
                 // Successfully loaded, update texture
                if (CreateOrUpdateTexture(node.loadedCvImage.value(), node.textureId)) {
                    node.imageWidth = node.loadedCvImage.value().cols;
                    node.imageHeight = node.loadedCvImage.value().rows;
                    // node.lastLoadedPath = node.imagePath.value(); // Optional: Keep track if needed elsewhere
                } else {
                    // Texture creation failed
                    node.textureId = 0; // Ensure texture ID is reset
                    // node.lastLoadedPath = ""; // Reset if using lastLoadedPath tracking
                    node.loadedCvImage.reset(); // Clear the cv::Mat too
                    node.imageWidth = 0;
                    node.imageHeight = 0;
                    std::cerr << "Error: Failed to create texture for " << node.imagePath.value() << std::endl;
                }

            } else {
                // Loading failed (ImageProcessor::loadImage returned empty Mat) [cite: 3]
                std::cerr << "Error: Failed to load image " << node.imagePath.value() << std::endl;
                // Delete existing texture if any
                if (node.textureId != 0) {
                    glDeleteTextures(1, &node.textureId);
                    node.textureId = 0;
                }
                node.imageWidth = 0;
                node.imageHeight = 0;
                // node.lastLoadedPath = ""; // Reset if using lastLoadedPath tracking
                node.loadedCvImage.reset(); // Clear the cv::Mat
            }
        } else {
            // Path is empty, clear resources
            if (node.textureId != 0) {
                glDeleteTextures(1, &node.textureId);
                node.textureId = 0;
            }
            node.imageWidth = 0;
            node.imageHeight = 0;
            // node.lastLoadedPath = ""; // Reset if using lastLoadedPath tracking
            node.loadedCvImage.reset();
        }
        // --- End of block that runs only when pathChanged is true ---
    }

    // --- Display Image using ImGui::Image ---
    if (node.textureId != 0 && node.imageWidth > 0 && node.imageHeight > 0) {
        displayImage(node);
    } else {
        // Optionally display a placeholder if no image is loaded/valid
        ImGui::TextDisabled("No image loaded");
    }
}

// --- Helper Function for Processing Nodes (Brightness, Blur) ---
void RenderProcessingNode(Node& node) {
    // Specific rendering logic for nodes like Brightness, Blur

    // Input Attribute
    ImNodes::BeginInputAttribute(node.inputSlotId);
    ImGui::Text("Input");
    ImNodes::EndInputAttribute();

    // Value Input
    // Note: 'inputBuffers' needs to be accessible here.
    static std::map<int, std::string> inputBuffers;
    char buf[32];
    snprintf(buf, sizeof(buf), "##val%d", node.id);

    // Use .value_or(0.0f) to provide a default if optional is empty
    if (inputBuffers.find(node.id) == inputBuffers.end())
        inputBuffers[node.id] = std::to_string(node.value.value_or(0.0f));

    char input[32];
    strncpy(input, inputBuffers[node.id].c_str(), sizeof(input));
    input[sizeof(input) - 1] = '\0';

    if (ImGui::InputText(buf, input, IM_ARRAYSIZE(input), ImGuiInputTextFlags_EnterReturnsTrue)) {
        try {
            node.value = std::stof(input); // Assign directly to optional float
            inputBuffers[node.id] = input;
        } catch (...) {
            // Optional: Add visual feedback for parse error
        }
    }

    // Output Attribute
    ImNodes::BeginOutputAttribute(node.outputSlotId);
    // Align text to the right for output node
    ImGui::Indent(node.width - ImGui::CalcTextSize("Output").x - ImGui::GetStyle().FramePadding.x);
    ImGui::Text("Output");
    ImNodes::EndOutputAttribute();
}


// Remember to include the helper for texture creation from previous steps
// bool CreateOrUpdateTexture(const cv::Mat& image, GLuint& textureId);

void RenderProcessDisplayNode(Node& node) {
    // Input Attribute
    ImNodes::BeginInputAttribute(node.inputSlotId);
    ImGui::Text("Input");
    ImNodes::EndInputAttribute();

    ImGui::Spacing();

    // Process Button
    if (ImGui::Button("Process Graph")) {
        node.processingRequested = true; // Set flag to process
    }

    // --- Trigger Processing and Immediately Update processedImage ---
    if (node.processingRequested) {
        node.processingRequested = false; // Reset flag

        const Link* inputLink = FindLinkConnectedToInput(node.inputSlotId, links);
        if (inputLink) {
            Node* prevNode = FindNodeByOutputAttr(inputLink->fromSlot, nodes);
            if (prevNode) {
                std::cout << "--- Processing Triggered for Node " << node.id << " ---" << std::endl;
                std::map<int, cv::Mat> processingCache;
                cv::Mat result = ProcessGraphRecursive(prevNode->id, processingCache, nodes, links);

                if (!result.empty()) {
                    std::cout << "--- Processing Finished. Updating Texture and Processed Image for Node " << node.id << " ---" << std::endl;
                    node.loadedCvImage = result; // Store the final result
                    node.processedImage = result.clone(); // Store the processed image for saving

                    // Update this node's texture
                    if (CreateOrUpdateTexture(node.loadedCvImage.value(), node.textureId)) {
                        node.imageWidth = node.loadedCvImage.value().cols;
                        node.imageHeight = node.loadedCvImage.value().rows;
                    } else {
                        // Texture creation failed
                        node.textureId = 0;
                        node.imageWidth = 0;
                        node.imageHeight = 0;
                        node.loadedCvImage.reset();
                        node.processedImage.reset();
                        std::cerr << "Error: Failed to create texture for ProcessDisplay node " << node.id << std::endl;
                    }
                } else {
                    std::cerr << "Error: Processing graph for node " << node.id << " resulted in an empty image." << std::endl;
                    // Clear previous result if processing failed
                    if (node.textureId != 0) glDeleteTextures(1, &node.textureId);
                    node.textureId = 0;
                    node.imageWidth = 0;
                    node.imageHeight = 0;
                    node.loadedCvImage.reset();
                    node.processedImage.reset();
                }
            } else {
                std::cerr << "Error: Could not find node connected to input of ProcessDisplay node " << node.id << std::endl;
                // Clear results if no input node
                if (node.textureId != 0) glDeleteTextures(1, &node.textureId);
                node.textureId = 0;
                node.imageWidth = 0;
                node.imageHeight = 0;
                node.loadedCvImage.reset();
                node.processedImage.reset();
            }
        } else {
            std::cerr << "Error: ProcessDisplay node " << node.id << " is not connected." << std::endl;
            // Clear results if not connected
            if (node.textureId != 0) glDeleteTextures(1, &node.textureId);
            node.textureId = 0;
            node.imageWidth = 0;
            node.imageHeight = 0;
            node.loadedCvImage.reset();
            node.processedImage.reset();
        }
    }

    // --- Display Result Image ---
    if (node.textureId != 0 && node.imageWidth > 0 && node.imageHeight > 0) {
        displayImage(node);
    } else {
        // Placeholder text
        ImGui::TextDisabled("Result will appear here");
    }

    // --- Save Button Rendering ---
    if (node.processedImage.has_value() && !node.processedImage.value().empty()) {
        if (ImGui::Button(("Save Image##" + std::to_string(node.id)).c_str())) {
            std::string filename = "output_" + std::to_string(node.id) + ".png";
            ImageProcessor::saveImage(node.processedImage.value(), filename);
            std::cout << "Saved processed image to " << filename << std::endl;
        }
    } else {
        ImGui::Text("No image to save");
    }
}


// --- Modified RenderNodes Function ---
void RenderNodes() {
    ImGui::SetNextWindowPos(ImVec2(200, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize - ImVec2(200, 0), ImGuiCond_Always);

    ImGui::Begin("Node Editor Area", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
    ImNodes::BeginNodeEditor();

    for (Node& node : nodes) {
        ImNodes::BeginNode(node.id);

        // --- Title (Common to all nodes) ---
        ImNodes::BeginNodeTitleBar();
        ImGui::TextUnformatted(node.name.c_str());
        ImNodes::EndNodeTitleBar();

        ImGui::PushItemWidth(node.width); // Set width for inputs inside the node

        // --- Call Specific Renderer based on Type ---
        switch (node.type) {
            case OperationType::LoadImage:
                RenderLoadImageNode(node);
                break;
            case OperationType::Brightness:
            case OperationType::Blur:
                RenderProcessingNode(node);
                break;
            case OperationType::ProcessDisplay: // New case
                RenderProcessDisplayNode(node);
                break;
            // Add cases for future node types here
            // default:
            //     ImGui::Text("Unknown Node Type"); // Optional: Handle unexpected types
            //     break;
        }

        ImGui::PopItemWidth(); // Matches PushItemWidth

         // --- Resizer (Common to all nodes, position might need adjustment) ---
        // This positioning might need tweaking depending on the content of each node type.
        // Consider making the resizer positioning relative to the node's bottom-right corner.
        ImVec2 node_pos = ImNodes::GetNodeDimensions(node.id); // Requires ImNodes > v0.4 maybe? check version
        // Fallback if GetNodeDimensions isn't available or suitable:
        // ImVec2 grip_pos = ImGui::GetCursorScreenPos(); // This depends heavily on layout flow
        // Alternative: Position relative to node position from ImNodes if available

        // Simple positioning based on current cursor (might overlap content)
        // ImGui::SetCursorScreenPos(ImVec2(ImGui::GetItemRectMin().x + node.width - 10, ImGui::GetItemRectMax().y - 10)); // Example positioning attempt

        // Keeping original logic for now, but be aware it might need refinement:
        ImVec2 current_cursor = ImGui::GetCursorScreenPos(); // Where cursor is *after* rendering node content
        ImVec2 node_screen_pos = ImNodes::GetNodeScreenSpacePos(node.id);
        float node_height = ImGui::GetItemRectSize().y; // Height of the *last* item added
        // It's hard to get the *total* node height accurately without ImNodes helpers
        // This resizer logic might be better placed *outside* the specific render functions
        // or use a fixed offset if node heights are predictable.

        // Let's try putting it consistently after PopItemWidth but before EndNode
        ImGui::SameLine(); // Try to place it on the same line if possible? May not work well.
        ImGui::PushID(node.id); // Unique ID for each grip
        // Use a small invisible button as a grab handle area
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + node.width - 20); // Move towards the right edge
        if (ImGui::Button(">", ImVec2(20, 20))) {}; // Resizer visual (can be styled/made invisible)
        if (ImGui::IsItemActive()) {
            node.width += ImGui::GetIO().MouseDelta.x;
            node.width = ImClamp(node.width, 100.0f, 300.0f); // Clamp to a reasonable range
        }
        ImGui::PopID();


        ImNodes::EndNode();
    }

    // Render links (unchanged)
    for (const auto& link : links) {
        ImNodes::Link(link.id, link.fromSlot, link.toSlot);
    }

    ImNodes::EndNodeEditor();

    // Check for new links (unchanged)
    int startAttr, endAttr;
if (ImNodes::IsLinkCreated(&startAttr, &endAttr)) {
    handleNodeConnection(startAttr, endAttr);
}

    // Check for deleted links/nodes (You would add this logic here if needed)
    // ...

    ImGui::End(); // End Node Editor Area window
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
    if (ImGui::Button("Add Process/Display Node")) {
        AddNode(OperationType::ProcessDisplay, "Process & Display", ImVec2(250, 400)); // Adjust position
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