//*****************************************************************************
//
// エントリーポイント
// Author : Ryo Kobayashi
//
// 画面をクリアする
//
//*****************************************************************************

//*****************************************************************************
// インクルードファイル
//*****************************************************************************
#include "main.h"

#include <string>
#include <thread>

#include <vulkan/vulkan.h>

//*****************************************************************************
// マクロ定義
//*****************************************************************************
#define UNUSED(x) x
#define WND_MESSAGE_CONTINUE 1
#define WND_MESSAGE_END 0
#define NUM_SAMPLES VK_SAMPLE_COUNT_1_BIT

//*****************************************************************************
// 定数定義
//*****************************************************************************
const int32_t GAME_WINDOW = (WS_OVERLAPPEDWINDOW ^ (WS_THICKFRAME | WS_MAXIMIZEBOX));
const uint32_t MAX_GPU_COUNT = 4;
const uint32_t MAX_QUEUE_FAMILY_COUNT = 4;
const uint32_t MAX_COMMAND_BUFFER_COUNT = 2;
const uint32_t FENCE_TIMEOUT = 1000000;

//*****************************************************************************
// プロトタイプ定義
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
// グローバル変数
//*****************************************************************************
// ウィンドウ
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

// アプリケーション
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
  // ウィンドウ矩形範囲設定
  rc.top = 0;
  rc.left = 0;
  rc.right = g_uWindowWidth;
  rc.bottom = g_uWindowHeight;

  // ウィンドウクラス初期化
  WNDCLASSEX wcex =
  {
    sizeof(WNDCLASSEX),
    CS_CLASSDC,
    WndProc,  // ウィンドウプロシージャ関数
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

  // ウィンドウクラス登録
  if(FAILED(RegisterClassEx(&wcex)))
  {
    MessageBox(NULL, "ウィンドウクラス登録失敗", "エラー", MB_OK);
    return;
  }

  // ウィンドウ幅合わせ
  AdjustWindowRect(
    &rc,    // ウィンドウ幅
    GAME_WINDOW, // ウィンドウスタイル
    FALSE);    // メニューの有無

               // ウィンドウ座標設定
               // タスクバーを除いたディスプレイ幅取得
  int32_t nDisplayWidth = GetSystemMetrics(SM_CXFULLSCREEN);
  int32_t nDisplayHeight = GetSystemMetrics(SM_CYFULLSCREEN);

  // ウィンドウ作成
  int32_t nWindowWidth = rc.right - rc.left;  // ウィンドウ横幅
  int32_t nWindowHeight = rc.bottom - rc.top;  // ウィンドウ縦幅
  int32_t nWindowPosX = (nDisplayWidth >> 1) - (nWindowWidth >> 1);  // ウィンドウ左上X座標
  int32_t nWindowPosY = (nDisplayHeight >> 1) - (nWindowHeight >> 1);  // ウィンドウ左上Y座標

  // ウィンドウの作成
  g_hWnd = CreateWindowEx(0,
    g_strClassName.c_str(),
    g_strCaption.c_str(),
    // 可変枠、最大化ボタンを取り除く
    GAME_WINDOW,
    nWindowPosX,
    nWindowPosY,
    nWindowWidth,
    nWindowHeight,
    NULL,
    NULL,
    hInstance,
    NULL);

  // 作成失敗表示
  if(!g_hWnd)
  {
    MessageBox(NULL, "ウィンドウ作成失敗", "エラー", MB_OK);
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

  // アプリケーション情報
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
  // インスタンス生成情報
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

  // インスタンス生成
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

  // このAPIで使用可能なGPUの数を取得
  result = vkEnumeratePhysicalDevices(
    g_vkInstance,
    &g_uPhysicalDeviceCount,
    nullptr);

  if(g_uPhysicalDeviceCount <= 0)
  {
    MessageBox(g_hWnd, "compatible gpu not found", "", MB_OK);
  }

  // GPUオブジェクト取得
  result = vkEnumeratePhysicalDevices(
    g_vkInstance,
    &g_uPhysicalDeviceCount,
    g_vkPhysicalDevices);

  // QueueFamily情報を取得
  vkGetPhysicalDeviceQueueFamilyProperties(
    g_vkPhysicalDevices[0],
    &g_uPhysDevQueueFamilyPropertyCount,
    nullptr);

  if(g_uPhysDevQueueFamilyPropertyCount <= 0)
  {
    MessageBox(g_hWnd, "queue family props not found", "", MB_OK);
  }

  // QueueFamilyオブジェクトを取得
  vkGetPhysicalDeviceQueueFamilyProperties(
    g_vkPhysicalDevices[0],
    &g_uPhysDevQueueFamilyPropertyCount,
    g_vkQueueFamilyProps);

  // GPUメモリープロパティ取得
  vkGetPhysicalDeviceMemoryProperties(
    g_vkPhysicalDevices[0],
    &g_vkPhysDevMemoryProps);
  // GPU情報取得
  vkGetPhysicalDeviceProperties(
    g_vkPhysicalDevices[0],
    &g_vkPhysDevProps);
}

void initVulkanLogicalDevice()
{
  // なぜかqueueが16個より多く作れる...
  // queueのpriorityは0.0が最低で1.0が最高
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

  // デバイス生成
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
    // エラー
    MessageBox(g_hWnd, "cannot create fence", "", MB_OK);
    exit(-1);
  }
}

void initVulkanCommandPool()
{
  if(g_uPhysDevQueueFamilyPropertyCount <= 0)
  {
    // エラー
    MessageBox(g_hWnd, "queue family not found", "", MB_OK);
    exit(-1);
  }

  // コマンドバッファプール作成
  uint32_t uQueueFamilyIndex = 0;
  VkCommandPoolCreateInfo infoCmdPool =
  {
    VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,       // type
    nullptr,                                          // next
    VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,  // flags このふらぐよくわからん。何故いきなりせっていするようにした？
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
    // エラー
    MessageBox(g_hWnd, "cannot create command pool", "", MB_OK);
    exit(-1);
  }

  // コマンドバッファ確保
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
    // エラー
    MessageBox(g_hWnd, "cannot allocate command buffers", "", MB_OK);
    exit(-1);
  }
}

void initVulkanSwapChain()
{
  VkResult result = VK_INCOMPLETE;
#ifdef WIN32
  // Win32依存
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
    // エラー
    MessageBox(g_hWnd, "cannot create surface", "", MB_OK);
    exit(-1);
  }

  // presentをサポートしているかどうか検査(普通はしてると思った)
  VkBool32* pSupportPresent = new VkBool32[g_vkQueueFamilyProps[0].queueCount];
  for(uint32_t i = 0; i < g_uPhysDevQueueFamilyPropertyCount; i++)
  {
    vkGetPhysicalDeviceSurfaceSupportKHR(
      g_vkPhysicalDevices[0],
      i,
      g_vkSurface,
      &pSupportPresent[i]);
  }
  // graphicsをサポートしているqueueを検索(普通はしてると思った)
  // 最初に見つかったやつでいいや
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
    // エラー
    MessageBox(g_hWnd, "queue supports graphics and present not found", "", MB_OK);
    exit(-1);
  }

  g_uGraphicsQueueFamilyIndex = uGraphicsQueueNodeIndex;

  // サポートされているsurfaceのformat一覧を取得
  // まずは個数
  uint32_t uFormatCount = 0;
  result = vkGetPhysicalDeviceSurfaceFormatsKHR(
    g_vkPhysicalDevices[0],
    g_vkSurface,
    &uFormatCount,
    nullptr);
  if(result != VK_SUCCESS)
  {
    // エラー
    MessageBox(g_hWnd, "cannot get surface format count", "", MB_OK);
    exit(-1);
  }
  // フォーマット一覧
  VkSurfaceFormatKHR* pSurfFormats = new VkSurfaceFormatKHR[uFormatCount];
  result = vkGetPhysicalDeviceSurfaceFormatsKHR(
    g_vkPhysicalDevices[0],
    g_vkSurface,
    &uFormatCount,
    pSurfFormats);
  if(result != VK_SUCCESS)
  {
    // エラー
    MessageBox(g_hWnd, "cannot get surface formats", "", MB_OK);
    exit(-1);
  }
  // フォーマットを取得
  g_vkSwapChainImageFormat = pSurfFormats[0].format;

  // サーフェスの設定可能なサイズを取得
  VkSurfaceCapabilitiesKHR surfCapabilities;
  result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
    g_vkPhysicalDevices[0], g_vkSurface, &surfCapabilities);
  if(result != VK_SUCCESS)
  {
    // エラー
    MessageBox(g_hWnd, "cannot get surface capabilities", "", MB_OK);
    exit(-1);
  }
  uint32_t uPresentModeCount;
  result = vkGetPhysicalDeviceSurfacePresentModesKHR(
    g_vkPhysicalDevices[0], g_vkSurface, &uPresentModeCount, nullptr);
  if(result != VK_SUCCESS)
  {
    // エラー
    MessageBox(g_hWnd, "cannot get present mode count", "", MB_OK);
    exit(-1);
  }
  VkPresentModeKHR* presentModes = new VkPresentModeKHR[uPresentModeCount];
  result = vkGetPhysicalDeviceSurfacePresentModesKHR(
    g_vkPhysicalDevices[0], g_vkSurface, &uPresentModeCount, presentModes);
  if(result != VK_SUCCESS)
  {
    // エラー
    MessageBox(g_hWnd, "cannot get present modes", "", MB_OK);
    exit(-1);
  }
  // サイズが定義されているならcapabilitiesのサーフェスサイズに
  // -1は来ないはず
  VkExtent2D swapChainExtent; // 単なる矩形サイズ
  if(surfCapabilities.currentExtent.width == (uint32_t)-1)
  {
    // 定義されてない
    swapChainExtent.width = g_uWindowWidth;
    swapChainExtent.height = g_uWindowHeight;
  }
  else
  {
    // サイズが定義されている
    swapChainExtent = surfCapabilities.currentExtent;
  }

  // プレゼントモードを決定する
  // MAILBOXモード : 待ち時間が短い&ティアリングしない
  // IMMEDIATEモード : 一番速い&ティアリングが発生
  // FIFOモード : 
  // V-Syncモードだと思っていいかも
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

  // SwapChainで使用するimageオブジェクトの個数を決定
  uint32_t uDesiredNumberOfSwapChainImages =
    surfCapabilities.minImageCount + 1;
  if(surfCapabilities.maxImageCount > 0 &&
    uDesiredNumberOfSwapChainImages > surfCapabilities.maxImageCount)
  {
    // SwapChainが多すぎてはいけないので最大値に制限する
    uDesiredNumberOfSwapChainImages = surfCapabilities.maxImageCount;
  }

  // このフラグはまだreferenceページでも記述されていない
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
    swapChainExtent,                              // image extent (横幅縦幅)
    1,                                            // image array layers
    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,          // image usage flag (Framebufferっぽいなぁ)
    VK_SHARING_MODE_EXCLUSIVE,                    // image sharing mode (queue family間？)
    1,                                            // queue family index count
    &g_uGraphicsQueueFamilyIndex,                 // queue family indices
    preTransform,                                 // 画像を変換する (3DSみたいなやつかな？)
    VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,            // 多分アルファブレンドしないとか？
    swapchainPresentMode,                         // present mode
    true,                                         // clipping
    0L,                                           // old swapchain
  };

  // ふえぇ…やっとこさスワップチェイン作成だぁ…
  result = vkCreateSwapchainKHR(
    g_vkDevice,
    &infoSwapChain,
    nullptr,
    &g_vkSwapchain);
  if(result != VK_SUCCESS)
  {
    // エラー
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

  // バックバッファを作成する
  // スワップチェインに使用している画像数取得
  result = vkGetSwapchainImagesKHR(
    g_vkDevice,
    g_vkSwapchain,
    &g_uSwapchainImageCount,
    nullptr);
  if(result != VK_SUCCESS)
  {
    // エラー
    MessageBox(g_hWnd, "cannot get swapchain image count", "", MB_OK);
    exit(-1);
  }
  else if(2 != g_uSwapchainImageCount)
  {
    // エラー
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
    // エラー
    MessageBox(g_hWnd, "cannot get swapchain images", "", MB_OK);
    exit(-1);
  }

  VkImageViewCreateInfo arrInfo[2];

  // ビューを作成
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
      arrInfo[i].subresourceRange.aspectMask, // 色情報であることには変わりない
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
  // 深度フォーマット
  const VkFormat formatDepth = VK_FORMAT_D24_UNORM_S8_UINT;
  // 深度フォーマットが使用可能か調べる
  // (今時使えないGPUなんてあるんだろうか)
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
    // エラー
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
  infoImg.samples = NUM_SAMPLES;// image, renderpass, pipelinelayoutでもこことあわせる
  infoImg.tiling = imgTiling;
  infoImg.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  infoImg.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  infoImg.queueFamilyIndexCount = 0;
  infoImg.pQueueFamilyIndices = nullptr;
  infoImg.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

  // 画像のバッファ領域を確保
  VkMemoryAllocateInfo infoMemAlloc = {};
  infoMemAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  infoMemAlloc.pNext = nullptr;
  infoMemAlloc.allocationSize = 0;
  infoMemAlloc.memoryTypeIndex = 0;

  // viewを作成
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
    // エラー
    MessageBox(g_hWnd, "cannot begin command buffer", "", MB_OK);
    exit(-1);
  }
}

void endVulkanCommandBuffer(uint32_t idx)
{
  VkResult result = vkEndCommandBuffer(g_vkCmdBufs[idx]);
  if(result != VK_SUCCESS)
  {
    // エラー
    MessageBox(g_hWnd, "cannot end command buffer", "", MB_OK);
    exit(-1);
  }
}

void queueVulkanCommandBuffer()
{
  // queueにバッファを送出(やっとか…)
  VkPipelineStageFlags pipelineStageFlags =
    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT; // このフラグにすると、グラフィックスパイプラインのケツにくるみたい
  VkSubmitInfo infoSubmit[1] =
  {
    VK_STRUCTURE_TYPE_SUBMIT_INFO,  // type
    nullptr,                        // next
    0,                              // wait semaphore count
    nullptr,                        // wait semaphores (objects)
    &pipelineStageFlags,            // このフラグが設定されたパイプラインステージと同期をとる
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
    // エラー
    MessageBox(g_hWnd, "cannot submit command buffer to queue", "", MB_OK);
    exit(-1);
  }

  while((result = vkWaitForFences(
    g_vkDevice, 1, &g_vkDrawFence, VK_TRUE, FENCE_TIMEOUT)) == VK_TIMEOUT);

  if(result != VK_SUCCESS)
  {
    // エラー
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
    img,                                    // アクセスオーダーを変えたくない画像への参照
    subresourceRange                        // subresource range
  };

  // アクセスフラグを設定
  // このへんはFramebufferがテクスチャで使われるときとかに
  // GPUで書き込む前に、テクスチャで使うな～
  // って感じの命令
  // optimal : 最適(最適化についての設定か？)
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

  // とりあえずパイプラインの最初で全部済ますようにしてしまう
  VkPipelineStageFlags srcStages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  VkPipelineStageFlags dstStages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

  vkCmdPipelineBarrier(
    cmd,
    srcStages, dstStages,
    0,  // dependency flags
    0, nullptr, // 汎用メモリバリア
    0, nullptr, // バッファメモリバリア
    1, &barrierImage);
}

void procWindowThread()
{
  initWindow();
  
  // ウィンドウ表示
  ShowWindow(g_hWnd, SW_SHOW);
  UpdateWindow(g_hWnd);

  MSG msg;
  msg.message = WM_NULL;
  volatile bool bLoop = true;
  while(bLoop)
  {
    // 終了
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
  // メッセージがある場合処理
  int32_t message = WND_MESSAGE_CONTINUE;
  BOOL bIsMessageExist = PeekMessage(&msg, 0, 0, 0, PM_REMOVE);
  if(TRUE == bIsMessageExist)
  {
    // 終了メッセージが送られた場合
    if(msg.message == WM_QUIT)
    {
      message = WND_MESSAGE_END;
    }
    // 送られていない場合
    else
    {
      // メッセージの翻訳と送出
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }

  return message;
}

void uninitWindow()
{
  // ウィンドウクラス登録解除
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

  // 03 : 04と前後してる
  // コマンドバッファを一個ずつ開放
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
  // PhysicalDeviceオブジェクトは列挙されただけで、生成されたわけではない
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
  // メッセージによって分岐
  switch(uMsg)
  {
    // ウィンドウ破棄
  case WM_DESTROY:
    // 終了メッセージ送出
    PostQuitMessage(0);
    break;

    // キー入力
  case WM_KEYDOWN:
    // キー入力処理
    KeyInputProc(hWnd, wParam);
    break;

    // ウィンドウサイズ変更時
  case WM_SIZE:
    break;

  default:
    break;
  }

  // 標準処理に投げる
  return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void KeyInputProc(HWND hWnd, WPARAM wParam)
{
  switch(wParam)
  {
    // エスケープが押されたとき
  case VK_ESCAPE:
    DestroyWindow(hWnd);
    break;
  }
}

// EOF
