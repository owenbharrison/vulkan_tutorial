#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>

#include <vector>
#include <cstring>

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

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
	auto func=(PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if(func!=nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	} else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
	auto func=(PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if(func!=nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}

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

	void initVulkan() {
		createInstance();
		setupDebugMessenger();
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
		if(enable_validation_layers) {
			DestroyDebugUtilsMessengerEXT(instance, debug_messenger, nullptr);
		}

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