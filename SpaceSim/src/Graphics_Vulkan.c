#include "Graphics.h"
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan\vulkan.h>
#include <malloc.h>
#include <assert.h>
#include "defines.h"
#include "JobSystem.h"


#define SWAPCHAIN_IMAGE_COUNT 2


static VkInstance g_instance;
static VkRenderPass g_renderPass;
static struct 
{
	VkSwapchainKHR swapchain;
	uint32 currentImage;
	struct
	{
		VkImage image;
		VkImageView view;
		VkFramebuffer framebuffer;
	} backBuffers[SWAPCHAIN_IMAGE_COUNT];
} g_swapchain;

static struct
{
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	VkQueue queue;
	VkCommandPool commandPool;
} g_threadLocal[MAX_NUMBER_THREADS];

VkCommandBuffer g_commandBuffers[SWAPCHAIN_IMAGE_COUNT];


JOB_ENTRY(initThreadLocal)
{
	// poll available physical devices
	uint32 physCount = 0;
	vkEnumeratePhysicalDevices(g_instance, &physCount, NULL);
	VkPhysicalDevice* pPhysicalDevices = alloca(sizeof(VkPhysicalDevice)*physCount);
	vkEnumeratePhysicalDevices(g_instance, &physCount, pPhysicalDevices);

	// get properties
	VkPhysicalDevice physicalDevice = pPhysicalDevices[0];
	for (uint32 i = 0; i < physCount; ++i)
	{
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(pPhysicalDevices[i], &properties);

		uint32 queueCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(pPhysicalDevices[i], &queueCount, NULL);
		VkQueueFamilyProperties* pQueueProperties = alloca(sizeof(VkQueueFamilyProperties) * queueCount);
		vkGetPhysicalDeviceQueueFamilyProperties(pPhysicalDevices[i], &queueCount, pQueueProperties);
	}


	// create device
	float priority = 1.0f;

	VkDeviceQueueCreateInfo queueCreateInfo = {
		VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		NULL, //next
		0, // flags
		0, // queueFamilyIndex
		1, // queueCount
		&priority
	};

	uint32 deviceLayerPropertyCount;
	vkEnumerateDeviceLayerProperties(physicalDevice, &deviceLayerPropertyCount, NULL);
	VkLayerProperties* pDeviceLayerProperties = alloca(sizeof(VkLayerProperties) * deviceLayerPropertyCount);
	vkEnumerateDeviceLayerProperties(physicalDevice, &deviceLayerPropertyCount, pDeviceLayerProperties);

	char** ppDeviceLayerNames = alloca(sizeof(char*) * deviceLayerPropertyCount);
	for (uint32 i = 0; i < deviceLayerPropertyCount; ++i)
	{
		ppDeviceLayerNames[i] = pDeviceLayerProperties[i].layerName;
	}



	VkDeviceCreateInfo deviceCreateInfo = {
		VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		NULL, //next
		0, // flags
		1, // queueCount
		&queueCreateInfo,
		0,//deviceLayerPropertyCount, //layerCount
		ppDeviceLayerNames, //layerNames
		0, //extensionCount
		NULL, //extensionNames
		NULL, //features
	};

	VkDevice device;
	vkCreateDevice(physicalDevice, &deviceCreateInfo, NULL, &device);

	// get main queue
	VkQueue queue;
	vkGetDeviceQueue(device, 0, 0, &queue);


	// create command pool
	VkCommandPoolCreateInfo commandPoolCreateInfo = {
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		NULL, //next
		0, //flags
		0 //family index
	};

	VkCommandPool commandPool;
	vkCreateCommandPool(device, &commandPoolCreateInfo, NULL, &commandPool);


	int thread = jsGetCurrentThread();
	g_threadLocal[thread].physicalDevice = physicalDevice;
	g_threadLocal[thread].device = device;
	g_threadLocal[thread].queue = queue;
	g_threadLocal[thread].commandPool = commandPool;

}



void initVulkan(int width, int height, WindowHandle window)
{
	assert(jsGetCurrentThread() == MAIN_THREAD);


	const char* extensionNames[] = { "VK_KHR_surface", VK_KHR_WIN32_SURFACE_EXTENSION_NAME };
	uint32 instanceLayerPropertyCount;
	vkEnumerateInstanceLayerProperties(&instanceLayerPropertyCount, NULL);
	VkLayerProperties* pInstanceLayerProperties = alloca(sizeof(VkLayerProperties) * instanceLayerPropertyCount);
	vkEnumerateInstanceLayerProperties(&instanceLayerPropertyCount, pInstanceLayerProperties);

	char** ppInstanceLayerNames = alloca(sizeof(char*) * instanceLayerPropertyCount);
	uint32 instanceLayerCount = 0;
	for (uint32 i = 0; i < instanceLayerPropertyCount; ++i)
	{
		if (i != 10)
			ppInstanceLayerNames[instanceLayerCount++] = pInstanceLayerProperties[i].layerName;
	}

	VkApplicationInfo appInfo = {
		VK_STRUCTURE_TYPE_APPLICATION_INFO,
		NULL,
		"SpaceSim",
		0,
		"",
		0,
		1 << 22
	};



	// create instance
	VkInstanceCreateInfo instanceCreateInfo = {
		VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, 
		NULL, //next
		0, // flags
		&appInfo,
		instanceLayerCount, // enabled layers count
		ppInstanceLayerNames, // enabled layers
		ARRAYSIZE(extensionNames),
		extensionNames
	};

	vkCreateInstance(&instanceCreateInfo, NULL, &g_instance);

	// get extension pointers
	PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR)vkGetInstanceProcAddr(g_instance, "vkCreateWin32SurfaceKHR");

	// init side threads
	JobDecl initJob;
	initJob.fpFunction = &initThreadLocal;
	initJob.pName = "graphics_init_thread_local";
	initJob.pParams = NULL;

	Counter* initCounter = NULL;

	for (int i = 0; i < jsNumThreads(); ++i)
	{
		if (i != MAIN_THREAD)
		{
			jsRunJobsInThread(&initJob, 1, &initCounter, i);
		}
	}

	// init main threads
	initThreadLocal(NULL);

	VkPhysicalDevice physicalDevice = g_threadLocal[MAIN_THREAD].physicalDevice;
	VkDevice device = g_threadLocal[MAIN_THREAD].device;
	VkQueue queue = g_threadLocal[MAIN_THREAD].queue;
	VkCommandPool commandPool = g_threadLocal[MAIN_THREAD].commandPool;


	// create surface
	VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {
		VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
		NULL, // next
		0, //flags
		GetModuleHandle(NULL),
		window
	};

	VkSurfaceKHR surface;
	vkCreateWin32SurfaceKHR(g_instance, &surfaceCreateInfo, NULL, &surface);

	


	// create swapchain
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities);

	assert(SWAPCHAIN_IMAGE_COUNT >= surfaceCapabilities.minImageCount && SWAPCHAIN_IMAGE_COUNT <= surfaceCapabilities.maxImageCount);
	
	uint32 formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, NULL);
	// get prefered format
	VkSurfaceFormatKHR* pSurfaceFormats = alloca(sizeof(VkSurfaceFormatKHR) * formatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, pSurfaceFormats);
	VkSurfaceFormatKHR surfaceFormat = pSurfaceFormats[0];

	VkFormat format = VK_FORMAT_B8G8R8A8_UNORM;
	// if no format is preferred choose standard
	if (surfaceFormat.format != VK_FORMAT_UNDEFINED)
		format = surfaceFormat.format;
	


	VkSwapchainCreateInfoKHR swapchainCreateInfo = {
		VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		NULL, // next
		0, //flags
		surface,
		2, // minImageCount
		format,
		surfaceFormat.colorSpace,
		{width, height},
		1, // image array layers
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		VK_SHARING_MODE_EXCLUSIVE,
		0, // family index count
		NULL, // family indeices
		surfaceCapabilities.currentTransform,
		VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		VK_PRESENT_MODE_FIFO_KHR,
		TRUE, // clipped
		VK_NULL_HANDLE
	};


	VkSwapchainKHR swapchain;
	vkCreateSwapchainKHR(device, &swapchainCreateInfo, NULL, &swapchain);
	g_swapchain.swapchain = swapchain;


	// get swap chain image
	uint32 swapchainImageCount;
	vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, NULL);
	
	VkImage* pSwapchainImages = alloca(sizeof(VkImage) * swapchainImageCount);
	vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, pSwapchainImages);

	// create render pass

	VkAttachmentDescription attachmentDescription = {
		0, // flags
		format,
		VK_SAMPLE_COUNT_1_BIT,
		VK_ATTACHMENT_LOAD_OP_CLEAR,
		VK_ATTACHMENT_STORE_OP_STORE,
		VK_ATTACHMENT_LOAD_OP_DONT_CARE, //stencil
		VK_ATTACHMENT_STORE_OP_DONT_CARE,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, //initial layout
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL //final layout
	};

	VkAttachmentReference colorAttachment = {
		0,
		VK_IMAGE_LAYOUT_GENERAL
	};

	VkSubpassDescription subpassDescription = {
		0, //flags
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		0, // input attachment count
		NULL, // input attachments
		1, // color attachment count
		&colorAttachment,
		NULL, // resource attachment
		NULL, // depth stencil attachment
		0, // perserve attachment count
		NULL // preserve attachment
	};

	VkRenderPassCreateInfo renderPassCreateInfo = {
		VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		NULL, //next
		0, // flags
		1, //attachment count
		&attachmentDescription,
		1, //subpath count
		&subpassDescription,
		0, // dependency count
		NULL // dependencies
	};

	VkRenderPass renderPass;
	vkCreateRenderPass(device, &renderPassCreateInfo, NULL, &renderPass);
	g_renderPass = renderPass;

	for (int i = 0; i < SWAPCHAIN_IMAGE_COUNT; ++i)
	{

		VkImageViewCreateInfo backbufferCreateInfo = {
			VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			NULL, // next
			0, // flags
			pSwapchainImages[i],
			VK_IMAGE_VIEW_TYPE_2D,
			format,
			{ VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A },
			{ VK_IMAGE_ASPECT_COLOR_BIT, 0 /*base mip level*/, 1 /*level count*/, 0 /*base array layer*/, 1 /*layer count*/ }
		};

		VkImageView backbufferView;
		vkCreateImageView(device, &backbufferCreateInfo, NULL, &backbufferView);

		// create frame buffer

		VkFramebufferCreateInfo framebufferCreateInfo = {
			VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			NULL, // next
			0, // flags
			renderPass,
			1, // attachment count
			&backbufferView,
			width,
			height,
			1 // layers
		};
		VkFramebuffer framebuffer;
		vkCreateFramebuffer(device, &framebufferCreateInfo, NULL, &framebuffer);


		g_swapchain.backBuffers[i].image = pSwapchainImages[i];
		g_swapchain.backBuffers[i].view = backbufferView;
		g_swapchain.backBuffers[i].framebuffer = framebuffer;
	}



	VkSemaphoreCreateInfo presentCompleteCreateInfo = {
		VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		NULL,
		0
	};



	for (int i = 0; i < SWAPCHAIN_IMAGE_COUNT; ++i)
	{

		// create command bufferS

		VkCommandBufferAllocateInfo commandBufferAllocateInfo = {
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			NULL, // next
			commandPool,
			VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			1 // command buffer count
		};

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &commandBuffer);
		g_commandBuffers[i] = commandBuffer;

		//begin recording
		VkCommandBufferBeginInfo commandBufferBeginInfo = {
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			NULL, //next
			0, //flags
			NULL
		};

		vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);

		// begin render pass
		VkClearValue clearValue = { 0 };
		clearValue.color.float32[i] = 1.0f;
		clearValue.color.float32[3] = 1.0f;

		VkRenderPassBeginInfo renderpassBeginInfo = {
			VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			NULL, // next
			renderPass,
			g_swapchain.backBuffers[i].framebuffer,
			{{0,0},{width,height}}, //render area
			1, // clear value count
			&clearValue
		};

		vkCmdBeginRenderPass(commandBuffer, &renderpassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		// set viewport
		VkViewport viewPort = {
			0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f
		};

		vkCmdSetViewport(commandBuffer, 0, 1, &viewPort);

		// end renderpass
		vkCmdEndRenderPass(commandBuffer);


		//end recording
		vkEndCommandBuffer(commandBuffer);
	}
	
	jsWaitForCounter(initCounter);

}

void initGraphics(int width, int height, WindowHandle window)
{
	initVulkan(width, height, window);
}

void render(double dt)
{
	assert(jsGetCurrentThread() == MAIN_THREAD);

	// submit command buffer

	vkAcquireNextImageKHR(g_threadLocal[MAIN_THREAD].device, g_swapchain.swapchain, UINT64_MAX, VK_NULL_HANDLE, VK_NULL_HANDLE, &g_swapchain.currentImage);

	VkSubmitInfo submitInfo = {
		VK_STRUCTURE_TYPE_SUBMIT_INFO,
		NULL, //next
		0, // semaphoreCount
		NULL, // semaphores
		NULL, // pipelineStages
		1, // commandBufferCount
		&g_commandBuffers[g_swapchain.currentImage],
		0, // signalCount
		NULL // signals
	};

	vkQueueSubmit(g_threadLocal[MAIN_THREAD].queue, 1, &submitInfo, VK_NULL_HANDLE);
}


void present(WindowHandle window)
{
	assert(jsGetCurrentThread() == MAIN_THREAD);

	VkPresentInfoKHR presentInfo = {
		VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		NULL, //next
		0, // wait semaphore count
		NULL, // wait semaphore
		1, // swapchain count
		&g_swapchain.swapchain,
		&g_swapchain.currentImage
	};

	vkQueuePresentKHR(g_threadLocal[MAIN_THREAD].queue, &presentInfo);
}