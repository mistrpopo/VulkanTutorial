#pragma once
#include <vulkan/vulkan.h>
#include <functional>
#include <sstream>
#include <vector>
#include <algorithm>
#include <set>
#include <fstream>
#include <glm/glm.hpp> //linear algebra related types like vectors and matrices
#include <array>

//VDeleter : wrapper class to make sure we always cleanup VkObject-s

template <typename T>
class VDeleter {
public:
	VDeleter() : VDeleter([](T _) {}) {}

	VDeleter(std::function<void(T, VkAllocationCallbacks*)> deletef) {
		object = VK_NULL_HANDLE;
		this->deleter = [=](T obj) { deletef(obj, nullptr); };
	}

	VDeleter(const VDeleter<VkInstance>& instance, std::function<void(VkInstance, T, VkAllocationCallbacks*)> deletef) {
		object = VK_NULL_HANDLE;
		this->deleter = [&instance, deletef](T obj) { deletef(instance, obj, nullptr); };
	}

	VDeleter(const VDeleter<VkDevice>& device, std::function<void(VkDevice, T, VkAllocationCallbacks*)> deletef) {
		object = VK_NULL_HANDLE;
		this->deleter = [&device, deletef](T obj) { deletef(device, obj, nullptr); };
	}

	~VDeleter() {
		cleanup();
	}

	T* operator &() {
		cleanup();
		return &object;
	}

	operator T() const {
		return object;
	}

private:
	T object;
	std::function<void(T)> deleter;

	void cleanup() {
		if (object != VK_NULL_HANDLE) {
			deleter(object);
		}
		object = VK_NULL_HANDLE;
	}
};

VkResult CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback) {
	auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pCallback);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
	if (func != nullptr) {
		func(instance, callback, pAllocator);
	}
}

std::string FormatDebugMessage(VkDebugReportFlagsEXT flags, const char* msg) {
	std::stringstream s;
	s 
		<< "::"
		<< (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT ? "INFORMATION::" : "")
		<< (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT ? "WARNING::" : "")
		<< (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT ? "PERFORMANCE_WARNING::" : "")
		<< (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT ? "ERROR::" : "")
		<< (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT ? "DEBUG::" : "")
		<< msg;

	return s.str();
}


enum QueueFamilyProperty {
	GraphicsFamily,
	PresentFamily,
	NumberOfProperties
};

struct QueueFamilyIndices {
	QueueFamilyIndices() {}
	QueueFamilyIndices(VkPhysicalDevice device, VkSurfaceKHR surface) {
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		for (const auto& queueFamily : queueFamilies) {
			int i = int(&queueFamily - &*queueFamilies.begin());

			if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				index[GraphicsFamily] = i;
			}
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
			if (queueFamily.queueCount > 0 && presentSupport) {
				index[PresentFamily] = i;
			}

			if (isComplete()) {
				break;
			}
		}
	}

	int operator[](int i) { return index[i]; }
	std::set<int> uniqueQueueFamilies() { return std::set<int>(index.begin(), index.end()); }
	bool isComplete() {
		return std::all_of(index.begin(), index.end(), [](int i) { return i != -1; });
	}

	std::vector<uint32_t> index = std::vector<uint32_t>(NumberOfProperties, -1);
};

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

static std::vector<char> loadFile(const std::string& filename) {
	std::ifstream file(filename, std::ios::ate | std::ios::binary);
	if (!file.is_open()) {
		throw std::runtime_error("failed to open file!");
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);
	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();

	return buffer;
}

template<class T>
void setData(uint32_t &count, T *&data, const std::vector<T> &vec) //todo
{
	count = static_cast<uint32_t>(vec.size());
	data = vec.data();
}

struct Vertex {
	glm::vec2 pos;
	glm::vec3 color;


	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription = {};

		//describe the memory structure of the buffer to Vulkan
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions = {};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		return attributeDescriptions;
	}
};