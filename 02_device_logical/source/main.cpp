//*****************************************************************************
//
// エントリーポイント
// Author : Ryo Kobayashi
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
#define UNUSED(x) x;
#define WND_MESSAGE_CONTINUE 1
#define WND_MESSAGE_END 0

//*****************************************************************************
// 定数定義
//*****************************************************************************
const int32_t GAME_WINDOW = (WS_OVERLAPPEDWINDOW ^ (WS_THICKFRAME | WS_MAXIMIZEBOX));
const uint32_t MAX_GPU_COUNT = 4;
const uint32_t MAX_QUEUE_FAMILY_COUNT = 4;

//*****************************************************************************
// プロトタイプ定義
//*****************************************************************************
void initWindow();
void initVulkan();
void initVulkanInstance();
void initVulkanPhysicalDevices();
void initVulkanLogicalDevice();

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
VkDevice g_vkDevice;

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
  if(g_vkDevice)
  {
    vkDestroyDevice(g_vkDevice, nullptr);
    g_vkDevice = nullptr;
  }

  // PhysicalDeviceオブジェクトは列挙されただけで、生成されたわけではない
  memset(g_vkPhysicalDevices, 0, sizeof(g_vkPhysicalDevices));

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
