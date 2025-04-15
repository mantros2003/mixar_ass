#include "_Node.h"
#include "ImageProcessor.h"
#include <map> // For memoization cache

// Find a node by its unique ID
Node* FindNodeById(int nodeId, std::vector<Node>& nodes) {
    for (Node& node : nodes) {
        if (node.id == nodeId) {
            return &node;
        }
    }
    return nullptr;
}

// Find the link connected TO a specific input attribute ID
const Link* FindLinkConnectedToInput(int inputAttrId, std::vector<Link>& links) {
    for (const Link& link : links) {
        if (link.toSlot == inputAttrId) {
            return &link;
        }
    }
    return nullptr;
}

// Find the node whose OUTPUT attribute matches the given ID
Node* FindNodeByOutputAttr(int outputAttrId, std::vector<Node>& nodes) {
    for (Node& node : nodes) {
        if (node.outputSlotId == outputAttrId) {
            return &node;
        }
    }
    return nullptr;
}

// Recursive function to process the graph ending at nodeId
// Returns the processed image or an empty Mat on failure
// Uses a cache to avoid reprocessing nodes within a single "Process" click
cv::Mat ProcessGraphRecursive(int nodeId, std::map<int, cv::Mat>& cache, std::vector<Node>& nodes, std::vector<Link>& links) {
    // Check cache first
    if (cache.count(nodeId)) {
        return cache[nodeId];
    }

    Node* currentNode = FindNodeById(nodeId, nodes);
    if (!currentNode) {
        std::cerr << "Error: Node not found during processing: " << nodeId << std::endl;
        return cv::Mat(); // Return empty Mat on error
    }

    cv::Mat resultImage;

    switch (currentNode->type) {
        case OperationType::LoadImage:
            if (currentNode->imagePath.has_value() && !currentNode->imagePath.value().empty()) {
                // Use the already loaded image if available and path matches, otherwise load
                // Note: This assumes LoadImage nodes load when path changes.
                // If not, loading needs to happen here explicitly.
                 if (!currentNode->loadedCvImage.has_value()) {
                    std::cout << "Processing: Loading image for node " << nodeId << std::endl;
                     currentNode->loadedCvImage = ImageProcessor::loadImage(currentNode->imagePath.value());
                 }

                if (currentNode->loadedCvImage.has_value()) {
                    resultImage = currentNode->loadedCvImage.value().clone(); // Clone to avoid modifying cache
                } else {
                     std::cerr << "Error: Failed to load image for node " << nodeId << " path: " << currentNode->imagePath.value() << std::endl;
                     resultImage = cv::Mat();
                }
            } else {
                 std::cerr << "Error: No image path for LoadImage node " << nodeId << std::endl;
                 resultImage = cv::Mat();
            }
            break;

        case OperationType::Brightness:
        case OperationType::Blur:
            // --- Process Ancestor Node(s) ---
            {
                const Link* inputLink = FindLinkConnectedToInput(currentNode->inputSlotId, links);
                if (!inputLink) {
                    std::cerr << "Error: Input node " << nodeId << " is not connected." << std::endl;
                    resultImage = cv::Mat();
                    break; // Exit switch case
                }

                Node* prevNode = FindNodeByOutputAttr(inputLink->fromSlot, nodes);
                if (!prevNode) {
                     std::cerr << "Error: Could not find node connected to input of " << nodeId << std::endl;
                     resultImage = cv::Mat();
                     break; // Exit switch case
                }

                // Recursively process the previous node
                cv::Mat inputImage = ProcessGraphRecursive(prevNode->id, cache, nodes, links);

                if (inputImage.empty()) {
                    std::cerr << "Error: Input image for node " << nodeId << " is empty." << std::endl;
                    resultImage = cv::Mat(); // Propagate error
                } else {
                    // --- Apply Current Node's Operation ---
                    std::cout << "Processing: Applying operation for node " << nodeId << " (" << currentNode->name << ")" << std::endl;
                    float value = currentNode->value.value_or(0.0f); // Get value safely

                    if (currentNode->type == OperationType::Brightness) {
                         resultImage = ImageProcessor::applyBrightness(inputImage, static_cast<int>(value)); // Assuming value is brightness offset
                    } else if (currentNode->type == OperationType::Blur) {
                         int kernelSize = static_cast<int>(value);
                         if (kernelSize <= 0 || kernelSize % 2 == 0) {
                             kernelSize = 3; // Default to 3 if value is invalid
                             std::cerr << "Warning: Invalid blur kernel size (" << value << ") for node " << nodeId << ". Using 3." << std::endl;
                         }
                         resultImage = ImageProcessor::applyBlur(inputImage, kernelSize);
                    }
                    // Add other processing node types here...
                }
            }
            break;

        case OperationType::ProcessDisplay:
            // Should not be called directly on ProcessDisplay node in this recursive function
            std::cerr << "Error: ProcessGraphRecursive called on ProcessDisplay node " << nodeId << std::endl;
            resultImage = cv::Mat();
            break;

        default:
            std::cerr << "Error: Unknown node type encountered during processing: " << static_cast<int>(currentNode->type) << std::endl;
            resultImage = cv::Mat();
            break;

    }

    // Store result in cache before returning
    cache[nodeId] = resultImage;
    return resultImage;
}