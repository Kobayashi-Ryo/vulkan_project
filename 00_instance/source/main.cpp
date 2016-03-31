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

//*****************************************************************************
// �v���g�^�C�v��`
//*****************************************************************************
void initWindow();
void initVulkan();

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
VkInstance g_vkInstance = nullptr;

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
  VkResult result;

  while(!g_bWindowInitialized)
  {
    Sleep(1);
  }
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
  if(g_vkInstance)
  {
    vkDestroyInstance(g_vkInstance, nullptr);
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
