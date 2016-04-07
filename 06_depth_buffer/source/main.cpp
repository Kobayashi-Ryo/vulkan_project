//*****************************************************************************
//
// �G���g���[�|�C���g
// Author : Ryo Kobayashi
//
// ��ʂ��N���A����
//
//*****************************************************************************

//*****************************************************************************
// �C���N���[�h�t�@�C��
//*****************************************************************************
#include "main.h"

#include <string>
#include <thread>

#include <vulkan/vulkan.h>

//*****************************************************************************
// �}�N����`
//*****************************************************************************
#define UNUSED(x) x
#define WND_MESSAGE_CONTINUE 1
#define WND_MESSAGE_END 0
#define NUM_SAMPLES VK_SAMPLE_COUNT_1_BIT

//*****************************************************************************
// �萔��`
//*****************************************************************************
const int32_t GAME_WINDOW = (WS_OVERLAPPEDWINDOW ^ (WS_THICKFRAME | WS_MAXIMIZEBOX));
const uint32_t MAX_GPU_COUNT = 4;
const uint32_t MAX_QUEUE_FAMILY_COUNT = 4;
const uint32_t MAX_COMMAND_BUFFER_COUNT = 2;
const uint32_t FENCE_TIMEOUT = 1000000;

//*****************************************************************************
// �v���g�^�C�v��`
//*****************************************************************************
void initWindow();
void initVulkan();
void initVulkanInstance();
void initVulkanPhysicalDevices();
void initVulkanLogicalDevice();
void initVulkanGraphicsQueue();
void initVulkanDrawFence();
void initVulkanCommandPool();
void initVulkanSwapChain();
void initVulkanBackBuffers();
void initVulkanDepthStencilBuffer();

void beginVulkanCommandBuffer(uint32_t idx);
void endVulkanCommandBuffer(uint32_t idx);
void queueVulkanCommandBuffer();

void cmdSetImageMemoryBarrier(VkCommandBuffer cmd, VkImage img, VkImageAspectFlags aspectMask, VkImageLayout oldImageLayout, VkImageLayout newImageLayout);

void procWindowThread();
int32_t procWindowMessage(MSG& msg);

void uninitWindow();
void uninitVulkan();

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void KeyInputProc(HWND hWnd, WPARAM wParam);

//*****************************************************************************
// �O���[�o���ϐ�
//*****************************************************************************
// �E�B���h�E
HWND g_hWnd = nullptr;
uint32_t g_uWindowWidth = 960;
uint32_t g_uWindowHeight = 540;
std::string g_strClassName = "kb_vulkan_project";
std::string g_strCaption = "kbVulkan Demo";
volatile bool g_bWindowInitialized = false;
std::thread g_threadWindow;

// Vulkan
// 00
VkInstance g_vkInstance = nullptr;
// 01
uint32_t g_uPhysicalDeviceCount = 0;
VkPhysicalDevice g_vkPhysicalDevices[MAX_GPU_COUNT];
uint32_t g_uPhysDevQueueFamilyPropertyCount = 0;
VkQueueFamilyProperties g_vkQueueFamilyProps[MAX_QUEUE_FAMILY_COUNT];
VkPhysicalDeviceMemoryProperties g_vkPhysDevMemoryProps;
VkPhysicalDeviceProperties g_vkPhysDevProps;
// 02
VkDevice g_vkDevice = nullptr;
// 03
VkCommandPool g_vkCmdPool = 0L;
VkCommandBuffer g_vkCmdBufs[MAX_COMMAND_BUFFER_COUNT];
// 04
VkSurfaceKHR g_vkSurface = 0L;
uint32_t g_uGraphicsQueueFamilyIndex = UINT32_MAX;
VkSwapchainKHR g_vkSwapchain = 0L;
uint32_t g_uSwapchainImageCount = 0;
VkFormat g_vkSwapChainImageFormat;
VkQueue g_vkGraphicsQueue = nullptr;
uint32_t g_uGraphicsQueueIndex = 0;
VkFence g_vkDrawFence = 0L;
// 05
VkImage g_vkSwapChainImages[2];
VkImageView g_vkSwapChainImageViews[2];
// 06
VkFormat g_vkDepthFormat;
VkImage g_vkDepthStencilBuffer;
VkImageView g_vkDepthStencilBufferView;

// �A�v���P�[�V����
bool g_bIsAppRunning = true;

int32_t WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
{
  UNUSED(hInstance);
  UNUSED(hPrevInstance);
  UNUSED(pCmdLine);
  UNUSED(nCmdShow);

  g_threadWindow = std::thread(procWindowThread);
  initVulkan();

  while(g_bIsAppRunning)
  {
    beginVulkanCommandBuffer(0);
    endVulkanCommandBuffer(0);
    queueVulkanCommandBuffer();
    Sleep(16);
  }

  uninitVulkan();

  g_threadWindow.join();

  return 0;
}

void initWindow()
{
  HINSTANCE hInstance = GetModuleHandle(nullptr);

  RECT rc;
  // �E�B���h�E��`�͈͐ݒ�
  rc.top = 0;
  rc.left = 0;
  rc.right = g_uWindowWidth;
  rc.bottom = g_uWindowHeight;

  // �E�B���h�E�N���X������
  WNDCLASSEX wcex =
  {
    sizeof(WNDCLASSEX),
    CS_CLASSDC,
    WndProc,  // �E�B���h�E�v���V�[�W���֐�
    0,
    0,
    hInstance,
    NULL,
    LoadCursor(NULL, IDC_ARROW),
    (HBRUSH)(COLOR_WINDOW + 1),
    NULL,
    g_strClassName.c_str(),
    NULL
  };

  // �E�B���h�E�N���X�o�^
  if(FAILED(RegisterClassEx(&wcex)))
  {
    MessageBox(NULL, "�E�B���h�E�N���X�o�^���s", "�G���[", MB_OK);
    return;
  }

  // �E�B���h�E�����킹
  AdjustWindowRect(
    &rc,    // �E�B���h�E��
    GAME_WINDOW, // �E�B���h�E�X�^�C��
    FALSE);    // ���j���[�̗L��

               // �E�B���h�E���W�ݒ�
               // �^�X�N�o�[���������f�B�X�v���C���擾
  int32_t nDisplayWidth = GetSystemMetrics(SM_CXFULLSCREEN);
  int32_t nDisplayHeight = GetSystemMetrics(SM_CYFULLSCREEN);

  // �E�B���h�E�쐬
  int32_t nWindowWidth = rc.right - rc.left;  // �E�B���h�E����
  int32_t nWindowHeight = rc.bottom - rc.top;  // �E�B���h�E�c��
  int32_t nWindowPosX = (nDisplayWidth >> 1) - (nWindowWidth >> 1);  // �E�B���h�E����X���W
  int32_t nWindowPosY = (nDisplayHeight >> 1) - (nWindowHeight >> 1);  // �E�B���h�E����Y���W

  // �E�B���h�E�̍쐬
  g_hWnd = CreateWindowEx(0,
    g_strClassName.c_str(),
    g_strCaption.c_str(),
    // �Ϙg�A�ő剻�{�^������菜��
    GAME_WINDOW,
    nWindowPosX,
    nWindowPosY,
    nWindowWidth,
    nWindowHeight,
    NULL,
    NULL,
    hInstance,
    NULL);

  // �쐬���s�\��
  if(!g_hWnd)
  {
    MessageBox(NULL, "�E�B���h�E�쐬���s", "�G���[", MB_OK);
    return;
  }

  g_bWindowInitialized = true;
}

void initVulkan()
{
  while(!g_bWindowInitialized)
  {
    Sleep(1);
  }

  initVulkanInstance();
  initVulkanPhysicalDevices();
  initVulkanLogicalDevice();
  initVulkanDrawFence();
  initVulkanCommandPool();
  initVulkanSwapChain();
  initVulkanGraphicsQueue();
  initVulkanBackBuffers();
  initVulkanDepthStencilBuffer();
}

void initVulkanInstance()
{
  VkResult result;

  // �A�v���P�[�V�������
  const VkApplicationInfo infoApp =
  {
    VK_STRUCTURE_TYPE_APPLICATION_INFO, // type
    nullptr,                            // next
    g_strCaption.c_str(),               // app name
    0,                                  // app version
    g_strCaption.c_str(),               // engine name
    0,                                  // engine version
    VK_API_VERSION,                     // api version
  };
  // �C���X�^���X�������
  uint32_t ulayerCount = 0;
  const char* const * ppLayerNames = nullptr;
  uint32_t uExtensionCount = 0;
  const char* const * ppExtensionNames = nullptr;
  VkInstanceCreateInfo infoInstance =
  {
    VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, // type
    nullptr,                                // next
    0,                                      // flags
    &infoApp,                               // app info
    ulayerCount,                            // layer count
    ppLayerNames,                           // layer names
    uExtensionCount,                        // extension count
    ppExtensionNames                        // extension names
  };

  // �C���X�^���X����
  result = vkCreateInstance(&infoInstance,
    nullptr,
    &g_vkInstance);

  if(result != VK_SUCCESS)
  {
    MessageBox(g_hWnd, "instance error", "", MB_OK);
  }
}

void initVulkanPhysicalDevices()
{
  VkResult result = VK_SUCCESS;

  // ����API�Ŏg�p�\��GPU�̐����擾
  result = vkEnumeratePhysicalDevices(
    g_vkInstance,
    &g_uPhysicalDeviceCount,
    nullptr);

  if(g_uPhysicalDeviceCount <= 0)
  {
    MessageBox(g_hWnd, "compatible gpu not found", "", MB_OK);
  }

  // GPU�I�u�W�F�N�g�擾
  result = vkEnumeratePhysicalDevices(
    g_vkInstance,
    &g_uPhysicalDeviceCount,
    g_vkPhysicalDevices);

  // QueueFamily�����擾
  vkGetPhysicalDeviceQueueFamilyProperties(
    g_vkPhysicalDevices[0],
    &g_uPhysDevQueueFamilyPropertyCount,
    nullptr);

  if(g_uPhysDevQueueFamilyPropertyCount <= 0)
  {
    MessageBox(g_hWnd, "queue family props not found", "", MB_OK);
  }

  // QueueFamily�I�u�W�F�N�g���擾
  vkGetPhysicalDeviceQueueFamilyProperties(
    g_vkPhysicalDevices[0],
    &g_uPhysDevQueueFamilyPropertyCount,
    g_vkQueueFamilyProps);

  // GPU�������[�v���p�e�B�擾
  vkGetPhysicalDeviceMemoryProperties(
    g_vkPhysicalDevices[0],
    &g_vkPhysDevMemoryProps);
  // GPU���擾
  vkGetPhysicalDeviceProperties(
    g_vkPhysicalDevices[0],
    &g_vkPhysDevProps);
}

void initVulkanLogicalDevice()
{
  // �Ȃ���queue��16��葽������...
  // queue��priority��0.0���Œ��1.0���ō�
  const float_t afQueuePriorities[] = {0.0f, 0.1f, 0.2f, 0.3f};
  VkDeviceQueueCreateInfo infoDevQueueCreate =
  {
    VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, // type
    nullptr,                                    // next
    0,                                          // flags
    0,                                          // queue family index
    4,                                          // queue count
    afQueuePriorities                           // queue priorities
  };

  VkDeviceCreateInfo infoDevCreate =
  {
    VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, // type
    nullptr,                              // next
    0,                                    // flags
    1,                                    // queue create count
    &infoDevQueueCreate,                  // device queue create info
    0,                                    // enabled layer count
    nullptr,                              // enabled layer names
    0,                                    // enabled extension count
    nullptr,                              // enabled extension names
    nullptr                               // enabled physical device features
  };

  // �f�o�C�X����
  VkResult result;
  result = vkCreateDevice(
    g_vkPhysicalDevices[0],
    &infoDevCreate,
    nullptr,
    &g_vkDevice);
}

void initVulkanDrawFence()
{
  VkFenceCreateInfo infoFence =
  {
    VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,  // type
    nullptr,                              // next
    0,                                    // create flag
  };
  VkResult result = vkCreateFence(
    g_vkDevice,
    &infoFence,
    nullptr,
    &g_vkDrawFence);
  if(result != VK_SUCCESS)
  {
    // �G���[
    MessageBox(g_hWnd, "cannot create fence", "", MB_OK);
    exit(-1);
  }
}

void initVulkanCommandPool()
{
  if(g_uPhysDevQueueFamilyPropertyCount <= 0)
  {
    // �G���[
    MessageBox(g_hWnd, "queue family not found", "", MB_OK);
    exit(-1);
  }

  // �R�}���h�o�b�t�@�v�[���쐬
  uint32_t uQueueFamilyIndex = 0;
  VkCommandPoolCreateInfo infoCmdPool =
  {
    VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,       // type
    nullptr,                                          // next
    VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,  // flags ���̂ӂ炮�悭�킩���B���̂����Ȃ肹���Ă�����悤�ɂ����H
    uQueueFamilyIndex,                                // queue family index
  };
  VkResult result =
    vkCreateCommandPool(
      g_vkDevice,
      &infoCmdPool,
      nullptr,
      &g_vkCmdPool);
  if(result != VK_SUCCESS)
  {
    // �G���[
    MessageBox(g_hWnd, "cannot create command pool", "", MB_OK);
    exit(-1);
  }

  // �R�}���h�o�b�t�@�m��
  VkCommandBufferAllocateInfo infoCmdBufAlloc =
  {
    VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, // type
    nullptr,                                        // next
    g_vkCmdPool,                                    // command pool
    VK_COMMAND_BUFFER_LEVEL_PRIMARY,                // command buffer level
    MAX_COMMAND_BUFFER_COUNT,                       // command buffer count
  };
  result = vkAllocateCommandBuffers(
    g_vkDevice,
    &infoCmdBufAlloc,
    g_vkCmdBufs);
  if(result != VK_SUCCESS)
  {
    // �G���[
    MessageBox(g_hWnd, "cannot allocate command buffers", "", MB_OK);
    exit(-1);
  }
}

void initVulkanSwapChain()
{
  VkResult result = VK_INCOMPLETE;
#ifdef WIN32
  // Win32�ˑ�
  VkWin32SurfaceCreateInfoKHR infoSurfaceCreate =
  {
    VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,  // type
    nullptr,                                          // next
    0,                                                // flags
    GetModuleHandle(nullptr),                         // win instance
    g_hWnd,                                           // window handle
  };
  result = vkCreateWin32SurfaceKHR(
    g_vkInstance,
    &infoSurfaceCreate,
    nullptr,
    &g_vkSurface);
#endif
  if(result != VK_SUCCESS)
  {
    // �G���[
    MessageBox(g_hWnd, "cannot create surface", "", MB_OK);
    exit(-1);
  }

  // present���T�|�[�g���Ă��邩�ǂ�������(���ʂ͂��Ă�Ǝv����)
  VkBool32* pSupportPresent = new VkBool32[g_vkQueueFamilyProps[0].queueCount];
  for(uint32_t i = 0; i < g_uPhysDevQueueFamilyPropertyCount; i++)
  {
    vkGetPhysicalDeviceSurfaceSupportKHR(
      g_vkPhysicalDevices[0],
      i,
      g_vkSurface,
      &pSupportPresent[i]);
  }
  // graphics���T�|�[�g���Ă���queue������(���ʂ͂��Ă�Ǝv����)
  // �ŏ��Ɍ���������ł�����
  uint32_t uGraphicsQueueNodeIndex = UINT32_MAX;
  for(uint32_t i = 0; i < g_uPhysDevQueueFamilyPropertyCount; i++)
  {
    bool bIsValidSurface =
      ((g_vkQueueFamilyProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
        pSupportPresent[i]);
    if(bIsValidSurface)
    {
      uGraphicsQueueNodeIndex = i;
      break;
    }
  }
  delete[] pSupportPresent;

  if(uGraphicsQueueNodeIndex == UINT32_MAX)
  {
    // �G���[
    MessageBox(g_hWnd, "queue supports graphics and present not found", "", MB_OK);
    exit(-1);
  }

  g_uGraphicsQueueFamilyIndex = uGraphicsQueueNodeIndex;

  // �T�|�[�g����Ă���surface��format�ꗗ���擾
  // �܂��͌�
  uint32_t uFormatCount = 0;
  result = vkGetPhysicalDeviceSurfaceFormatsKHR(
    g_vkPhysicalDevices[0],
    g_vkSurface,
    &uFormatCount,
    nullptr);
  if(result != VK_SUCCESS)
  {
    // �G���[
    MessageBox(g_hWnd, "cannot get surface format count", "", MB_OK);
    exit(-1);
  }
  // �t�H�[�}�b�g�ꗗ
  VkSurfaceFormatKHR* pSurfFormats = new VkSurfaceFormatKHR[uFormatCount];
  result = vkGetPhysicalDeviceSurfaceFormatsKHR(
    g_vkPhysicalDevices[0],
    g_vkSurface,
    &uFormatCount,
    pSurfFormats);
  if(result != VK_SUCCESS)
  {
    // �G���[
    MessageBox(g_hWnd, "cannot get surface formats", "", MB_OK);
    exit(-1);
  }
  // �t�H�[�}�b�g���擾
  g_vkSwapChainImageFormat = pSurfFormats[0].format;

  // �T�[�t�F�X�̐ݒ�\�ȃT�C�Y���擾
  VkSurfaceCapabilitiesKHR surfCapabilities;
  result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
    g_vkPhysicalDevices[0], g_vkSurface, &surfCapabilities);
  if(result != VK_SUCCESS)
  {
    // �G���[
    MessageBox(g_hWnd, "cannot get surface capabilities", "", MB_OK);
    exit(-1);
  }
  uint32_t uPresentModeCount;
  result = vkGetPhysicalDeviceSurfacePresentModesKHR(
    g_vkPhysicalDevices[0], g_vkSurface, &uPresentModeCount, nullptr);
  if(result != VK_SUCCESS)
  {
    // �G���[
    MessageBox(g_hWnd, "cannot get present mode count", "", MB_OK);
    exit(-1);
  }
  VkPresentModeKHR* presentModes = new VkPresentModeKHR[uPresentModeCount];
  result = vkGetPhysicalDeviceSurfacePresentModesKHR(
    g_vkPhysicalDevices[0], g_vkSurface, &uPresentModeCount, presentModes);
  if(result != VK_SUCCESS)
  {
    // �G���[
    MessageBox(g_hWnd, "cannot get present modes", "", MB_OK);
    exit(-1);
  }
  // �T�C�Y����`����Ă���Ȃ�capabilities�̃T�[�t�F�X�T�C�Y��
  // -1�͗��Ȃ��͂�
  VkExtent2D swapChainExtent; // �P�Ȃ��`�T�C�Y
  if(surfCapabilities.currentExtent.width == (uint32_t)-1)
  {
    // ��`����ĂȂ�
    swapChainExtent.width = g_uWindowWidth;
    swapChainExtent.height = g_uWindowHeight;
  }
  else
  {
    // �T�C�Y����`����Ă���
    swapChainExtent = surfCapabilities.currentExtent;
  }

  // �v���[���g���[�h�����肷��
  // MAILBOX���[�h : �҂����Ԃ��Z��&�e�B�A�����O���Ȃ�
  // IMMEDIATE���[�h : ��ԑ���&�e�B�A�����O������
  // FIFO���[�h : 
  // V-Sync���[�h���Ǝv���Ă�������
  VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
  for(uint32_t i = 0; i < uPresentModeCount; i++)
  {
    if(presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
    {
      swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
      break;
    }
    else if(presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)
    {
      swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    }
  }

  // SwapChain�Ŏg�p����image�I�u�W�F�N�g�̌�������
  uint32_t uDesiredNumberOfSwapChainImages =
    surfCapabilities.minImageCount + 1;
  if(surfCapabilities.maxImageCount > 0 &&
    uDesiredNumberOfSwapChainImages > surfCapabilities.maxImageCount)
  {
    // SwapChain���������Ă͂����Ȃ��̂ōő�l�ɐ�������
    uDesiredNumberOfSwapChainImages = surfCapabilities.maxImageCount;
  }

  // ���̃t���O�͂܂�reference�y�[�W�ł��L�q����Ă��Ȃ�
  VkSurfaceTransformFlagBitsKHR preTransform;
  if(surfCapabilities.supportedTransforms &
    VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
  {
    preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
  }
  else
  {
    preTransform = surfCapabilities.currentTransform;
  }

  VkSwapchainCreateInfoKHR infoSwapChain =
  {
    VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,  // type
    nullptr,                                      // next
    0,                                            // flags
    g_vkSurface,                                  // surface
    uDesiredNumberOfSwapChainImages,              // swapchain count
    g_vkSwapChainImageFormat,                         // image format
    VK_COLORSPACE_SRGB_NONLINEAR_KHR,             // color space
    swapChainExtent,                              // image extent (�����c��)
    1,                                            // image array layers
    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,          // image usage flag (Framebuffer���ۂ��Ȃ�)
    VK_SHARING_MODE_EXCLUSIVE,                    // image sharing mode (queue family�ԁH)
    1,                                            // queue family index count
    &g_uGraphicsQueueFamilyIndex,                 // queue family indices
    preTransform,                                 // �摜��ϊ����� (3DS�݂����Ȃ���ȁH)
    VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,            // �����A���t�@�u�����h���Ȃ��Ƃ��H
    swapchainPresentMode,                         // present mode
    true,                                         // clipping
    0L,                                           // old swapchain
  };

  // �ӂ����c����Ƃ����X���b�v�`�F�C���쐬�����c
  result = vkCreateSwapchainKHR(
    g_vkDevice,
    &infoSwapChain,
    nullptr,
    &g_vkSwapchain);
  if(result != VK_SUCCESS)
  {
    // �G���[
    MessageBox(g_hWnd, "cannot create swapchain", "", MB_OK);
    exit(-1);
  }

  delete [] presentModes;
  delete [] pSurfFormats;
}

void initVulkanGraphicsQueue()
{
  vkGetDeviceQueue(
    g_vkDevice,
    g_uGraphicsQueueFamilyIndex,
    g_uGraphicsQueueIndex,
    &g_vkGraphicsQueue);
}

void initVulkanBackBuffers()
{
  VkResult result;

  // �o�b�N�o�b�t�@���쐬����
  // �X���b�v�`�F�C���Ɏg�p���Ă���摜���擾
  result = vkGetSwapchainImagesKHR(
    g_vkDevice,
    g_vkSwapchain,
    &g_uSwapchainImageCount,
    nullptr);
  if(result != VK_SUCCESS)
  {
    // �G���[
    MessageBox(g_hWnd, "cannot get swapchain image count", "", MB_OK);
    exit(-1);
  }
  else if(2 != g_uSwapchainImageCount)
  {
    // �G���[
    MessageBox(g_hWnd, "unexpected swapchain image count", "", MB_OK);
    exit(-1);
  }

  result = vkGetSwapchainImagesKHR(
    g_vkDevice,
    g_vkSwapchain,
    &g_uSwapchainImageCount,
    g_vkSwapChainImages);
  if(result != VK_SUCCESS)
  {
    // �G���[
    MessageBox(g_hWnd, "cannot get swapchain images", "", MB_OK);
    exit(-1);
  }

  VkImageViewCreateInfo arrInfo[2];

  // �r���[���쐬
  for(uint32_t i = 0; i < g_uSwapchainImageCount; i++)
  {
    VkComponentMapping components;
    components.r = VK_COMPONENT_SWIZZLE_R;
    components.g = VK_COMPONENT_SWIZZLE_G;
    components.b = VK_COMPONENT_SWIZZLE_B;
    components.a = VK_COMPONENT_SWIZZLE_A;
    VkImageSubresourceRange subresourceRange;
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = 1;
    subresourceRange.baseArrayLayer = 0;
    subresourceRange.layerCount = 1;
    VkImageViewCreateInfo infoIv =
    {
      VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, // type
      nullptr,                                  // next
      0,                                        // flags
      g_vkSwapChainImages[i],                       // image
      VK_IMAGE_VIEW_TYPE_2D,                    // image view type
      g_vkSwapChainImageFormat,                     // image format
      components,                               // color components
      subresourceRange,                         // subresource range
    };
    arrInfo[i] = infoIv;
  }

  beginVulkanCommandBuffer(0);
  for(uint32_t i = 0; i < g_uSwapchainImageCount; i++)
  {
    // Memorry barrier
    cmdSetImageMemoryBarrier(
      g_vkCmdBufs[0],
      g_vkSwapChainImages[i],
      arrInfo[i].subresourceRange.aspectMask, // �F���ł��邱�Ƃɂ͕ς��Ȃ�
      VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    // create view
    result = vkCreateImageView(
      g_vkDevice,
      &arrInfo[i],
      nullptr,
      &g_vkSwapChainImageViews[i]);
  }
  endVulkanCommandBuffer(0);
  queueVulkanCommandBuffer();
}

void initVulkanDepthStencilBuffer()
{
  VkImageCreateInfo infoImg = {};
  // �[�x�t�H�[�}�b�g
  const VkFormat formatDepth = VK_FORMAT_D24_UNORM_S8_UINT;
  // �[�x�t�H�[�}�b�g���g�p�\�����ׂ�
  // (�����g���Ȃ�GPU�Ȃ�Ă���񂾂낤��)
  VkFormatProperties formatProps;
  vkGetPhysicalDeviceFormatProperties(
    g_vkPhysicalDevices[0],
    formatDepth,
    &formatProps);
  VkImageTiling imgTiling = VK_IMAGE_TILING_OPTIMAL;
  if(formatProps.linearTilingFeatures &
    VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
  {
    infoImg.tiling = VK_IMAGE_TILING_LINEAR;
  }
  else if(formatProps.optimalTilingFeatures &
    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
  {
    infoImg.tiling = VK_IMAGE_TILING_OPTIMAL;
  }
  else
  {
    // �G���[
    MessageBox(g_hWnd, "D24_S8 format is not supported", "", MB_OK);
    exit(-1);
  }

  infoImg.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  infoImg.pNext = nullptr;
  infoImg.flags = 0;
  infoImg.imageType = VK_IMAGE_TYPE_2D;
  infoImg.format = formatDepth;
  infoImg.extent.width = g_uWindowWidth;
  infoImg.extent.height = g_uWindowHeight;
  infoImg.extent.depth = 1;
  infoImg.mipLevels = 1;
  infoImg.arrayLayers = 1;
  infoImg.samples = NUM_SAMPLES;// image, renderpass, pipelinelayout�ł������Ƃ��킹��
  infoImg.tiling = imgTiling;
  infoImg.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  infoImg.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  infoImg.queueFamilyIndexCount = 0;
  infoImg.pQueueFamilyIndices = nullptr;
  infoImg.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

  // �摜�̃o�b�t�@�̈���m��
  VkMemoryAllocateInfo infoMemAlloc = {};
  infoMemAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  infoMemAlloc.pNext = nullptr;
  infoMemAlloc.allocationSize = 0;
  infoMemAlloc.memoryTypeIndex = 0;

  // view���쐬
  VkImageViewCreateInfo infoIv = {};
  infoIv.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  infoIv.pNext = nullptr;
  infoIv.flags = 0;
  infoIv.image = VK_NULL_HANDLE;
  infoIv.viewType = VK_IMAGE_VIEW_TYPE_2D;
  infoIv.format = formatDepth;
  infoIv.components.r = VK_COMPONENT_SWIZZLE_R;
  infoIv.components.g = VK_COMPONENT_SWIZZLE_G;
  infoIv.components.b = VK_COMPONENT_SWIZZLE_B;
  infoIv.components.a = VK_COMPONENT_SWIZZLE_A;
  infoIv.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
  infoIv.subresourceRange.baseMipLevel = 0;
  infoIv.subresourceRange.levelCount = 1;
  infoIv.subresourceRange.baseArrayLayer = 0;
  infoIv.subresourceRange.layerCount = 1;


}

void beginVulkanCommandBuffer(uint32_t idx)
{
  VkCommandBufferBeginInfo infoCmdBufBegin =
  {
    VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,  // type
    nullptr,                                      // next
    0,                                            // flags
    nullptr,                                      // inheritance info
  };

  VkResult result = vkBeginCommandBuffer(
    g_vkCmdBufs[idx],
    &infoCmdBufBegin);
  if(result != VK_SUCCESS)
  {
    // �G���[
    MessageBox(g_hWnd, "cannot begin command buffer", "", MB_OK);
    exit(-1);
  }
}

void endVulkanCommandBuffer(uint32_t idx)
{
  VkResult result = vkEndCommandBuffer(g_vkCmdBufs[idx]);
  if(result != VK_SUCCESS)
  {
    // �G���[
    MessageBox(g_hWnd, "cannot end command buffer", "", MB_OK);
    exit(-1);
  }
}

void queueVulkanCommandBuffer()
{
  // queue�Ƀo�b�t�@�𑗏o(����Ƃ��c)
  VkPipelineStageFlags pipelineStageFlags =
    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT; // ���̃t���O�ɂ���ƁA�O���t�B�b�N�X�p�C�v���C���̃P�c�ɂ���݂���
  VkSubmitInfo infoSubmit[1] =
  {
    VK_STRUCTURE_TYPE_SUBMIT_INFO,  // type
    nullptr,                        // next
    0,                              // wait semaphore count
    nullptr,                        // wait semaphores (objects)
    &pipelineStageFlags,            // ���̃t���O���ݒ肳�ꂽ�p�C�v���C���X�e�[�W�Ɠ������Ƃ�
    1,                              // command buffer count
    g_vkCmdBufs,                    // command buffers
    0,                              // signal semaphore count
    nullptr,                        // signal semaphores
  };

  VkResult result = vkQueueSubmit(
    g_vkGraphicsQueue,
    1,
    infoSubmit,
    g_vkDrawFence);
  if(result != VK_SUCCESS)
  {
    // �G���[
    MessageBox(g_hWnd, "cannot submit command buffer to queue", "", MB_OK);
    exit(-1);
  }

  while((result = vkWaitForFences(
    g_vkDevice, 1, &g_vkDrawFence, VK_TRUE, FENCE_TIMEOUT)) == VK_TIMEOUT);

  if(result != VK_SUCCESS)
  {
    // �G���[
    MessageBox(g_hWnd, "wait fence error", "", MB_OK);
    exit(-1);
  }
}

void cmdSetImageMemoryBarrier(VkCommandBuffer cmd, VkImage img, VkImageAspectFlags aspectMask, VkImageLayout oldImageLayout, VkImageLayout newImageLayout)
{
  VkImageSubresourceRange subresourceRange =
  {
    aspectMask, // aspect mask
    0,          // base mip level
    1,          // level count
    0,          // base array layer
    1           // layer count
  };
  VkImageMemoryBarrier barrierImage =
  {
    VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, // type
    nullptr,                                // next
    0,                                      // src access mask
    0,                                      // dst access mask
    oldImageLayout,                         // old image layout
    newImageLayout,                         // new image layout
    0,                                      // src queue family index
    0,                                      // dst queue family index
    img,                                    // �A�N�Z�X�I�[�_�[��ς������Ȃ��摜�ւ̎Q��
    subresourceRange                        // subresource range
  };

  // �A�N�Z�X�t���O��ݒ�
  // ���̂ւ��Framebuffer���e�N�X�`���Ŏg����Ƃ��Ƃ���
  // GPU�ŏ������ޑO�ɁA�e�N�X�`���Ŏg���ȁ`
  // ���Ċ����̖���
  // optimal : �œK(�œK���ɂ��Ă̐ݒ肩�H)
  switch(oldImageLayout)
  {
  case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
  {
    barrierImage.srcAccessMask |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  }
  break;
  }

  switch(newImageLayout)
  {
  case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
  {
    barrierImage.dstAccessMask |= VK_ACCESS_MEMORY_READ_BIT;
  }
  break;

  case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
  {
    barrierImage.srcAccessMask =
      VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
    barrierImage.dstAccessMask =
      VK_ACCESS_SHADER_READ_BIT;
  }
  break;

  case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
  {
    barrierImage.dstAccessMask =
      VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
  }
  break;

  case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
  {
    barrierImage.dstAccessMask =
      VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
  }
  break;

  }

  // �Ƃ肠�����p�C�v���C���̍ŏ��őS���ς܂��悤�ɂ��Ă��܂�
  VkPipelineStageFlags srcStages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  VkPipelineStageFlags dstStages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

  vkCmdPipelineBarrier(
    cmd,
    srcStages, dstStages,
    0,  // dependency flags
    0, nullptr, // �ėp�������o���A
    0, nullptr, // �o�b�t�@�������o���A
    1, &barrierImage);
}

void procWindowThread()
{
  initWindow();
  
  // �E�B���h�E�\��
  ShowWindow(g_hWnd, SW_SHOW);
  UpdateWindow(g_hWnd);

  MSG msg;
  msg.message = WM_NULL;
  volatile bool bLoop = true;
  while(bLoop)
  {
    // �I��
    if(WND_MESSAGE_END == procWindowMessage(msg))
    {
      g_bIsAppRunning = false;
      bLoop = false;
      break;
    }
    Sleep(16);
  }

  uninitWindow();
}

int32_t procWindowMessage(MSG& msg)
{
  // ���b�Z�[�W������ꍇ����
  int32_t message = WND_MESSAGE_CONTINUE;
  BOOL bIsMessageExist = PeekMessage(&msg, 0, 0, 0, PM_REMOVE);
  if(TRUE == bIsMessageExist)
  {
    // �I�����b�Z�[�W������ꂽ�ꍇ
    if(msg.message == WM_QUIT)
    {
      message = WND_MESSAGE_END;
    }
    // �����Ă��Ȃ��ꍇ
    else
    {
      // ���b�Z�[�W�̖|��Ƒ��o
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }

  return message;
}

void uninitWindow()
{
  // �E�B���h�E�N���X�o�^����
  UnregisterClass(g_strClassName.c_str(), GetModuleHandle(NULL));
}

void uninitVulkan()
{
  // 05
  for(uint32_t i = 0; i < 2; i++)
  {
    if(g_vkSwapChainImageViews[i])
    {
      vkDestroyImageView(
        g_vkDevice,
        g_vkSwapChainImageViews[i],
        nullptr);
      g_vkSwapChainImageViews[i] = 0L;
      g_vkSwapChainImages[i] = 0L;
    }
  }

  // 04
  if(g_vkDrawFence)
  {
    vkDestroyFence(
      g_vkDevice,
      g_vkDrawFence,
      nullptr);
    g_vkDrawFence = 0L;
  }

  // 03 : 04�ƑO�サ�Ă�
  // �R�}���h�o�b�t�@������J��
  //for(int32_t i = 0; i < MAX_COMMAND_BUFFER_COUNT; i++)
  //{
  //  if(g_vkCmdBufs[i])
  //  {
  //    vkFreeCommandBuffers(
  //      g_vkDevice,
  //      g_vkCmdPool,
  //      1,
  //      &g_vkCmdBufs[i]);
  //    g_vkCmdBufs[i] = nullptr;
  //  }
  //}
  if(g_vkCmdBufs[0] && g_vkCmdBufs[1])
  {
    vkFreeCommandBuffers(
      g_vkDevice,
      g_vkCmdPool,
      MAX_COMMAND_BUFFER_COUNT,
      g_vkCmdBufs);
    memset(g_vkCmdBufs, 0, sizeof(g_vkCmdBufs));
  }
  if(g_vkCmdPool)
  {
    vkDestroyCommandPool(
      g_vkDevice,
      g_vkCmdPool,
      nullptr);
    g_vkCmdPool = 0L;
  }

  // 04
  if(g_vkSwapchain)
  {
    vkDestroySwapchainKHR(
      g_vkDevice,
      g_vkSwapchain,
      nullptr);
    g_vkSwapchain = 0L;
  }
  if(g_vkSurface)
  {
    vkDestroySurfaceKHR(
      g_vkInstance,
      g_vkSurface,
      nullptr);
    g_vkSurface = 0L;
  }

  // 02
  if(g_vkDevice)
  {
    vkDestroyDevice(g_vkDevice, nullptr);
    g_vkDevice = nullptr;
  }

  // 01
  // PhysicalDevice�I�u�W�F�N�g�͗񋓂��ꂽ�����ŁA�������ꂽ�킯�ł͂Ȃ�
  memset(g_vkPhysicalDevices, 0, sizeof(g_vkPhysicalDevices));

  // 00
  if(g_vkInstance)
  {
    vkDestroyInstance(g_vkInstance, nullptr);
    g_vkInstance = nullptr;
  }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  // ���b�Z�[�W�ɂ���ĕ���
  switch(uMsg)
  {
    // �E�B���h�E�j��
  case WM_DESTROY:
    // �I�����b�Z�[�W���o
    PostQuitMessage(0);
    break;

    // �L�[����
  case WM_KEYDOWN:
    // �L�[���͏���
    KeyInputProc(hWnd, wParam);
    break;

    // �E�B���h�E�T�C�Y�ύX��
  case WM_SIZE:
    break;

  default:
    break;
  }

  // �W�������ɓ�����
  return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void KeyInputProc(HWND hWnd, WPARAM wParam)
{
  switch(wParam)
  {
    // �G�X�P�[�v�������ꂽ�Ƃ�
  case VK_ESCAPE:
    DestroyWindow(hWnd);
    break;
  }
}

// EOF
