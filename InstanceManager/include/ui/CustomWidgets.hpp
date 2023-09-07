#include <algorithm>
#include <type_traits>

#include "imgui.h"

namespace ImGui {

	template<typename T, typename = std::enable_if_t<std::is_arithmetic<T>::value>>
	bool InputScalarWithRange(const char* label, T* p_data, T p_min = std::numeric_limits<T>::min(), T p_max = std::numeric_limits<T>::max(), T p_step = 1, T p_step_fast = 1, const char* format = nullptr, ImGuiInputTextFlags flags = 0) {
		if (p_min > p_max)
			return false;

		bool value_changed = ImGui::InputScalar(label, GetDataType<T>(), p_data, &p_step, &p_step_fast, format, flags);

		if (value_changed) {
			*p_data = std::clamp(*p_data, p_min, p_max);
		}

		return value_changed;
	}

	template<typename T, typename = std::enable_if_t<std::is_arithmetic<T>::value>>
	bool InputScalarWithRange(const char* label, T* p_data, const T* p_min, const T* p_max, const T* p_step = nullptr, const T* p_step_fast = nullptr, const char* format = nullptr, ImGuiInputTextFlags flags = 0) {
		if (p_min && p_max && *p_min > *p_max)
			return false;

		bool value_changed = ImGui::InputScalar(label, GetDataType<T>(), p_data, p_step, p_step_fast, format, flags);

		if (value_changed) {
			*p_data = std::clamp(*p_data, p_min ? *p_min : std::numeric_limits<T>::min(), p_max ? *p_max : std::numeric_limits<T>::max());
		}

		return value_changed;
	}

	template<typename T>
	ImGuiDataType GetDataType();

	template<>
	ImGuiDataType GetDataType<float>() {
		return ImGuiDataType_Float;
	}

	template<>
	ImGuiDataType GetDataType<int>() {
		return ImGuiDataType_S32;
	}
}// namespace ImGui
