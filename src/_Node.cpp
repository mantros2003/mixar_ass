#include "_Node.h"

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