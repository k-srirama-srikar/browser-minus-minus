#pragma once

#include <vector>
#include <mutex>
#include <memory>
#include "dom.hpp"

// RenderCommand represents a single rendering instruction
struct RenderCommand {
    enum Type { DrawText, DrawRect, DrawImage, Clear } type;
    float x, y, width, height;
    std::string content;
    std::string color;
    std::string imagePath;
    int fontSize;
    
    RenderCommand() : type(Clear), x(0), y(0), width(0), height(0), fontSize(0) {}
};

// TripleBuffer for thread-safe passing of render trees from Logic thread to Main thread
// Uses three buffers in rotation: back (currently being written), mid (ready to read), front (being read)
template<typename T>
class TripleBuffer {
private:
    std::vector<T> buffers[3];  // back=0, mid=1, front=2
    mutable int backIdx = 0, midIdx = 1, frontIdx = 2;
    std::mutex updateMutex;
    mutable std::mutex readMutex;
    
public:
    TripleBuffer() = default;
    
    // Get reference to back buffer for writing (from Logic thread)
    std::vector<T>& getBackBuffer() {
        return buffers[backIdx];
    }
    
    // Swap back and mid buffers - called after logic thread finishes updating
    void swapBuffers() {
        std::lock_guard<std::mutex> lock(updateMutex);
        int oldMid = midIdx;
        midIdx = backIdx;
        backIdx = oldMid;
    }
    
    // Get front buffer for reading (from Main/Render thread)
    const std::vector<T>& getFrontBuffer() const {
        std::lock_guard<std::mutex> lock(readMutex);
        // Swap mid and front if available
        if (midIdx != frontIdx) {
            int oldFront = frontIdx;
            frontIdx = midIdx;
            midIdx = oldFront;
        }
        return buffers[frontIdx];
    }
    
    // Clear all buffers
    void clearAll() {
        std::lock_guard<std::mutex> lock(updateMutex);
        buffers[0].clear();
        buffers[1].clear();
        buffers[2].clear();
    }
};
