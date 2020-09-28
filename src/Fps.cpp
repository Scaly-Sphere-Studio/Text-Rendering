#include "SSS/Text-Rendering/Fps.hpp"

SSS_TR_BEGIN__

    // --- Constructor ---

// Constructor, calls the SSS::TR::TextArea constructor with given parameters
Fps::Fps(int w, int h, TR::TextOpt const& opt) try
    : TextArea(w, h)
{
    opt_ = opt;
}
CATCH_AND_RETHROW_METHOD_EXC__

    // --- Basic functions ---

// Adds one frame to the counter.
// Calculates FPS value every second, and return true if the value changed.
bool Fps::addFrame()
{
    ++frames_;
    return calculateFps_();
}

// Returns the FPS value
long long Fps::get() const noexcept
{
    return fps_;
}

    // --- Private functions ---

// Calculates FPS value every second, and return true if the value changed.
bool Fps::calculateFps_() try
{
    using namespace std::chrono;
    seconds const sec = duration_cast<seconds>(system_clock::now() - stored_time_);

    // Update fps every second
    if (sec >= seconds(1)) {
        // Determine fps
        long long const new_fps = frames_ / sec.count();
        // Reset fps and increase stored time
        frames_ = 0;
        stored_time_ += sec;
        // Update old fps if needed
        if (new_fps != fps_) {
            fps_ = new_fps;
            clear();
            loadString(std::to_string(fps_), opt_);
            return true;
        }
    }
    return false;
}
CATCH_AND_RETHROW_METHOD_EXC__

SSS_TR_END__
