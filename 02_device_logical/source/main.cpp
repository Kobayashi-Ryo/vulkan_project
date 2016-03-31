//*****************************************************************************
//
// �G���g���[�|�C���g
// Author : Ryo Kobayashi
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
#define UNUSED(x) x;
#define WND_MESSAGE_CONTINUE 1
#define WND_MESSAGE_END 0

//*****************************************************************************
// �萔��`
//*****************************************************************************
const int32_t GAME_WINDOW = (WS_OVERLAPPEDWINDOW ^ (WS_THICKFRAME | WS_MAXIMIZEBOX));
const uint32_t MAX_GPU_COUNT = 4;
const uint32_t MAX_QUEUE_FAMILY_COUNT = 4;

//*****************************************************************************
// �v���g�^�C�v��`
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
VkDevice g_vkDevice;

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
  if(g_vkDevice)
  {
    vkDestroyDevice(g_vkDevice, nullptr);
    g_vkDevice = nullptr;
  }

  // PhysicalDevice�I�u�W�F�N�g�͗񋓂��ꂽ�����ŁA�������ꂽ�킯�ł͂Ȃ�
  memset(g_vkPhysicalDevices, 0, sizeof(g_vkPhysicalDevices));

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
