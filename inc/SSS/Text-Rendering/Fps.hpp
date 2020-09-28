#pragma once

#include "SSS/Text-Rendering/TextArea.hpp"

SSS_TR_BEGIN__

class Fps : private TextArea {
public:
// --- Inherited ---
    
    // Inherited from TextArea
    using TextArea::renderTo;

// --- Constructor ---

    // Constructor, calls the TextArea constructor with given parameters
    Fps(int w, int h, TextOpt const& opt);

// --- Basic functions ---

    // Adds one frame to the counter.
    // Calculates FPS value every second, and return true if the value changed.
    bool addFrame();
    // Returns the FPS value
    long long get() const noexcept;

private:
// --- Private functions ---

    // Calculates FPS value every second, and return true if the value changed.
    bool calculateFps_();

// --- Variables ---

    long long frames_{ 0 }; // Frames counter
    long long fps_{ 0 };    // FPS value

    TextOpt opt_{ nullptr };    // Options used to write the FPS
    
    std::chrono::system_clock::time_point const start_time_
        { std::chrono::system_clock::now() };   // Time the instance was created
    std::chrono::system_clock::time_point stored_time_
        { start_time_ };    // Last time the FPS value was calculated
};

SSS_TR_END__
