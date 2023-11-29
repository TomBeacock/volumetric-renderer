#pragma once

#include <glm/glm.hpp>
#include <imgui.h>

#include <string>
#include <vector>

namespace Vol::UI::Components
{
struct Gradient {
    template <typename T>
    struct Marker {
        float location;
        T value;
    };

    using ColorMarker = Marker<glm::vec3>;
    using AlphaMarker = Marker<float>;

    std::vector<ColorMarker> color_markers;
    std::vector<AlphaMarker> alpha_markers;

    explicit Gradient();

    glm::vec3 sample_color(float location);
    float sample_alpha(float location);
    glm::vec4 sample(float location);
    std::vector<glm::vec4> discretize(size_t count);

    std::pair<bool, size_t> add_color_marker(float location, glm::vec3 value);
    std::pair<bool, size_t> add_alpha_marker(float location, float value);
    bool remove_color_marker(size_t index);
    bool remove_alpha_marker(size_t index);
};

template <typename T>
static bool operator<(
    const Gradient::Marker<T> &lhs,
    const Gradient::Marker<T> &rhs)
{
    return lhs.location < rhs.location;
}

template <typename T>
static bool operator>(
    const Gradient::Marker<T> &lhs,
    const Gradient::Marker<T> &rhs)
{
    return lhs.location > rhs.location;
}

template <typename T>
static bool operator<(const Gradient::Marker<T> &lhs, float rhs)
{
    return lhs.location < rhs;
}

struct GradientEditState {
    enum class MarkerType { None, Alpha, Color };

    MarkerType selected_type = MarkerType::None;
    int selected_index = -1;
    bool dragging = false;
};

bool gradient_edit(
    const char *str_id,
    Gradient &gradient,
    GradientEditState &state,
    std::string &status_text);
}  // namespace Vol::UI::Components