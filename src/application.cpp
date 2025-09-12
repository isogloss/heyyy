#include "application.h"
#include "monitor_manager.h"
#include "injection_manager.h"
#include "frame_projector.h"
#include <commctrl.h>
#include <sstream>
#include <iomanip>

#pragma comment(lib, "comctl32.lib")

const wchar_t* const Application::WINDOW_CLASS_NAME = L"Pick6MainWindow";

Application::Application()
    : m_hInstance(nullptr)
    , m_hwnd(nullptr)
    , m_btnStartStop(nullptr)
    , m_cmbMonitors(nullptr)
    , m_chkVSync(nullptr)
    , m_chkAutoReattach(nullptr)
    , m_lblStatus(nullptr)
    , m_txtDiagnostics(nullptr)
    , m_capturing(false)
    , m_vsyncEnabled(true)
    , m_autoReattachEnabled(true)
    , m_selectedMonitor(0)
{
}

Application::~Application() {
    Shutdown();
}

bool Application::Initialize(HINSTANCE hInstance) {
    m_hInstance = hInstance;

    // Initialize common controls
    INITCOMMONCONTROLSEX icc = {};
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icc);

    // Register window class
    WNDCLASSEXW wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WindowProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    wcex.lpszClassName = WINDOW_CLASS_NAME;
    wcex.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wcex.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

    if (!RegisterClassExW(&wcex)) {
        return false;
    }

    // Create main window
    const int width = 600;
    const int height = 400;
    const int x = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
    const int y = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;

    m_hwnd = CreateWindowExW(
        0,
        WINDOW_CLASS_NAME,
        L"pick6 - FiveM Frame Projector",
        WS_OVERLAPPEDWINDOW,
        x, y, width, height,
        nullptr, nullptr, hInstance, this
    );

    if (!m_hwnd) {
        return false;
    }

    // Initialize core components
    m_monitorManager = std::make_unique<MonitorManager>();
    m_injectionManager = std::make_unique<InjectionManager>();
    m_frameProjector = std::make_unique<FrameProjector>();

    if (!m_monitorManager->Initialize()) {
        return false;
    }

    // Create UI controls
    CreateControls();
    LayoutControls();
    UpdateUI();

    // Show window
    ShowWindow(m_hwnd, SW_SHOW);
    UpdateWindow(m_hwnd);

    return true;
}

int Application::Run() {
    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        
        // Update diagnostics periodically
        static DWORD lastUpdate = 0;
        DWORD now = GetTickCount();
        if (now - lastUpdate > 1000) { // Update every second
            UpdateDiagnostics();
            lastUpdate = now;
        }
    }
    return static_cast<int>(msg.wParam);
}

void Application::Shutdown() {
    StopCapture();
    
    if (m_hwnd) {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
    
    if (m_hInstance) {
        UnregisterClassW(WINDOW_CLASS_NAME, m_hInstance);
    }
}

LRESULT CALLBACK Application::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    Application* app = nullptr;
    
    if (uMsg == WM_NCCREATE) {
        CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        app = static_cast<Application*>(cs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
    } else {
        app = reinterpret_cast<Application*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }
    
    if (app) {
        return app->HandleMessage(uMsg, wParam, lParam);
    }
    
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT Application::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_COMMAND:
        {
            HWND control = reinterpret_cast<HWND>(lParam);
            WORD notifyCode = HIWORD(wParam);
            
            if (control == m_btnStartStop && notifyCode == BN_CLICKED) {
                OnStartStop();
            } else if (control == m_cmbMonitors && notifyCode == CBN_SELCHANGE) {
                OnMonitorSelect();
            } else if (control == m_chkVSync && notifyCode == BN_CLICKED) {
                OnVSyncToggle();
            } else if (control == m_chkAutoReattach && notifyCode == BN_CLICKED) {
                OnAutoReattachToggle();
            }
        }
        break;
        
    case WM_SIZE:
        LayoutControls();
        break;
        
    case WM_CLOSE:
        OnClose();
        break;
        
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
        
    default:
        return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
    }
    
    return 0;
}

void Application::CreateControls() {
    const DWORD buttonStyle = WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON;
    const DWORD labelStyle = WS_VISIBLE | WS_CHILD | SS_LEFT;
    const DWORD comboStyle = WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST;
    const DWORD checkStyle = WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX;
    const DWORD textStyle = WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL | ES_MULTILINE | ES_READONLY;

    // Start/Stop button
    m_btnStartStop = CreateWindowW(L"BUTTON", L"Start Capture",
        buttonStyle, 0, 0, 0, 0, m_hwnd, nullptr, m_hInstance, nullptr);

    // Monitor selection
    CreateWindowW(L"STATIC", L"Monitor:",
        labelStyle, 0, 0, 0, 0, m_hwnd, nullptr, m_hInstance, nullptr);
    
    m_cmbMonitors = CreateWindowW(L"COMBOBOX", nullptr,
        comboStyle, 0, 0, 0, 0, m_hwnd, nullptr, m_hInstance, nullptr);

    // Options
    m_chkVSync = CreateWindowW(L"BUTTON", L"VSync",
        checkStyle, 0, 0, 0, 0, m_hwnd, nullptr, m_hInstance, nullptr);
    
    m_chkAutoReattach = CreateWindowW(L"BUTTON", L"Auto-Reattach",
        checkStyle, 0, 0, 0, 0, m_hwnd, nullptr, m_hInstance, nullptr);

    // Status
    m_lblStatus = CreateWindowW(L"STATIC", L"Status: Ready",
        labelStyle, 0, 0, 0, 0, m_hwnd, nullptr, m_hInstance, nullptr);

    // Diagnostics
    CreateWindowW(L"STATIC", L"Diagnostics:",
        labelStyle, 0, 0, 0, 0, m_hwnd, nullptr, m_hInstance, nullptr);
    
    m_txtDiagnostics = CreateWindowW(L"EDIT", nullptr,
        textStyle, 0, 0, 0, 0, m_hwnd, nullptr, m_hInstance, nullptr);

    // Set default states
    Button_SetCheck(m_chkVSync, m_vsyncEnabled ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(m_chkAutoReattach, m_autoReattachEnabled ? BST_CHECKED : BST_UNCHECKED);

    // Populate monitor list
    if (m_monitorManager) {
        const auto& monitors = m_monitorManager->GetMonitors();
        for (size_t i = 0; i < monitors.size(); ++i) {
            std::wstringstream ws;
            ws << L"Monitor " << (i + 1) << L" (" << monitors[i].width << L"x" << monitors[i].height << L")";
            ComboBox_AddString(m_cmbMonitors, ws.str().c_str());
        }
        if (!monitors.empty()) {
            ComboBox_SetCurSel(m_cmbMonitors, 0);
        }
    }
}

void Application::LayoutControls() {
    RECT clientRect;
    GetClientRect(m_hwnd, &clientRect);
    
    const int margin = 10;
    const int buttonHeight = 25;
    const int labelHeight = 20;
    const int spacing = 5;
    
    int y = margin;
    const int controlWidth = (clientRect.right - clientRect.left) - 2 * margin;
    
    // Start/Stop button
    SetWindowPos(m_btnStartStop, nullptr, margin, y, controlWidth, buttonHeight,
        SWP_NOZORDER | SWP_NOACTIVATE);
    y += buttonHeight + spacing * 2;
    
    // Monitor selection
    HWND lblMonitor = GetWindow(m_cmbMonitors, GW_HWNDPREV);
    SetWindowPos(lblMonitor, nullptr, margin, y, controlWidth, labelHeight,
        SWP_NOZORDER | SWP_NOACTIVATE);
    y += labelHeight + spacing;
    
    SetWindowPos(m_cmbMonitors, nullptr, margin, y, controlWidth, buttonHeight * 8,
        SWP_NOZORDER | SWP_NOACTIVATE);
    y += buttonHeight + spacing * 2;
    
    // Options checkboxes
    const int checkWidth = controlWidth / 2 - spacing;
    SetWindowPos(m_chkVSync, nullptr, margin, y, checkWidth, buttonHeight,
        SWP_NOZORDER | SWP_NOACTIVATE);
    SetWindowPos(m_chkAutoReattach, nullptr, margin + checkWidth + spacing, y, checkWidth, buttonHeight,
        SWP_NOZORDER | SWP_NOACTIVATE);
    y += buttonHeight + spacing * 2;
    
    // Status
    SetWindowPos(m_lblStatus, nullptr, margin, y, controlWidth, labelHeight,
        SWP_NOZORDER | SWP_NOACTIVATE);
    y += labelHeight + spacing * 2;
    
    // Diagnostics label
    HWND lblDiag = GetWindow(m_txtDiagnostics, GW_HWNDPREV);
    SetWindowPos(lblDiag, nullptr, margin, y, controlWidth, labelHeight,
        SWP_NOZORDER | SWP_NOACTIVATE);
    y += labelHeight + spacing;
    
    // Diagnostics text
    const int diagHeight = (clientRect.bottom - clientRect.top) - y - margin;
    SetWindowPos(m_txtDiagnostics, nullptr, margin, y, controlWidth, diagHeight,
        SWP_NOZORDER | SWP_NOACTIVATE);
}

void Application::OnStartStop() {
    if (m_capturing) {
        StopCapture();
    } else {
        StartCapture();
    }
}

void Application::OnMonitorSelect() {
    m_selectedMonitor = ComboBox_GetCurSel(m_cmbMonitors);
}

void Application::OnVSyncToggle() {
    m_vsyncEnabled = Button_GetCheck(m_chkVSync) == BST_CHECKED;
    if (m_frameProjector) {
        m_frameProjector->SetVSyncEnabled(m_vsyncEnabled);
    }
}

void Application::OnAutoReattachToggle() {
    m_autoReattachEnabled = Button_GetCheck(m_chkAutoReattach) == BST_CHECKED;
}

void Application::OnClose() {
    StopCapture();
    DestroyWindow(m_hwnd);
}

void Application::StartCapture() {
    if (!m_injectionManager || !m_frameProjector) {
        SetWindowTextW(m_lblStatus, L"Status: Error - Components not initialized");
        return;
    }
    
    // Start injection
    if (!m_injectionManager->InjectIntoFiveM()) {
        SetWindowTextW(m_lblStatus, L"Status: Error - Failed to inject into FiveM");
        return;
    }
    
    // Start projection
    if (!m_frameProjector->StartProjection(m_selectedMonitor)) {
        SetWindowTextW(m_lblStatus, L"Status: Error - Failed to start projection");
        m_injectionManager->StopInjection();
        return;
    }
    
    m_capturing = true;
    SetWindowTextW(m_btnStartStop, L"Stop Capture");
    SetWindowTextW(m_lblStatus, L"Status: Capturing");
}

void Application::StopCapture() {
    if (m_capturing) {
        if (m_frameProjector) {
            m_frameProjector->StopProjection();
        }
        if (m_injectionManager) {
            m_injectionManager->StopInjection();
        }
        
        m_capturing = false;
        SetWindowTextW(m_btnStartStop, L"Start Capture");
        SetWindowTextW(m_lblStatus, L"Status: Ready");
    }
}

void Application::UpdateUI() {
    // Update monitor list if changed
    // This would be called if monitor configuration changes
}

void Application::UpdateDiagnostics() {
    std::wstringstream diagnostics;
    
    if (m_injectionManager) {
        auto stats = m_injectionManager->GetStatistics();
        diagnostics << L"FiveM Process: " << (stats.processFound ? L"Found" : L"Not Found") << L"\r\n";
        diagnostics << L"Injection Status: " << (stats.injected ? L"Injected" : L"Not Injected") << L"\r\n";
    }
    
    if (m_frameProjector) {
        auto stats = m_frameProjector->GetStatistics();
        diagnostics << L"Frames Received: " << stats.framesReceived << L"\r\n";
        diagnostics << L"Frames Rendered: " << stats.framesRendered << L"\r\n";
        diagnostics << L"FPS: " << std::fixed << std::setprecision(1) << stats.fps << L"\r\n";
        diagnostics << L"Latency: " << stats.latencyMs << L" ms\r\n";
    }
    
    SetWindowTextW(m_txtDiagnostics, diagnostics.str().c_str());
}