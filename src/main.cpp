#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>

#include <vector>
#include <cstring>

#include <optional>

#include <set>

#include <cstdint>
#include <limits>
#include <algorithm>

const uint32_t WIDTH=800;
const uint32_t HEIGHT=600;

const std::vector<const char*> validation_layers={
	"VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
const bool enable_validation_layers=false;
#else
const bool enable_validation_layers=true;
#endif

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* create_info, const VkAllocationCallbacks* allocator, VkDebugUtilsMessengerEXT* debug_messenger) {
	auto func=(PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if(func!=nullptr) {
		return func(instance, create_info, allocator, debug_messenger);
	} else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debug_messenger, const VkAllocationCallbacks* allocator) {
	auto func=(PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if(func!=nullptr) {
		func(instance, debug_messenger, allocator);
	}
}

struct QueueFamilyIndices {
	std::optional<uint32_t> graphics_family;
	std::optional<uint32_t> present_family;

	bool isComplete() {
		return graphics_family.has_value()&&present_family.has_value();
	}
};

const std::vector<const char*> device_extensions={
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> present_modes;
};

class HelloTriangleApplication {
public:
	void run() {
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();
	}

private:
	GLFWwindow* window;

	VkInstance instance;
	VkDebugUtilsMessengerEXT debug_messenger;

	VkPhysicalDevice physical_device=VK_NULL_HANDLE;

	VkDevice device;

	VkQueue graphics_queue;

	VkSurfaceKHR surface;

	VkQueue presentQueue;

	VkSwapchainKHR swap_chain;

	std::vector<VkImage> swap_chain_images;

	VkFormat swap_chain_image_format;

	VkExtent2D swap_chain_extent;

	std::vector<const char*> getRequiredExtensions() {
		uint32_t glfw_ext_ct=0;
		const char** glfw_extensions;
		glfw_extensions=glfwGetRequiredInstanceExtensions(&glfw_ext_ct);

		std::vector<const char*> extensions(glfw_extensions, glfw_extensions+glfw_ext_ct);

		if(enable_validation_layers) {
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return extensions;
	}

	bool checkValidationLayerSupport() {
		uint32_t layer_ct;
		vkEnumerateInstanceLayerProperties(&layer_ct, nullptr);

		std::vector<VkLayerProperties> avail_layers(layer_ct);
		vkEnumerateInstanceLayerProperties(&layer_ct, avail_layers.data());

		for(const char* layer_name:validation_layers) {
			bool layer_found=false;

			for(const auto& layer_props:avail_layers) {
				if(std::strcmp(layer_name, layer_props.layerName)==0) {
					layer_found=true;
					break;
				}
			}

			if(!layer_found) return false;
		}

		return true;
	}

	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& create_info) {
		create_info={};
		create_info.sType=VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		create_info.messageSeverity=VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		create_info.messageType=VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		create_info.pfnUserCallback=debugCallback;
	}

	void createInstance() {
		if(enable_validation_layers&&!checkValidationLayerSupport()) {
			throw std::runtime_error("validation layers requested, but not available!");
		}

		VkApplicationInfo app_info{};
		app_info.sType=VK_STRUCTURE_TYPE_APPLICATION_INFO;
		app_info.pApplicationName="Hello Triangle";
		app_info.applicationVersion=VK_MAKE_VERSION(1, 0, 0);
		app_info.pEngineName="No Engine";
		app_info.engineVersion=VK_MAKE_VERSION(1, 0, 0);
		app_info.apiVersion=VK_API_VERSION_1_0;

		VkInstanceCreateInfo create_info{};
		create_info.sType=VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		create_info.pApplicationInfo=&app_info;

		auto extensions=getRequiredExtensions();
		create_info.enabledExtensionCount=static_cast<uint32_t>(extensions.size());
		create_info.ppEnabledExtensionNames=extensions.data();

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
		if(enable_validation_layers) {
			create_info.enabledLayerCount=static_cast<uint32_t>(validation_layers.size());
			create_info.ppEnabledLayerNames=validation_layers.data();

			populateDebugMessengerCreateInfo(debugCreateInfo);
			create_info.pNext=(VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
		} else {
			create_info.enabledLayerCount=0;

			create_info.pNext=nullptr;
		}

		if(vkCreateInstance(&create_info, nullptr, &instance)!=VK_SUCCESS) {
			throw std::runtime_error("failed to create instance!");
		}
	}

	void setupDebugMessenger() {
		if(!enable_validation_layers) return;

		VkDebugUtilsMessengerCreateInfoEXT create_info;
		populateDebugMessengerCreateInfo(create_info);

		if(CreateDebugUtilsMessengerEXT(instance, &create_info, nullptr, &debug_messenger)!=VK_SUCCESS) {
			throw std::runtime_error("failed to set up debug messenger!");
		}
	}

	void createSurface() {
		if(glfwCreateWindowSurface(instance, window, nullptr, &surface)!=VK_SUCCESS) {
			throw std::runtime_error("failed to create window surface!");
		}
	}

	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
		QueueFamilyIndices indices;

		uint32_t queue_family_ct=0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_ct, nullptr);

		std::vector<VkQueueFamilyProperties> queue_families(queue_family_ct);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_ct, queue_families.data());

		int i=0;
		for(const auto& queue_family:queue_families) {
			if(queue_family.queueFlags&VK_QUEUE_GRAPHICS_BIT) {
				indices.graphics_family=i;
			}

			VkBool32 present_support=false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);
			if(present_support) {
				indices.present_family=i;
			}

			if(indices.isComplete()) break;

			i++;
		}

		return indices;
	}

	bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
		uint32_t extension_ct;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_ct, nullptr);

		std::vector<VkExtensionProperties> avail_extensions(extension_ct);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_ct, avail_extensions.data());

		std::set<std::string> required_extensions(device_extensions.begin(), device_extensions.end());

		for(const auto& extension:avail_extensions) {
			required_extensions.erase(extension.extensionName);
		}

		return required_extensions.empty();
	}

	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
		SwapChainSupportDetails details;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

		uint32_t format_ct;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_ct, nullptr);

		if(format_ct!=0) {
			details.formats.resize(format_ct);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_ct, details.formats.data());
		}

		uint32_t present_mode_ct;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_ct, nullptr);

		if(present_mode_ct!=0) {
			details.present_modes.resize(present_mode_ct);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_ct, details.present_modes.data());
		}

		return details;
	}

	bool isDeviceSuitable(VkPhysicalDevice device) {
		QueueFamilyIndices indices=findQueueFamilies(device);

		bool extensions_supported=checkDeviceExtensionSupport(device);

		bool swap_chain_adequate=false;
		if(extensions_supported) {
			SwapChainSupportDetails swapChainSupport=querySwapChainSupport(device);
			swap_chain_adequate=!swapChainSupport.formats.empty()&&!swapChainSupport.present_modes.empty();
		}

		return indices.isComplete()&&extensions_supported&&swap_chain_adequate;
	}

	void pickPhysicalDevice() {
		uint32_t device_ct=0;
		vkEnumeratePhysicalDevices(instance, &device_ct, nullptr);
		if(device_ct==0) {
			throw std::runtime_error("failed to find GPUs with Vulkan support!");
		}

		std::vector<VkPhysicalDevice> devices(device_ct);
		vkEnumeratePhysicalDevices(instance, &device_ct, devices.data());
	
		for(const auto& device:devices) {
			if(isDeviceSuitable(device)) {
				physical_device=device;
				break;
			}
		}

		if(physical_device==VK_NULL_HANDLE) {
			throw std::runtime_error("failed to find a suitable GPU!");
		}
	}

	void createLogicalDevice() {
		QueueFamilyIndices indices=findQueueFamilies(physical_device);

		std::vector<VkDeviceQueueCreateInfo> queue_create_infos{};
		std::set<uint32_t> unique_queue_families={
			indices.graphics_family.value(),
			indices.present_family.value()
		};

		float queue_priority=1.f;
		for(uint32_t queue_family:unique_queue_families) {
			VkDeviceQueueCreateInfo queue_create_info{};
			queue_create_info.sType=VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queue_create_info.queueFamilyIndex=queue_family;
			queue_create_info.queueCount=1;
			queue_create_info.pQueuePriorities=&queue_priority;
			queue_create_infos.push_back(queue_create_info);
		}

		VkPhysicalDeviceFeatures device_features{};

		VkDeviceCreateInfo create_info{};
		create_info.sType=VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		create_info.queueCreateInfoCount=static_cast<uint32_t>(queue_create_infos.size());
		create_info.pQueueCreateInfos=queue_create_infos.data();

		create_info.pEnabledFeatures=&device_features;

		create_info.enabledExtensionCount=static_cast<uint32_t>(device_extensions.size());
		create_info.ppEnabledExtensionNames=device_extensions.data();

		if(enable_validation_layers) {
			create_info.enabledLayerCount=static_cast<uint32_t>(validation_layers.size());
			create_info.ppEnabledLayerNames=validation_layers.data();
		} else {
			create_info.enabledLayerCount=0;
		}

		if(vkCreateDevice(physical_device, &create_info, nullptr, &device)!=VK_SUCCESS) {
			throw std::runtime_error("failed to create logical device!");
		}

		vkGetDeviceQueue(device, indices.graphics_family.value(), 0, &graphics_queue);
		vkGetDeviceQueue(device, indices.present_family.value(), 0, &presentQueue);
	}

	//select bgr8 & srgb if possible
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats) {
		for(const auto& available_format:available_formats) {
			if(available_format.format==VK_FORMAT_B8G8R8A8_SRGB&&available_format.colorSpace==VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return available_format;
			}
		}

		return available_formats[0];
	}

	//select triple buffering if possible
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& available_present_modes) {
		for(const auto& available_present_mode:available_present_modes) {
			if(available_present_mode==VK_PRESENT_MODE_MAILBOX_KHR) {
				return available_present_mode;
			}
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
		if(capabilities.currentExtent.width!=std::numeric_limits<uint32_t>::max()) {
			return capabilities.currentExtent;
		} else {
			int width, height;
			glfwGetFramebufferSize(window, &width, &height);

			VkExtent2D actual_extent={
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height)
			};

			actual_extent.width=std::clamp(actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			actual_extent.height=std::clamp(actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

			return actual_extent;
		}
	}

	void createSwapChain() {
		SwapChainSupportDetails swap_chain_support=querySwapChainSupport(physical_device);

		VkSurfaceFormatKHR surface_format=chooseSwapSurfaceFormat(swap_chain_support.formats);
		VkPresentModeKHR present_mode=chooseSwapPresentMode(swap_chain_support.present_modes);
		VkExtent2D extent=chooseSwapExtent(swap_chain_support.capabilities);
	
		uint32_t image_ct=swap_chain_support.capabilities.minImageCount+1;

		if(swap_chain_support.capabilities.maxImageCount>0&&image_ct>swap_chain_support.capabilities.maxImageCount) {
			image_ct=swap_chain_support.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR create_info{};
		create_info.sType=VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		create_info.surface=surface;

		create_info.minImageCount=image_ct;
		create_info.imageFormat=surface_format.format;
		create_info.imageColorSpace=surface_format.colorSpace;
		create_info.imageExtent=extent;
		create_info.imageArrayLayers=1;
		create_info.imageUsage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		QueueFamilyIndices indices=findQueueFamilies(physical_device);
		uint32_t queue_family_indices[]={indices.graphics_family.value(), indices.present_family.value()};

		if(indices.graphics_family!=indices.present_family) {
			create_info.imageSharingMode=VK_SHARING_MODE_CONCURRENT;
			create_info.queueFamilyIndexCount=2;
			create_info.pQueueFamilyIndices=queue_family_indices;
		} else {
			create_info.imageSharingMode=VK_SHARING_MODE_EXCLUSIVE;
		}

		create_info.preTransform=swap_chain_support.capabilities.currentTransform;

		create_info.compositeAlpha=VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

		create_info.presentMode=present_mode;
		create_info.clipped=VK_TRUE;

		create_info.oldSwapchain=VK_NULL_HANDLE;

		if(vkCreateSwapchainKHR(device, &create_info, nullptr, &swap_chain)!=VK_SUCCESS) {
			throw std::runtime_error("failed to create swap chain!");
		}

		vkGetSwapchainImagesKHR(device, swap_chain, &image_ct, nullptr);
		swap_chain_images.resize(image_ct);
		vkGetSwapchainImagesKHR(device, swap_chain, &image_ct, swap_chain_images.data());

		swap_chain_image_format=surface_format.format;
		swap_chain_extent=extent;
	}

	void initVulkan() {
		createInstance();
		setupDebugMessenger();
		createSurface();
		pickPhysicalDevice();
		createLogicalDevice();
		createSwapChain();
	}

	void initWindow() {
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		window=glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
	}

	void mainLoop() {
		while(!glfwWindowShouldClose(window)) {
			glfwPollEvents();
		}
	}

	void cleanup() {
		vkDestroySwapchainKHR(device, swap_chain, nullptr);
		
		vkDestroyDevice(device, nullptr);
		
		if(enable_validation_layers) {
			DestroyDebugUtilsMessengerEXT(instance, debug_messenger, nullptr);
		}

		vkDestroySurfaceKHR(instance, surface, nullptr);

		vkDestroyInstance(instance, nullptr);

		glfwDestroyWindow(window);

		glfwTerminate();
	}

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT msg_severity, VkDebugUtilsMessageTypeFlagsEXT msg_type, const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data) {
		std::cerr<<"validation layer: "<<callback_data->pMessage<<std::endl;

		return VK_FALSE;
	}
};

int main() {
	HelloTriangleApplication app;

	try {
		app.run();
	} catch(const std::exception& e) {
		std::cerr<<e.what()<<std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}