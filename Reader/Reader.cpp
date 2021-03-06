// Reader.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "Reader.h"
#include "TextBook.h"
#include "EpubBook.h"
#include "Keyset.h"
#include "Editctrl.h"
#include "Advset.h"
#include "DPIAwareness.h"
#include "barcode.h"
#if ENABLE_TAG
#include "tagset.h"
#endif
#include <stdio.h>
#include <shlwapi.h>
#include <CommDlg.h>
#include <commctrl.h>
#include <vector>
#ifdef _DEBUG
#include "dump.h"
#endif

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "shell32.lib")

#define MIN_ALPHA_VALUE             0x64

#define MAX_LOADSTRING              100

// Global Variables:
HINSTANCE hInst;                                // current instance
TCHAR szTitle[MAX_LOADSTRING];                    // The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
TCHAR szStatusClass[MAX_LOADSTRING];            // the status window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    Setting(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    Proxy(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    BgImage(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    JumpProgress(HWND, UINT, WPARAM, LPARAM);
#if ENABLE_NETWORK
INT_PTR CALLBACK    UpgradeProc(HWND, UINT, WPARAM, LPARAM);
#endif



int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

     // TODO: Place code here.
#ifdef _DEBUG
    SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)ApplicationCrashHandler);
#endif
    MSG msg;
    HACCEL hAccelTable;
    HANDLE hMutex;

    // Initialize global strings
    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadString(hInstance, IDC_READER, szWindowClass, MAX_LOADSTRING);
    LoadString(hInstance, IDC_STATUSBAR, szStatusClass, MAX_LOADSTRING);

    // check if application is already running
    hMutex = OpenMutex(MUTEX_ALL_ACCESS, TRUE, szTitle);
    if (hMutex == NULL && ERROR_FILE_NOT_FOUND == GetLastError())
    {
        hMutex = CreateMutex(NULL, TRUE, szTitle);
    }
    else
    {
        // There's another instance running.
        static int flag = 0;
        EnumWindows(EnumWindowsProc, (LPARAM)&flag);
        return 0;
    }

    // init gdiplus
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    // init com
    CoInitialize(NULL);

    // load cache file
    if (!Init())
    {
        return FALSE;
    }

    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_READER));

    // Main message loop:
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (!_hFindDlg || !IsDialogMessage(_hFindDlg, &msg))
        {
            if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }

    Exit();

    // uninit com
    CoUninitialize();

    // uninit gdiplus
    GdiplusShutdown(gdiplusToken);

    ReleaseMutex(hMutex);
    CloseHandle(hMutex);

    return (int) msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style            = CS_HREDRAW | CS_VREDRAW /*| CS_DBLCLKS*/;
    wcex.lpfnWndProc      = WndProc;
    wcex.cbClsExtra       = 0;
    wcex.cbWndExtra       = 0;
    wcex.hInstance        = hInstance;
    wcex.hIcon            = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_BOOK));
    wcex.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground    = CreateSolidBrush(_header->bg_color);//(HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName     = MAKEINTRESOURCE(IDC_READER);
    wcex.lpszClassName    = szWindowClass;
    wcex.hIconSm          = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_BOOK));

    return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable

   // prepare for XP style controls
   InitCommonControls();

   // fixed rect exception bug.
   if (_header->rect.right <= 0 || _header->rect.bottom <= 0)
   {
       int w = _header->rect.right - _header->rect.left;
       int h = _header->rect.bottom - _header->rect.top;
       if (w <= 0 || h <= 0)
       {
           // using default
           w = 300;
           h = 500;
       }
       _header->rect.left = (GetSystemMetrics(SM_CXSCREEN) - w) / 2;
       _header->rect.top = (GetSystemMetrics(SM_CYSCREEN) - h) / 2;
       _header->rect.right = _header->rect.left + w;
       _header->rect.bottom = _header->rect.top + h;
   }
   // -- fixed end
   
   hWnd = CreateWindowEx(WS_EX_ACCEPTFILES | WS_EX_LAYERED, szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      _header->rect.left, _header->rect.top, 
      _header->rect.right - _header->rect.left,
      _header->rect.bottom - _header->rect.top,
      NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }
   _hWnd = hWnd;
   UpdateLayoutForDpi(hWnd, &_header->rect, &_header->isDefault);
   UpdateFontForDpi(hWnd, &_header->font);
#if ENABLE_TAG
   for (int i=0; i<MAX_TAG_COUNT; i++)
   {
       UpdateFontForDpi(hWnd, &_header->tags[i].font);
   }
#endif
   SetLayeredWindowAttributes(hWnd, 0, _header->alpha < MIN_ALPHA_VALUE ? MIN_ALPHA_VALUE : _header->alpha, LWA_ALPHA);

   ShowSysTray(hWnd, _header->show_systray);
   ShowInTaskbar(hWnd, !_header->hide_taskbar);

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND    - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY    - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int wmId, wmEvent;
    PAINTSTRUCT ps;
    HDC hdc;
    LRESULT hit;
    POINT pt;
    RECT rc;
    static RECT s_rect;

    if (message == _uFindReplaceMsg)
    {
        OnFindText(hWnd, message, wParam, lParam);
        return 0;
    }
    if (WM_TASKBAR_CREATED == message)
    {
        ShowSysTray(hWnd, TRUE);
    }
    switch (message)
    {
    case WM_COMMAND:
        PauseAutoPage(hWnd);
        wmId    = LOWORD(wParam);
        wmEvent = HIWORD(wParam);
        // Parse the menu selections:
        if (wmId >= IDM_CHAPTER_BEGIN && wmId <= IDM_CHAPTER_END)
        {
            if (_Book)
                _Book->JumpChapter(hWnd, wmId);
            ResumeAutoPage(hWnd);
            break;
        }
        else if (wmId >= IDM_OPEN_BEGIN && wmId <= IDM_OPEN_END)
        {
            int item_id = wmId - IDM_OPEN_BEGIN;
            OnOpenItem(hWnd, item_id, FALSE);
            ResumeAutoPage(hWnd);
            break;
        }
        switch (wmId)
        {
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        case IDM_OPEN:
            OnOpenFile(hWnd, message, wParam, lParam);
            break;
        case IDM_CLEAR:
            OnClearFileList(hWnd, message, wParam, lParam);
            break;
        case IDM_FONT:
            OnSetFont(hWnd, message, wParam, lParam);
            break;
        case IDM_COLOR:
            OnSetBkColor(hWnd, message, wParam, lParam);
            break;
        case IDM_IMAGE:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_BG_IMAGE), hWnd, BgImage);
            break;
        case IDM_CONFIG:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_SETTING), hWnd, Setting);
            break;
        case IDM_KEYSET:
            KS_OpenDlg(hInst, hWnd);
            break;
        case IDM_ADVSET:
            ADV_OpenDlg(hInst, hWnd, &(_header->chapter_rule), _Book);
            break;
#if ENABLE_TAG
        case IDM_TAGSET:
            TS_OpenDlg(hInst, hWnd, _header->tags);
            if (_Book && !_Book->IsLoading())
            {
                _Book->Reset(hWnd, TRUE);
            }
            break;
#endif
        case IDM_PROXY:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_PROXY), hWnd, Proxy);
            break;
        case IDM_DEFAULT:
            OnRestoreDefault(hWnd, message, wParam, lParam);
            break;
        case IDM_VIEW:
            if (IsWindowVisible(_hTreeView))
            {
                ShowWindow(_hTreeView, SW_HIDE);
            }
            else
            {
                if (_Book && !_Book->IsLoading())
                {
                    TVITEM tvi;
                    HTREEITEM hti;
                    int index = _Book->GetCurChapterIndex();
                    if (index == -1)
                    {
                        break;
                    }
                    else if (index == 0)
                    {
                        hti = TreeView_GetRoot(_hTreeView);
                        TreeView_EnsureVisible(_hTreeView, hti);
                        TreeView_SelectItem(_hTreeView, hti);
                    }
                    else
                    {
                        hti = TreeView_GetRoot(_hTreeView);
                        do 
                        {
                            tvi.mask       = TVIF_PARAM;
                            tvi.hItem      = hti;
                            TreeView_GetItem(_hTreeView, &tvi);
                            if (tvi.lParam == index)
                            {
                                TreeView_EnsureVisible(_hTreeView, hti);
                                TreeView_SelectItem(_hTreeView, hti);
                                break;
                            }
                            hti = TreeView_GetNextSibling(_hTreeView, hti);
                        } while (hti);
                    }

                    GetClientRectExceptStatusBar(hWnd, &rc);
                    SetWindowPos(_hTreeView, NULL, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, SWP_SHOWWINDOW);
                    SetFocus(_hTreeView);
                }
            }
            break;
        case IDM_MARK:
            if (IsWindowVisible(_hTreeMark))
            {
                ShowWindow(_hTreeMark, SW_HIDE);
            }
            else
            {
                if (_Book && !_Book->IsLoading() && _item)
                {
                    if (_item->mark_size <= 0)
                        break;
                    GetClientRectExceptStatusBar(hWnd, &rc);
                    SetWindowPos(_hTreeMark, NULL, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, SWP_SHOWWINDOW);
                    SetFocus(_hTreeMark);
                }
            }
            break;
        default:
            ResumeAutoPage(hWnd);
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        ResumeAutoPage(hWnd);
        break;
    case WM_NOTIFY:
        switch (((LPNMHDR)lParam)->code)
        {
        case TVN_SELCHANGED:
            if (((LPNMHDR)lParam)->hwndFrom == _hTreeView)
            {
                if (IsWindowVisible(_hTreeView))
                {
                    HTREEITEM Selected;
                    TVITEM item;
                    Selected = TreeView_GetSelection(_hTreeView);
                    if (Selected)
                    {
                        item.mask       = TVIF_PARAM;
                        item.hItem      = Selected;
                        TreeView_GetItem(_hTreeView, &item);
                        if (_Book && !_Book->IsLoading())
                            _Book->JumpChapter(hWnd, item.lParam);
                        //ShowWindow(_hTreeView, SW_HIDE);
                        PostMessage(hWnd, WM_COMMAND, IDM_VIEW, NULL);
                    }
                }
            }
            break;
        case NM_RCLICK:
            if (((LPNMHDR)lParam)->hwndFrom == _hTreeMark)
            {
                HTREEITEM hItem = TreeView_GetNextItem(((LPNMHDR)lParam)->hwndFrom, 0, TVGN_DROPHILITE);
                if(hItem)
                    TreeView_SelectItem(((LPNMHDR)lParam)->hwndFrom, hItem);
            }
            break;
        case NM_DBLCLK:
            if (((LPNMHDR)lParam)->hwndFrom == _hTreeMark)
            {
                if (IsWindowVisible(_hTreeMark))
                {
                    HTREEITEM Selected;
                    TVITEM item;
                    Selected = TreeView_GetSelection(_hTreeMark);
                    if (Selected)
                    {
                        item.mask       = TVIF_PARAM;
                        item.hItem      = Selected;
                        TreeView_GetItem(_hTreeMark, &item);
                        if (_Book && !_Book->IsLoading() && _item)
                        {
                            _item->index = _item->mark[item.lParam];
                            _Book->Reset(hWnd);
                        }
                        //ShowWindow(_hTreeMark, SW_HIDE);
                        PostMessage(hWnd, WM_COMMAND, IDM_MARK, NULL);
                    }
                }
            }
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;
    case WM_CONTEXTMENU:
        if (wParam == (WPARAM)_hTreeMark)
        {
            POINT pt;
            pt.x = LOWORD(lParam); 
            pt.y = HIWORD(lParam);
            HMENU hMenu = CreatePopupMenu();
            if(hMenu)
            {
                InsertMenu(hMenu, -1, MF_BYPOSITION, IDM_MK_DEL, _T("Delete"));
                SetForegroundWindow(hWnd);
                int ret = TrackPopupMenu(hMenu, TPM_RETURNCMD, pt.x, pt.y, 0, _hTreeMark, NULL);
                DestroyMenu(hMenu);
                if (IDM_MK_DEL == ret)
                {
                    HTREEITEM Selected;
                    TVITEM item;
                    Selected = TreeView_GetSelection(_hTreeMark);
                    if (Selected)
                    {
                        item.mask       = TVIF_PARAM;
                        item.hItem      = Selected;
                        TreeView_GetItem(_hTreeMark, &item);

                        if (_Cache.del_mark(_item, item.lParam))
                            OnUpdateBookMark(hWnd);
                    }
                }
            }
        }
        break;
    case WM_CREATE:
        OnCreate(hWnd);
        break;
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        // TODO: Add any drawing code here...
        if (_WndInfo.bHideBorder || _WndInfo.bFullScreen)
        {
            ResetLayerd(hWnd);
            OnDraw(hWnd);
        }
        else
        {
            ResetLayerd(hWnd);
            OnPaint(hWnd, hdc);
        }
        EndPaint(hWnd, &ps);
        break;
    case WM_CLOSE:
        StopAutoPage(hWnd);
        if (_header->show_systray)
        {
            _tcscpy(_nid.szInfoTitle, _T("Reader"));
            _tcscpy(_nid.szInfo, _T("I'm here!"));
            _nid.uTimeout = 1000;
            Shell_NotifyIcon(NIM_MODIFY, &_nid);
            ShowHideWindow(hWnd);
            _Cache.save();
        }
        else
        {
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;
    case WM_DESTROY:
        ShowSysTray(hWnd, FALSE);
        StopAutoPage(hWnd);
        KS_UnRegisterAllHotKey(hWnd);
        if (_hMouseHook)
        {
            UnhookWindowsHookEx(_hMouseHook);
            _hMouseHook = NULL;
        }
        PostQuitMessage(0);
        // save rect
        if (_WndInfo.bHideBorder && !_WndInfo.bFullScreen)
        {
            GetClientRectExceptStatusBar(hWnd, &rc);
            ClientToScreen(hWnd, reinterpret_cast<POINT*>(&rc.left));
            ClientToScreen(hWnd, reinterpret_cast<POINT*>(&rc.right));
            rc.left = rc.left - _WndInfo.hbRect.left;
            rc.right = rc.right + _WndInfo.hbRect.right;
            rc.top = rc.top - _WndInfo.hbRect.top;
            rc.bottom = rc.bottom + _WndInfo.hbRect.bottom;
            _header->rect.left = rc.left;
            _header->rect.top = rc.top;
            _header->rect.right = rc.right;
            _header->rect.bottom = rc.bottom;
        }
        RestoreRectForDpi(hWnd, &_header->rect);
        RestoreFontForDpi(hWnd, &_header->font);
#if ENABLE_TAG
        for (int i=0; i<MAX_TAG_COUNT; i++)
        {
            RestoreFontForDpi(hWnd, &_header->tags[i].font);
        }
#endif
        // stop loading
        StopLoadingImage(hWnd);
        if (_loading)
        {
            if (_loading->hMemory)
                ::GlobalFree(_loading->hMemory);
            if (_loading->image)
                delete _loading->image;
            free(_loading->item);
            free(_loading);
            _loading = NULL;
        }
        // close book
        if (_Book)
        {
            delete _Book;
            _Book = NULL;
        }
        break;
    case WM_QUERYENDSESSION:
        //Exit(); // save data when poweroff ?
        return TRUE;
    case WM_NCHITTEST:
        hit = DefWindowProc(hWnd, message, wParam, lParam);
        if (hit == HTCLIENT && _WndInfo.bHideBorder && !_WndInfo.bFullScreen)
        {
            if (_header->page_mode == 2)
            {
                GetCursorPos(&pt);
                ScreenToClient(hWnd, &pt);
                GetClientRectExceptStatusBar(hWnd, &rc);
                rc.left = rc.right/3;
                rc.right = rc.right/3 * 2;
                if (PtInRect(&rc, pt))
                    hit = HTCAPTION;
            }
            else
            {
                GetCursorPos(&pt);
                ScreenToClient(hWnd, &pt);
                GetClientRectExceptStatusBar(hWnd, &rc);
                rc.bottom = rc.bottom/2 > 80 ? 80 : rc.bottom/2;
                if (PtInRect(&rc, pt))
                    hit = HTCAPTION;
            }
        }
        return hit;
    case WM_SIZING:
        if (EC_IsEditMode())
        {
            RECT *lprc = NULL;
            lprc = (RECT *)lParam;
            lprc->left = s_rect.left;
            lprc->top = s_rect.top;
            lprc->right = s_rect.right;
            lprc->bottom = s_rect.bottom;
            return TRUE;
        }
        break;
    case WM_SIZE:
        OnSize(hWnd, message, wParam, lParam);
        if (!IsZoomed(hWnd) && !IsIconic(hWnd) && !_WndInfo.bHideBorder && !_WndInfo.bFullScreen)
            GetWindowRect(hWnd, &_header->rect);
        GetWindowRect(hWnd, &s_rect);
        break;
    case WM_MOVE:
        OnMove(hWnd);
        if (!IsZoomed(hWnd) && !IsIconic(hWnd) && !_WndInfo.bHideBorder && !_WndInfo.bFullScreen)
            GetWindowRect(hWnd, &_header->rect);
        break;
    case WM_WINDOWPOSCHANGED:
        return DefWindowProc(hWnd, message, wParam, lParam);
    case WM_KEYDOWN:
        if (_Book && _Book->IsLoading())
            break;

        if (KS_KeyDownProc(hWnd, message, wParam, lParam))
            break;

        if (VK_ESCAPE == wParam)
        {
            if (_WndInfo.bFullScreen)
            {
                OnFullScreen(hWnd, message, wParam, lParam);
            }
        }
        else if ('P' == wParam)
        {
            if (GetAsyncKeyState(VK_CONTROL) & 0x8000
                && GetAsyncKeyState(VK_SHIFT) & 0x8000
                && GetAsyncKeyState(18) & 0x8000) // alt
            {
                if (!_hMouseHook)
                    _hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, hInst, NULL);
                else
                {
                    UnhookWindowsHookEx(_hMouseHook);
                    _hMouseHook = NULL;
                }
            }
        }
        break;
    case WM_HOTKEY:
        KS_HotKeyProc(hWnd, message, wParam, lParam);
        break;
    case WM_LBUTTONDOWN:
        if ((wParam & MK_LBUTTON) && (wParam & MK_RBUTTON) && _header->disable_lrhide == 0)
        {
            ShowHideWindow(hWnd);
            break;
        }
        if (_header->page_mode == 1)
        {
            OnPageDown(hWnd, message, wParam, lParam);
        }
        else if (_header->page_mode == 2)
        {
            GetCursorPos(&pt);
            ScreenToClient(hWnd, &pt);
            GetClientRectExceptStatusBar(hWnd, &rc);
            rc.left = rc.right/3 * 2;
            if (PtInRect(&rc, pt))
            {
                OnPageDown(hWnd, message, wParam, lParam);
            }
            else
            {
                rc.left = 0;
                rc.right = rc.right/3;
                if (PtInRect(&rc, pt))
                {
                    OnPageUp(hWnd, message, wParam, lParam);
                }
            }
        }
        break;
    case WM_RBUTTONDOWN:
        if ((wParam & MK_LBUTTON) && (wParam & MK_RBUTTON) && _header->disable_lrhide == 0)
        {
            ShowHideWindow(hWnd);
            break;
        }
        if (_header->page_mode == 1)
        {
            OnPageUp(hWnd, message, wParam, lParam);
        }
        break;
    case WM_NCLBUTTONDOWN:
        if (_hMouseHook)
        {
            switch (wParam)
            {
            case HTMINBUTTON:
                //PostMessage(hWnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
                return FALSE;
            case HTMAXBUTTON:
                //PostMessage(hWnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
                return FALSE;
            case HTCLOSE:
                UnhookWindowsHookEx(_hMouseHook);
                _hMouseHook = NULL;
                break;
            default:
                break;
            }
        }
        if (IsWindowVisible(_hTreeView))
        {
            ShowWindow(_hTreeView, SW_HIDE);
            GetCursorPos(&pt);
            GetMenuItemRect(hWnd, GetMenu(hWnd), 1, &rc);
            if (PtInRect(&rc, pt))
            {
                break;
            }
        }
        if (IsWindowVisible(_hTreeMark))
        {
            ShowWindow(_hTreeMark, SW_HIDE);
            GetCursorPos(&pt);
            GetMenuItemRect(hWnd, GetMenu(hWnd), 2, &rc);
            if (PtInRect(&rc, pt))
            {
                break;
            }
        }
        return DefWindowProc(hWnd, message, wParam, lParam);
    case WM_NCRBUTTONDOWN:
        if (IsWindowVisible(_hTreeView))
        {
            ShowWindow(_hTreeView, SW_HIDE);
            GetCursorPos(&pt);
            GetMenuItemRect(hWnd, GetMenu(hWnd), 1, &rc);
            if (PtInRect(&rc, pt))
            {
                break;
            }
        }
        if (IsWindowVisible(_hTreeMark))
        {
            ShowWindow(_hTreeMark, SW_HIDE);
            GetCursorPos(&pt);
            GetMenuItemRect(hWnd, GetMenu(hWnd), 2, &rc);
            if (PtInRect(&rc, pt))
            {
                break;
            }
        }
        return DefWindowProc(hWnd, message, wParam, lParam);
    case WM_MOUSEWHEEL:
        {
            const BYTE MIN_ALPHA = 0x01;
            const BYTE MAX_ALPHA = 0xff;
            const BYTE UNIT_STEP = 0x05;
            if (IsWindowVisible(_hTreeView))
            {
                SetFocus(_hTreeView);
                break;
            }
            if (IsWindowVisible(_hTreeMark))
            {
                SetFocus(_hTreeMark);
                break;
            }
            if (GET_WHEEL_DELTA_WPARAM(wParam) > 0)
            {
                if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
                {
                    if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
                    {
                        _header->alpha = MAX_ALPHA;
                        if (_WndInfo.bHideBorder || _WndInfo.bFullScreen)
                        {
                            ResetLayerd(hWnd);
                            OnDraw(hWnd);
                        }
                        else
                        {
                            ResetLayerd(hWnd);
                            SetLayeredWindowAttributes(hWnd, 0, _header->alpha < MIN_ALPHA_VALUE ? MIN_ALPHA_VALUE : _header->alpha, LWA_ALPHA);
                        }
                    }
                    else
                    {
                        if (_header->alpha < MAX_ALPHA - UNIT_STEP)
                            _header->alpha += UNIT_STEP;
                        else
                            _header->alpha = MAX_ALPHA;
                        if (_WndInfo.bHideBorder || _WndInfo.bFullScreen)
                        {
                            ResetLayerd(hWnd);
                            OnDraw(hWnd);
                        }
                        else
                        {
                            ResetLayerd(hWnd);
                            SetLayeredWindowAttributes(hWnd, 0, _header->alpha < MIN_ALPHA_VALUE ? MIN_ALPHA_VALUE : _header->alpha, LWA_ALPHA);
                        }
                    }
                }
                else if (GetAsyncKeyState(18) & 0x8000) // alt
                {
                    if (_textAlpha < MAX_ALPHA - UNIT_STEP)
                        _textAlpha += UNIT_STEP;
                    else
                        _textAlpha = MAX_ALPHA;
                    if (_WndInfo.bHideBorder || _WndInfo.bFullScreen)
                    {
                        ResetLayerd(hWnd);
                        OnDraw(hWnd);
                    }
                    else
                    {
                        ResetLayerd(hWnd);
                        SetLayeredWindowAttributes(hWnd, 0, _header->alpha < MIN_ALPHA_VALUE ? MIN_ALPHA_VALUE : _header->alpha, LWA_ALPHA);
                    }
                }
                else
                {
                    OnLineUp(hWnd, message, wParam, lParam);
                }
            }
            else
            {
                if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
                {
                    if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
                    {
                        _header->alpha = MIN_ALPHA;
                        if (_WndInfo.bHideBorder || _WndInfo.bFullScreen)
                        {
                            ResetLayerd(hWnd);
                            OnDraw(hWnd);
                        }
                        else
                        {
                            ResetLayerd(hWnd);
                            SetLayeredWindowAttributes(hWnd, 0, _header->alpha < MIN_ALPHA_VALUE ? MIN_ALPHA_VALUE : _header->alpha, LWA_ALPHA);
                        }
                    }
                    else
                    {
                        if (_header->alpha > MIN_ALPHA + UNIT_STEP)
                            _header->alpha -= UNIT_STEP;
                        else
                            _header->alpha = MIN_ALPHA;
                        if (_WndInfo.bHideBorder || _WndInfo.bFullScreen)
                        {
                            ResetLayerd(hWnd);
                            OnDraw(hWnd);
                        }
                        else
                        {
                            ResetLayerd(hWnd);
                            SetLayeredWindowAttributes(hWnd, 0, _header->alpha < MIN_ALPHA_VALUE ? MIN_ALPHA_VALUE : _header->alpha, LWA_ALPHA);
                        }
                    }
                }
                else if (GetAsyncKeyState(18) & 0x8000) // alt
                {
                    if (_textAlpha > 0x0f + UNIT_STEP)
                        _textAlpha -= UNIT_STEP;
                    else
                        _textAlpha = 0x0f;
                    if (_WndInfo.bHideBorder || _WndInfo.bFullScreen)
                    {
                        ResetLayerd(hWnd);
                        OnDraw(hWnd);
                    }
                    else
                    {
                        ResetLayerd(hWnd);
                        SetLayeredWindowAttributes(hWnd, 0, _header->alpha < MIN_ALPHA_VALUE ? MIN_ALPHA_VALUE : _header->alpha, LWA_ALPHA);
                    }
                }
                else
                {
                    OnLineDown(hWnd, message, wParam, lParam);
                }
            }
        }
        break;
    case WM_ERASEBKGND:
        if (!_Book)
            DefWindowProc(hWnd, message, wParam, lParam);
        break;
    case WM_DROPFILES:
        OnDropFiles(hWnd, message, wParam, lParam);
        break;
    case WM_TIMER:
        switch (wParam)
        {
        case IDT_TIMER_PAGE:
            if (_header->autopage_mode == 0)
                OnPageDown(hWnd, message, wParam, lParam);
            else
                OnLineDown(hWnd, message, wParam, lParam);
            if (_Book && _Book->IsLastPage())
                StopAutoPage(hWnd);
            break;
#if ENABLE_NETWORK
        case IDT_TIMER_UPGRADE:
            _Upgrade.Check(UpgradeCallback, hWnd);
            break;
#endif
        case IDT_TIMER_LOADING:
            {
                GUID Guid;
                KillTimer(hWnd, IDT_TIMER_LOADING);
                Guid = FrameDimensionTime;
                _loading->image->SelectActiveFrame(&Guid, _loading->frameIndex);
                SetTimer(hWnd, IDT_TIMER_LOADING, ((UINT*)_loading->item[0].value)[_loading->frameIndex] * 10, NULL);
                _loading->frameIndex = (++ _loading->frameIndex) % _loading->frameCount;
                GetClientRectExceptStatusBar(hWnd, &rc);
                InvalidateRect(hWnd, &rc, FALSE);
            }
            break;
        default:
            break;
        }
        break;
    case WM_UPDATE_CHAPTERS:
        OnUpdateChapters(hWnd);
        OnUpdateBookMark(hWnd);
        break;
#if ENABLE_NETWORK
    case WM_NEW_VERSION:
        DialogBox(hInst, MAKEINTRESOURCE(IDD_UPGRADE), hWnd, UpgradeProc);
        break;
#endif
    case WM_OPEN_BOOK:
        OnOpenBookResult(hWnd, wParam == 1);
        break;
    case WM_SYSTRAY:
        switch(lParam)
        {
        case WM_LBUTTONUP:
            if (!IsWindowVisible(hWnd))
                ShowWindow(hWnd, SW_SHOW);
            if (IsIconic(hWnd))
                ShowWindow(hWnd, SW_RESTORE);
            SetForegroundWindow(hWnd);
            break;
        case WM_RBUTTONDOWN:
        case WM_CONTEXTMENU:
            {
                int ret;
                POINT pt;
                GetCursorPos(&pt);
                HMENU hMenu = CreatePopupMenu();
                if(hMenu)
                {
                    InsertMenu(hMenu, -1, MF_BYPOSITION, IDM_ST_OPEN, _T("Open"));
                    InsertMenu(hMenu, -1, MF_BYPOSITION, IDM_ST_EXIT, _T("Exit"));
                    SetForegroundWindow(hWnd);
                    ret = TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_RETURNCMD, pt.x, pt.y, 0, hWnd, NULL);
                    DestroyMenu(hMenu);
                    if (IDM_ST_OPEN == ret)
                    {
                        if (!IsWindowVisible(hWnd))
                            ShowWindow(hWnd, SW_SHOW);
                        if (IsIconic(hWnd))
                            ShowWindow(hWnd, SW_RESTORE);
                        SetForegroundWindow(hWnd);
                    }
                    else if (IDM_ST_EXIT == ret)
                    {
                        DestroyWindow(hWnd);
                    }
                }
            }
            break;
        }
        break;
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
        {
            hdc = (HDC)wParam;
            SetTextColor(hdc, _header->font_color);
            SetBkColor(hdc, _header->bg_color);
            SetDCBrushColor(hdc, _header->bg_color);
            return (LRESULT) GetStockObject(DC_BRUSH);
        }
        break;
    case WM_ENDSESSION:
        {
            // save rect
            if (_WndInfo.bHideBorder && !_WndInfo.bFullScreen)
            {
                GetClientRectExceptStatusBar(hWnd, &rc);
                ClientToScreen(hWnd, reinterpret_cast<POINT*>(&rc.left));
                ClientToScreen(hWnd, reinterpret_cast<POINT*>(&rc.right));
                rc.left = rc.left - _WndInfo.hbRect.left;
                rc.right = rc.right + _WndInfo.hbRect.right;
                rc.top = rc.top - _WndInfo.hbRect.top;
                rc.bottom = rc.bottom + _WndInfo.hbRect.bottom;
                _header->rect.left = rc.left;
                _header->rect.top = rc.top;
                _header->rect.right = rc.right;
                _header->rect.bottom = rc.bottom;
            }
            RestoreRectForDpi(hWnd, &_header->rect);
            RestoreFontForDpi(hWnd, &_header->font);
#if ENABLE_TAG
            for (int i=0; i<MAX_TAG_COUNT; i++)
            {
                RestoreFontForDpi(hWnd, &_header->tags[i].font);
            }
#endif
            Exit();
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;
    case 0x02E0: //WM_DPICHANGED
        {            
            NONCLIENTMETRICS theMetrics;
            theMetrics.cbSize = sizeof(NONCLIENTMETRICS);
            SystemParametersInfo(SPI_GETNONCLIENTMETRICS,sizeof(NONCLIENTMETRICS), (PVOID) &theMetrics,0);
            HFONT hFont = CreateFontIndirect(&(theMetrics.lfMenuFont));
            SendMessage(_hTreeView, WM_SETFONT, (WPARAM)hFont, NULL);
            SendMessage(_hTreeView, TVM_SETITEMHEIGHT, theMetrics.iMenuHeight, NULL);
            SendMessage(_hTreeMark, WM_SETFONT, (WPARAM)hFont, NULL);
            SendMessage(_hTreeMark, TVM_SETITEMHEIGHT, theMetrics.iMenuHeight, NULL);
            DpiChanged(hWnd, &_header->font, &_header->rect, wParam, (RECT*)lParam);
        }
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

Gdiplus::Bitmap *LoadBarCode(void)
{
    static Bitmap *image = NULL;
    IStream* pStream = NULL;
    void *buffer = NULL;
    HGLOBAL hMemory = NULL;

    if (!image)
    {
        hMemory = ::GlobalAlloc(GMEM_MOVEABLE, BC_IMAGE_LENGHT);
        if (!hMemory)
        {
            return image;
        }
        buffer = ::GlobalLock(hMemory);
        if (!buffer)
        {
            ::GlobalFree(hMemory);
            return image;
        }
        CopyMemory(buffer, bc_image, BC_IMAGE_LENGHT);
        if (::CreateStreamOnHGlobal(hMemory, FALSE, &pStream) == S_OK)
        {
            image = Gdiplus::Bitmap::FromStream(pStream);
            pStream->Release();
        }
        ::GlobalUnlock(hMemory);
        //::GlobalFree(hMemory);
    }

    if (!image || Gdiplus::Ok != image->GetLastStatus())
    {
        delete image;
        image = NULL;
        ::GlobalFree(hMemory);
    }

    return image;
}

INT_PTR CALLBACK RewardProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    Gdiplus::Bitmap *image = NULL;
    RECT client;
    double dpiscaled;

    switch (message)
    {
    case WM_INITDIALOG:
        image = LoadBarCode();
        if (image)
        {
            dpiscaled = GetDpiScaled(GetParent(GetParent(hDlg)));
            if (dpiscaled <= 1.0f)
            {
                ::SetRect(&client, 0, 0, (int)(image->GetWidth()/2.0f), (int)(image->GetHeight()/2.0f));
            }
            else if (dpiscaled <= 1.25)
            {
                ::SetRect(&client, 0, 0, (int)(image->GetWidth()/3.0f*2.0f), (int)(image->GetHeight()/3.0f*2.0f));
            }
            else
            {
                ::SetRect(&client, 0, 0, image->GetWidth(), image->GetHeight());
            }
            AdjustWindowRect(&client, GetWindowLong(hDlg, GWL_STYLE), FALSE);
            SetWindowPos(hDlg, NULL, client.left, client.top, client.right-client.left, client.bottom-client.top, SWP_NOMOVE);
        }
        return (INT_PTR)TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    case WM_PAINT:
        {
            HDC hdc;
            PAINTSTRUCT ps;
            Gdiplus::Graphics *graphics = NULL;
            Rect rect;
            
            hdc = BeginPaint(hDlg, &ps);
            // TODO: Add any drawing code here...
            image = LoadBarCode();
            if (!image || Gdiplus::Ok != image->GetLastStatus())
                return (INT_PTR)FALSE;

            GetClientRect(hDlg, &client);
            
            // draw image
            graphics = new Graphics(hdc);
            if (graphics)
            {
                rect.X = 0;
                rect.Y = 0;
                rect.Width = client.right-client.left;
                rect.Height = client.bottom-client.top;
                graphics->SetInterpolationMode(InterpolationModeHighQualityBicubic);
                graphics->DrawImage(image, rect, 0, 0, image->GetWidth(), image->GetHeight(), UnitPixel);
                delete graphics;
            }

            EndPaint(hDlg, &ps);
            return (INT_PTR)TRUE;
        }
        break;
    default:
        break;
    }
    return (INT_PTR)FALSE;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    POINT pt;
    HWND hWnd;
    RECT rc;

    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        else if (LOWORD(wParam) == IDC_BUTTON_REWARD)
        {
            if (LoadBarCode())
            {
                DialogBox(hInst, MAKEINTRESOURCE(IDD_REWARD), hDlg, RewardProc);
            }
        }
        break;
    case WM_LBUTTONDOWN:
        pt.x = LOWORD(lParam);
        pt.y = HIWORD(lParam);
        hWnd = GetDlgItem(hDlg, IDC_STATIC_LINK);
        ClientToScreen(hDlg, &pt);
        GetWindowRect(hWnd, &rc);
        if (PtInRect(&rc, pt))
        {
            ShellExecute(NULL, _T("open"), _T("https://github.com/binbyu/Reader"), NULL, NULL, SW_SHOWNORMAL);
        }
        break;
    default:
        break;
    }
    return (INT_PTR)FALSE;
}

INT_PTR CALLBACK Setting(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    BOOL bResult = FALSE;
    BOOL bUpdated = FALSE;
    int value = 0;
    int cid;
    LRESULT res;
    BYTE temp;
    switch (message)
    {
    case WM_INITDIALOG:
        SetDlgItemInt(hDlg, IDC_EDIT_LINEGAP, _header->line_gap, FALSE);
        SetDlgItemInt(hDlg, IDC_EDIT_BORDER, _header->internal_border, FALSE);
        SetDlgItemInt(hDlg, IDC_EDIT_ELAPSE, _header->uElapse, TRUE);
        if (_header->page_mode == 0)
            cid = IDC_RADIO_MODE1;
        else if (_header->page_mode == 1)
            cid = IDC_RADIO_MODE2;
        else
            cid = IDC_RADIO_MODE3;
        SendMessage(GetDlgItem(hDlg, cid), BM_SETCHECK, BST_CHECKED, NULL);
        WheelSpeedInit(hDlg);
        // init window style
        if (_header->show_systray)
            SendMessage(GetDlgItem(hDlg, IDC_CHECK_TRAY), BM_SETCHECK, BST_CHECKED, NULL);
        else
            SendMessage(GetDlgItem(hDlg, IDC_CHECK_TRAY), BM_SETCHECK, BST_UNCHECKED, NULL);
        if (_header->hide_taskbar)
        {
            SendMessage(GetDlgItem(hDlg, IDC_CHECK_TASKBAR), BM_SETCHECK, BST_CHECKED, NULL);
            SendMessage(GetDlgItem(hDlg, IDC_CHECK_TRAY), BM_SETCHECK, BST_CHECKED, NULL);
            EnableWindow(GetDlgItem(hDlg, IDC_CHECK_TRAY), FALSE);
        }
        else
            SendMessage(GetDlgItem(hDlg, IDC_CHECK_TASKBAR), BM_SETCHECK, BST_UNCHECKED, NULL);
        if (_header->disable_lrhide)
            SendMessage(GetDlgItem(hDlg, IDC_CHECK_LRHIDE), BM_SETCHECK, BST_UNCHECKED, NULL);
        else
            SendMessage(GetDlgItem(hDlg, IDC_CHECK_LRHIDE), BM_SETCHECK, BST_CHECKED, NULL);
        if (_header->autopage_mode == 0)
            SendMessage(GetDlgItem(hDlg, IDC_RADIO_ATPAGE), BM_SETCHECK, BST_CHECKED, NULL);
        else
            SendMessage(GetDlgItem(hDlg, IDC_RADIO_ATWHEEL), BM_SETCHECK, BST_CHECKED, NULL);
        SendMessage(GetDlgItem(hDlg, IDC_CHECK_MENU_FONT), BM_SETCHECK, _header->meun_font ? BST_CHECKED : BST_UNCHECKED, NULL);
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            GetDlgItemInt(hDlg, IDC_EDIT_LINEGAP, &bResult, FALSE);
            if (!bResult)
            {
                MessageBox(hDlg, _T("Invalid line gap value!"), _T("Error"), MB_OK|MB_ICONERROR);
                return (INT_PTR)TRUE;
            }
            GetDlgItemInt(hDlg, IDC_EDIT_BORDER, &bResult, FALSE);
            if (!bResult)
            {
                MessageBox(hDlg, _T("Invalid internal border value!"), _T("Error"), MB_OK|MB_ICONERROR);
                return (INT_PTR)TRUE;
            }
            value = GetDlgItemInt(hDlg, IDC_EDIT_ELAPSE, &bResult, FALSE);
            if (!bResult || value == 0)
            {
                MessageBox(hDlg, _T("Invalid auto page time value!"), _T("Error"), MB_OK|MB_ICONERROR);
                return (INT_PTR)TRUE;
            }
            if (SendMessage(GetDlgItem(hDlg, IDC_COMBO_SPEED), CB_GETCURSEL, 0, NULL) != -1)
            {
                _header->wheel_speed = SendMessage(GetDlgItem(hDlg, IDC_COMBO_SPEED), CB_GETCURSEL, 0, NULL) + 1;
            }
            res = SendMessage(GetDlgItem(hDlg, IDC_RADIO_MODE1), BM_GETCHECK, 0, NULL);
            if (res == BST_CHECKED)
            {
                _header->page_mode = 0;
            }
            else
            {
                res = SendMessage(GetDlgItem(hDlg, IDC_RADIO_MODE2), BM_GETCHECK, 0, NULL);
                if (res == BST_CHECKED)
                {
                    _header->page_mode = 1;
                }
                else
                {
                    _header->page_mode = 2;
                }
            }
            value = GetDlgItemInt(hDlg, IDC_EDIT_LINEGAP, &bResult, FALSE);
            if (value != _header->line_gap)
            {
                _header->line_gap = value;
                bUpdated = TRUE;
            }
            value = GetDlgItemInt(hDlg, IDC_EDIT_BORDER, &bResult, FALSE);
            if (value != _header->internal_border)
            {
                _header->internal_border = value;
                bUpdated = TRUE;
            }
            value = GetDlgItemInt(hDlg, IDC_EDIT_ELAPSE, &bResult, FALSE);
            if (value != _header->uElapse)
            {
                _header->uElapse = value;
            }
            // show taskbar/tray or not
            res = SendMessage(GetDlgItem(hDlg, IDC_CHECK_TASKBAR), BM_GETCHECK, 0, NULL);
            if ((res == BST_CHECKED && _header->hide_taskbar == 0)
                || (res == BST_UNCHECKED && _header->hide_taskbar == 1))
            {
                _header->hide_taskbar = !_header->hide_taskbar;
                ShowInTaskbar(GetParent(hDlg), !_header->hide_taskbar);
            }
            res = SendMessage(GetDlgItem(hDlg, IDC_CHECK_TRAY), BM_GETCHECK, 0, NULL);
            if ((res == BST_CHECKED && _header->show_systray == 0)
                || (res == BST_UNCHECKED && _header->show_systray == 1))
            {
                _header->show_systray = !_header->show_systray;
                ShowSysTray(GetParent(hDlg), _header->show_systray);
            }
            res = SendMessage(GetDlgItem(hDlg, IDC_CHECK_LRHIDE), BM_GETCHECK, 0, NULL);
            _header->disable_lrhide = res == BST_CHECKED ? 0 : 1;
            res = SendMessage(GetDlgItem(hDlg, IDC_RADIO_ATPAGE), BM_GETCHECK, 0, NULL);
            _header->autopage_mode = res == BST_CHECKED ? 0 : 1;
            res = SendMessage(GetDlgItem(hDlg, IDC_CHECK_MENU_FONT), BM_GETCHECK, 0, NULL);
            temp = res == BST_CHECKED ? 1 : 0;
            if (temp != _header->meun_font)
            {
                _header->meun_font = temp;
                SetTreeviewFont();
            }
            _header->meun_font = temp;
            if (bUpdated)
            {
                if (_Book)
                    _Book->Reset(GetParent(hDlg));
            }
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
            break;
        case IDCANCEL:
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
            break;
        case IDC_CHECK_TASKBAR:
            res = SendMessage(GetDlgItem(hDlg, IDC_CHECK_TASKBAR), BM_GETCHECK, 0, NULL);
            if (res == BST_CHECKED)
            {
                SendMessage(GetDlgItem(hDlg, IDC_CHECK_TRAY), BM_SETCHECK, BST_CHECKED, NULL);
                EnableWindow(GetDlgItem(hDlg, IDC_CHECK_TRAY), FALSE);
            }
            else
            {
                EnableWindow(GetDlgItem(hDlg, IDC_CHECK_TRAY), TRUE);
            }
            break;
        case IDC_CHECK_TRAY:
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
    return (INT_PTR)FALSE;
}

INT_PTR CALLBACK Proxy(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT res;
    BOOL bResult = FALSE;
    int value = 0;
    TCHAR buf[64] = {0};
    switch (message)
    {
    case WM_INITDIALOG:
        SendMessage(GetDlgItem(hDlg, IDC_COMBO_PROXY), CB_ADDSTRING, 0, (LPARAM)_T("不使用代理"));
        SendMessage(GetDlgItem(hDlg, IDC_COMBO_PROXY), CB_ADDSTRING, 0, (LPARAM)_T("使用代理"));
        SendMessage(GetDlgItem(hDlg, IDC_COMBO_PROXY), CB_SETCURSEL, _header->proxy.enable ? 1 : 0, NULL);
        SetWindowText(GetDlgItem(hDlg, IDC_EDIT_PROXY_ADDR), _header->proxy.addr);
        SetWindowText(GetDlgItem(hDlg, IDC_EDIT_PROXY_USER), _header->proxy.user);
        SetWindowText(GetDlgItem(hDlg, IDC_EDIT_PROXY_PASS), _header->proxy.pass);
        if (_header->proxy.port == 0)
            SetWindowText(GetDlgItem(hDlg, IDC_EDIT_PROXY_PORT), _T(""));
        else
            SetDlgItemInt(hDlg, IDC_EDIT_PROXY_PORT, _header->proxy.port, TRUE);
        if (_header->proxy.enable)
        {
            EnableWindow(GetDlgItem(hDlg, IDC_EDIT_PROXY_ADDR), TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_EDIT_PROXY_PORT), TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_EDIT_PROXY_USER), TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_EDIT_PROXY_PASS), TRUE);
        }
        else
        {
            EnableWindow(GetDlgItem(hDlg, IDC_EDIT_PROXY_ADDR), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_EDIT_PROXY_PORT), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_EDIT_PROXY_USER), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_EDIT_PROXY_PASS), FALSE);
        }
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            GetWindowText(GetDlgItem(hDlg, IDC_EDIT_PROXY_PORT), buf, 64-1);
            if (buf[0])
            {
                value = GetDlgItemInt(hDlg, IDC_EDIT_PROXY_PORT, &bResult, FALSE);
                if (!bResult || value <= 0 || value >= 0xFFFF)
                {
                    MessageBox(hDlg, _T("Invalid port value!"), _T("Error"), MB_OK|MB_ICONERROR);
                    return (INT_PTR)TRUE;
                }
                _header->proxy.port = value;
            }
            else
            {
                _header->proxy.port = 0;
            }

            res = SendMessage(GetDlgItem(hDlg, IDC_COMBO_PROXY), CB_GETCURSEL, 0, NULL);
            if (res == 1)
            {
                _header->proxy.enable = TRUE;
            }
            else
            {
                _header->proxy.enable = FALSE;
            }
            GetWindowText(GetDlgItem(hDlg, IDC_EDIT_PROXY_ADDR), buf, 64-1);
            wcscpy(_header->proxy.addr, buf);
            GetWindowText(GetDlgItem(hDlg, IDC_EDIT_PROXY_USER), buf, 64-1);
            wcscpy(_header->proxy.user, buf);
            GetWindowText(GetDlgItem(hDlg, IDC_EDIT_PROXY_PASS), buf, 64-1);
            wcscpy(_header->proxy.pass, buf);
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
            break;
        case IDCANCEL:
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
            break;
        case IDC_COMBO_PROXY:
            res = SendMessage(GetDlgItem(hDlg, IDC_COMBO_PROXY), CB_GETCURSEL, 0, NULL);
            if (res == 0)
            {
                EnableWindow(GetDlgItem(hDlg, IDC_EDIT_PROXY_ADDR), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_EDIT_PROXY_PORT), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_EDIT_PROXY_USER), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_EDIT_PROXY_PASS), FALSE);
            }
            else
            {
                EnableWindow(GetDlgItem(hDlg, IDC_EDIT_PROXY_ADDR), TRUE);
                EnableWindow(GetDlgItem(hDlg, IDC_EDIT_PROXY_PORT), TRUE);
                EnableWindow(GetDlgItem(hDlg, IDC_EDIT_PROXY_USER), TRUE);
                EnableWindow(GetDlgItem(hDlg, IDC_EDIT_PROXY_PASS), TRUE);
            }
            break;
        }
        break;
    default:
        break;
    }
    return (INT_PTR)FALSE;
}

INT_PTR CALLBACK BgImage(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT res;
    TCHAR text[MAX_PATH] = {0};
    TCHAR *ext;
    RECT rc;
    switch (message)
    {
    case WM_INITDIALOG:
        SendMessage(GetDlgItem(hDlg, IDC_CHECK_BIENABLE), BM_SETCHECK, _header->bg_image.enable ? BST_CHECKED : BST_UNCHECKED, NULL);
        SetWindowText(GetDlgItem(hDlg, IDC_EDIT_BIFILE), _header->bg_image.file_name);
        SendMessage(GetDlgItem(hDlg, IDC_COMBO_BIMODE), CB_ADDSTRING, 0, (LPARAM)_T("拉伸缩放"));
        SendMessage(GetDlgItem(hDlg, IDC_COMBO_BIMODE), CB_ADDSTRING, 1, (LPARAM)_T("平铺"));
        SendMessage(GetDlgItem(hDlg, IDC_COMBO_BIMODE), CB_ADDSTRING, 2, (LPARAM)_T("旋转平铺"));
        SendMessage(GetDlgItem(hDlg, IDC_COMBO_BIMODE), CB_SETCURSEL, _header->bg_image.mode, NULL);
        if (_header->bg_image.enable)
        {
            EnableWindow(GetDlgItem(hDlg, IDC_COMBO_BIMODE), TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_EDIT_BIFILE), TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_BISEL), TRUE);
        }
        else
        {
            EnableWindow(GetDlgItem(hDlg, IDC_COMBO_BIMODE), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_EDIT_BIFILE), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_BISEL), FALSE);
        }
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            res = SendMessage(GetDlgItem(hDlg, IDC_CHECK_BIENABLE), BM_GETCHECK, 0, NULL);
            if (res == BST_CHECKED)
            {
                GetWindowText(GetDlgItem(hDlg, IDC_EDIT_BIFILE), text, MAX_PATH-1);
                if (!text[0])
                {
                    MessageBox(hDlg, _T("Please select a image!"), _T("Error"), MB_OK|MB_ICONERROR);
                    return (INT_PTR)TRUE;
                }
                ext = PathFindExtension(text);
                if (_tcscmp(ext, _T(".jpg")) != 0
                    && _tcscmp(ext, _T(".png")) != 0
                    && _tcscmp(ext, _T(".bmp")) != 0
                    && _tcscmp(ext, _T(".jpeg")) != 0)
                {
                    MessageBox(hDlg, _T("Invalid image format!"), _T("Error"), MB_OK|MB_ICONERROR);
                    return (INT_PTR)TRUE;
                }
                if (!FileExists(text))
                {
                    MessageBox(hDlg, _T("image is not exists!"), _T("Error"), MB_OK|MB_ICONERROR);
                    return (INT_PTR)TRUE;
                }
                res = SendMessage(GetDlgItem(hDlg, IDC_COMBO_BIMODE), CB_GETCURSEL, 0, NULL);
                if (res == -1)
                {
                    MessageBox(hDlg, _T("Invalid layout mode format!"), _T("Error"), MB_OK|MB_ICONERROR);
                    return (INT_PTR)TRUE;
                }
                _header->bg_image.mode = res;
                _header->bg_image.enable = 1;
                _tcscpy(_header->bg_image.file_name, text);
                if (_Book)
                    _Book->ReDraw(GetParent(hDlg));
                else
                {
                    GetClientRectExceptStatusBar(GetParent(hDlg), &rc);
                    InvalidateRect(GetParent(hDlg), &rc, FALSE);
                }
            }
            else
            {
                _header->bg_image.enable = 0;
                if (_Book)
                    _Book->ReDraw(GetParent(hDlg));
                else
                {
                    GetClientRectExceptStatusBar(GetParent(hDlg), &rc);
                    InvalidateRect(GetParent(hDlg), &rc, FALSE);
                }
            }
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
            break;
        case IDCANCEL:
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
            break;
        case IDC_CHECK_BIENABLE:
            res = SendMessage(GetDlgItem(hDlg, IDC_CHECK_BIENABLE), BM_GETCHECK, 0, NULL);
            if (res == BST_CHECKED)
            {
                EnableWindow(GetDlgItem(hDlg, IDC_COMBO_BIMODE), TRUE);
                EnableWindow(GetDlgItem(hDlg, IDC_EDIT_BIFILE), TRUE);
                EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_BISEL), TRUE);
            }
            else
            {
                EnableWindow(GetDlgItem(hDlg, IDC_COMBO_BIMODE), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_EDIT_BIFILE), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_BISEL), FALSE);
            }
            break;
        case IDC_BUTTON_BISEL:
            if (IsWindow(_hFindDlg))
            {
                DestroyWindow(_hFindDlg);
                _hFindDlg = NULL;
            }
            TCHAR szFileName[MAX_PATH] = {0};
            TCHAR szTitle[MAX_PATH] = {0};
            OPENFILENAME ofn = {0};
            ofn.lStructSize = sizeof(ofn);  
            ofn.hwndOwner = hDlg;  
            ofn.lpstrFilter = _T("jpg(*.jpg)\0*.jpg\0jpeg(*.jpeg)\0*.jpeg\0png(*.png)\0*.png\0bmp(*.bmp)\0*.bmp\0\0");
            ofn.lpstrInitialDir = NULL;
            ofn.lpstrFile = szFileName; 
            ofn.nMaxFile = sizeof(szFileName)/sizeof(*szFileName);  
            ofn.nFilterIndex = 0;
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;
            BOOL bSel = GetOpenFileName(&ofn);
            if (!bSel)
            {
                return 0;
            }
            SetWindowText(GetDlgItem(hDlg, IDC_EDIT_BIFILE), szFileName);
            break;
        }
        break;
    default:
        break;
    }
    return (INT_PTR)FALSE;
}

INT_PTR CALLBACK JumpProgress(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND hWnd;
    TCHAR buf[16] = {0};
    double progress;

    switch (message)
    {
    case WM_INITDIALOG:
        hWnd = GetDlgItem(hDlg, IDC_EDIT_JP);
        if (_Book)
        {
            progress = _Book->GetProgress();
            _stprintf(buf, _T("%0.2f"), progress);
            SetWindowText(hWnd, buf);
        }
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            if (LOWORD(wParam) == IDOK)
            {
                if (_item)
                {
                    hWnd = GetDlgItem(hDlg, IDC_EDIT_JP);
                    GetWindowText(hWnd, buf, 15);
                    progress = _tstof(buf);
                    if (progress < 0.0 || progress > 100.0)
                    {
                        MessageBox(hDlg, _T("Invalid value!"), _T("Error"), MB_OK|MB_ICONERROR);
                        return (INT_PTR)TRUE;
                    }
                    if (_Book)
                    {
                        _item->index = (int)(progress * _Book->GetTextLength() / 100);
                        if (_item->index == _Book->GetTextLength())
                            _item->index--;
                        _Book->Reset(GetParent(hDlg));
                    }
                }
            }

            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    default:
        break;
    }
    return (INT_PTR)FALSE;
}

#if ENABLE_NETWORK
INT_PTR CALLBACK UpgradeProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    json_item_data_t *vinfo;
    switch (message)
    {
    case WM_INITDIALOG:
        vinfo = _Upgrade.GetVersionInfo();
        SetWindowText(GetDlgItem(hDlg, IDC_EDIT_UPGRADE_VERSION), vinfo->version.c_str());
        SetWindowText(GetDlgItem(hDlg, IDC_EDIT_UPGRADE_DESC), vinfo->desc.c_str());
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        case IDCANCEL:
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        case IDC_BUTTON_UPGRADE_INGORE:
            vinfo = _Upgrade.GetVersionInfo();
            wcscpy(_header->ingore_version, vinfo->version.c_str());
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
            break;
        case IDC_BUTTON_UPGRADE_DOWN:
            vinfo = _Upgrade.GetVersionInfo();
            ShellExecute(NULL, _T("open"), vinfo->url.c_str(), NULL, NULL, SW_SHOWNORMAL);
            break;
        }
        break;
    default:
        break;
    }
    return (INT_PTR)FALSE;
}
#endif

LRESULT OnCreate(HWND hWnd)
{
    _WndInfo.hMenu = GetMenu(hWnd);
    // create status bar
    _WndInfo.hStatusBar = CreateStatusWindow(WS_CHILD | WS_VISIBLE, _T("Please open a text."), hWnd, IDC_STATUSBAR);
    // register find text dialog
    _uFindReplaceMsg = RegisterWindowMessage(FINDMSGSTRING);
    // create chapters tree view
    _hTreeView = CreateWindow(WC_TREEVIEW, _T("Tree View"), 
        /*WS_VISIBLE | */WS_CHILD /*| WS_BORDER*/ | TVS_HASLINES | TVS_NOHSCROLL | TVS_NOTOOLTIPS | TVS_LINESATROOT,
        0, 0, 200, 300, hWnd, NULL, hInst, NULL);
    _hTreeMark = CreateWindow(WC_TREEVIEW, _T("Tree Mark"), 
        /*WS_VISIBLE | */WS_CHILD /*| WS_BORDER*/ | TVS_HASLINES | TVS_NOHSCROLL /*| TVS_NOTOOLTIPS*/ | TVS_LINESATROOT,
        0, 0, 200, 300, hWnd, NULL, hInst, NULL);
    //TreeView_SetBkColor(_hTreeView, GetSysColor(COLOR_MENU));
    //TreeView_SetBkColor(_hTreeMark, GetSysColor(COLOR_MENU));
    SetTreeviewFont();

    RemoveMenus(hWnd, FALSE);
    OnUpdateMenu(hWnd);

    // open the last file
    if (_header->item_count > 0)
    {
        item_t* item = _Cache.get_item(0);
        OnOpenBook(hWnd, item->file_name, FALSE);
    }

    // check upgrade
#if ENABLE_NETWORK
    CheckUpgrade(hWnd);
#endif

    // init keyset
    KS_Init(hWnd, _header->keyset);

    return 0;
}

LRESULT OnUpdateMenu(HWND hWnd)
{
    int menu_begin_id = IDM_OPEN_BEGIN;
    HMENU hMenuBar = GetMenu(hWnd);
    HMENU hFile = GetSubMenu(hMenuBar, 0);
    if (hFile)
    {
        DeleteMenu(hMenuBar, 0, MF_BYPOSITION);
        hFile = NULL;
    }
    hFile = CreateMenu();
    AppendMenu(hFile, MF_STRING, IDM_OPEN, _T("打开"));
    AppendMenu(hFile, MF_SEPARATOR, 0, NULL);
    for (int i=0; i<_header->item_count; i++)
    {
        item_t* item = _Cache.get_item(i);
        AppendMenu(hFile, MF_STRING, (UINT_PTR)menu_begin_id++, item->file_name);
    }
    if (_header->item_count > 0)
        AppendMenu(hFile, MF_SEPARATOR, 0, NULL);
    AppendMenu(hFile, MF_STRING, IDM_CLEAR, _T("清空"));
    AppendMenu(hFile, MF_SEPARATOR, 0, NULL);
    AppendMenu(hFile, MF_STRING, IDM_EXIT, _T("退出"));
    InsertMenu(hMenuBar, 0, MF_BYPOSITION | MF_STRING | MF_POPUP, (UINT_PTR)hFile, L"文件(&F)");
    DrawMenuBar(hWnd);

    return 0;
}

LRESULT OnOpenItem(HWND hWnd, int item_id, BOOL forced)
{
    if (IsWindow(_hFindDlg))
    {
        DestroyWindow(_hFindDlg);
        _hFindDlg = NULL;
    }
    if (!forced && _item && _item->id == item_id && _Book && !_Book->IsLoading())
    {
        return 0;
    }

    BOOL bNotExist = FALSE;
    item_t* item = _Cache.get_item(item_id);
    if (!item)
    {
        bNotExist = TRUE;
    }
    else
    {
        if (PathFileExists(item->file_name))
        {
            OnOpenBook(hWnd, item->file_name, forced);
        }
        else
        {
            bNotExist = TRUE;
        }
    }

    if (bNotExist)
    {
        _Cache.delete_item(item_id);
        // update menu
        OnUpdateMenu(hWnd);
        return 0L;
    }

    return 0;
}

LRESULT OnClearFileList(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    RECT rect;
    if (_Book)
    {
        delete _Book;
        _Book = NULL;
    }
    _Cache.delete_all_item();
    OnUpdateMenu(hWnd);
    PostMessage(hWnd, WM_UPDATE_CHAPTERS, 0, NULL);
    GetClientRectExceptStatusBar(hWnd, &rect);
    InvalidateRect(hWnd, &rect, FALSE);
    SetWindowText(hWnd, szTitle);
    StopLoadingImage(hWnd);
    StopAutoPage(hWnd);
    return 0;
}

LRESULT OnSetFont(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    CHOOSEFONT cf;            // common dialog box structure
    LOGFONT logFont;
    static DWORD rgbCurrent;   // current text color
    BOOL bUpdate = FALSE;

    // Initialize CHOOSEFONT
    ZeroMemory(&cf, sizeof(cf));
    memcpy(&logFont, &_header->font, sizeof(LOGFONT));
    cf.lStructSize = sizeof (cf);
    cf.hwndOwner = hWnd;
    cf.lpLogFont = &logFont;
    cf.rgbColors = _header->font_color;
    cf.Flags = CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS | CF_EFFECTS;

    if (ChooseFont(&cf))
    {
        if (_header->font_color != cf.rgbColors)
        {
            _header->font_color = cf.rgbColors;
            bUpdate = TRUE;
        }
        _header->font.lfQuality = PROOF_QUALITY;

        if (0 != memcmp(&logFont, &_header->font, sizeof(LOGFONT)))
        {
            memcpy(&_header->font, &logFont, sizeof(LOGFONT));
            bUpdate = TRUE;
        }
        if (bUpdate && _Book)
        {
            SetTreeviewFont();
            _Book->Reset(hWnd);
        }
    }

    return 0;
}

LRESULT OnSetBkColor(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    RECT rect;
    CHOOSECOLOR cc;                 // common dialog box structure 
    static COLORREF acrCustClr[16]; // array of custom colors 

    ZeroMemory(&cc, sizeof(cc));
    cc.lStructSize = sizeof(cc);
    cc.hwndOwner = hWnd;
    cc.lpCustColors = (LPDWORD) acrCustClr;
    cc.rgbResult = _header->bg_color;
    cc.Flags = CC_FULLOPEN | CC_RGBINIT;

    if (ChooseColor(&cc))
    {
        if (_header->bg_color != cc.rgbResult)
        {
            _header->bg_color = cc.rgbResult;
            //_Book->ReDraw(hWnd);
            GetClientRectExceptStatusBar(hWnd, &rect);
            InvalidateRect(hWnd, &rect, FALSE);
        }
    }
    
    return 0;
}

LRESULT OnRestoreDefault(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int lastrule = _header->chapter_rule.rule;
    if (IsZoomed(hWnd))
        SendMessage(hWnd, WM_SYSCOMMAND, SC_RESTORE, 0);
    KS_UnRegisterAllHotKey(hWnd);
    _Cache.default_header();
    UpdateLayoutForDpi(hWnd, &_header->rect, &_header->isDefault);
    UpdateFontForDpi(hWnd, &_header->font);
    SetTreeviewFont();
    if (GetDpiScaled(hWnd) == 1.0f)
    {
        SetWindowPos(hWnd, NULL,
            _header->rect.left, _header->rect.top,
            _header->rect.right - _header->rect.left,
            _header->rect.bottom - _header->rect.top,
            /*SWP_DRAWFRAME*/SWP_NOREDRAW);
    }
    if (lastrule != _header->chapter_rule.rule)
    {
        // reload book
        OnOpenItem(hWnd, 0, TRUE);
    }
    else
    {
        if (_Book)
            _Book->Reset(hWnd);
    }
    KS_RegisterAllHotKey(hWnd);
    ShowSysTray(hWnd, _header->show_systray);
    ShowInTaskbar(hWnd, !_header->hide_taskbar);
    if (_WndInfo.bHideBorder || _WndInfo.bFullScreen)
    {
        ResetLayerd(hWnd);
        OnDraw(hWnd);
    }
    else
    {
        ResetLayerd(hWnd);
        SetLayeredWindowAttributes(hWnd, 0, _header->alpha < MIN_ALPHA_VALUE ? MIN_ALPHA_VALUE : _header->alpha, LWA_ALPHA);
        InvalidateRect(hWnd, NULL, TRUE);
    }
    return 0;
}

LRESULT OnPaint(HWND hWnd, HDC hdc)
{
    RECT rc;
    HBRUSH hBrush = NULL;
    HDC memdc = NULL;
    HBITMAP hBmp = NULL;
    HFONT hFont = NULL;
    Bitmap *image;
    Graphics *g = NULL;
    Rect rect;
    UINT w,h;
    double scale;

    GetClientRectExceptStatusBar(hWnd, &rc);

    // memory dc
    memdc = CreateCompatibleDC(hdc);

    // load bg image
    image = LoadBGImage(rc.right-rc.left,rc.bottom-rc.top);
    if (image)
    {
        image->GetHBITMAP(Color(0, 0, 0, 0), &hBmp);
        SelectObject(memdc, hBmp);
    }
    else
    {
        hBmp = CreateCompatibleBitmap(hdc, rc.right-rc.left, rc.bottom-rc.top);
        SelectObject(memdc, hBmp);

        // set bg color
        hBrush = CreateSolidBrush(_header->bg_color);
        SelectObject(memdc, hBrush);
        FillRect(memdc, &rc, hBrush);
    }

    // set font
    hFont = CreateFontIndirect(&_header->font);
    SelectObject(memdc, hFont);
    SetTextColor(memdc, _header->font_color);
    
    SetBkMode(memdc, TRANSPARENT);

    if (_Book && !_Book->IsLoading() && _bShowText)
        _Book->DrawPage(memdc);
    if (_loading && _loading->enable && _bShowText)
    {
        g = new Graphics(memdc);
        w = (UINT)(rc.right - rc.left) > _loading->image->GetWidth() ? _loading->image->GetWidth() : (UINT)(rc.right - rc.left);
        h = (UINT)(rc.bottom - rc.top) > _loading->image->GetHeight() ? _loading->image->GetHeight() : (UINT)(rc.bottom - rc.top);
        scale = ((double)_loading->image->GetWidth())/_loading->image->GetHeight();
        if (((double)w)/h > scale)
        {
            // image is too high
            w = (int)(scale * h);
        }
        else
        {
            // image is too wide
            h = (int)(w / scale);
        }

        rect.X = (rc.right - rc.left - w)/2;
        rect.Y = (rc.bottom - rc.top - h)/2;
        rect.Width = w;
        rect.Height = h;
        g->SetInterpolationMode(InterpolationModeHighQualityBicubic);
        g->DrawImage(_loading->image, rect, 0, 0, _loading->image->GetWidth(), _loading->image->GetHeight(), UnitPixel);
    }

    BitBlt(hdc, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, memdc, rc.left, rc.top, SRCCOPY);

    DeleteObject(hBmp);
    DeleteObject(hFont);
    DeleteObject(hBrush);
    DeleteDC(memdc);
    if (g)
        delete g;
    UpdateProgess();
    UpdateTitle(hWnd);
    return 0;
}

void OnDraw(HWND hWnd)
{
    RECT rc;
    HBRUSH hBrush = NULL;
    HDC memdc = NULL;
    HBITMAP hBmp = NULL;
    HFONT hFont = NULL;
    Bitmap *image;
    Graphics *g = NULL;
    Rect rect;
    INT w,h;
    double scale;
    HDC hdcScreen = NULL;
    HBITMAP hTextBmp = NULL;
    static u32 s_bgColor = 0;
    static Bitmap *s_bgColorImage = NULL;
    static int s_width = 0;
    static int s_height = 0;

    GetClientRectExceptStatusBar(hWnd, &rc);

    w = rc.right-rc.left;
    h = rc.bottom-rc.top;

#if 1
    // load bg image
    image = LoadBGImage(w, h, _header->alpha);
    if (!image)
    {
        if (s_bgColorImage && s_width == w && s_height == h && s_bgColor == _header->bg_color)
        {
            image = s_bgColorImage;
        }
        else
        {
            if (s_bgColorImage)
                delete s_bgColorImage;
            s_bgColorImage = new Bitmap(w, h, PixelFormat32bppARGB);
            // set bg color
            Gdiplus::Color color;
            Gdiplus::ARGB argb;
            Gdiplus::Graphics *g = Gdiplus::Graphics::FromImage(s_bgColorImage);
            color.SetFromCOLORREF(_header->bg_color);
            argb = color.GetValue();
            argb &= 0x00FFFFFF;
            argb |= (((DWORD)_header->alpha) << 24);
            color.SetValue(argb);
            Gdiplus::SolidBrush brush_tr(color);
            g->FillRectangle(&brush_tr, 0, 0, w, h);
            delete g;
            image = s_bgColorImage;
        }
    }

    // memory dc
    hdcScreen = GetDC(NULL);
    memdc = CreateCompatibleDC(hdcScreen);
    image->GetHBITMAP(Color(0, 0, 0, 0), &hBmp);
    SelectObject(memdc, hBmp);
#else
    // memory dc
    hdcScreen = GetDC(NULL);
    memdc = CreateCompatibleDC(hdcScreen);

    // load bg image
    image = LoadBGImage(w, h, _header->alpha);
    if (image)
    {
        image->GetHBITMAP(Color(0, 0, 0, 0), &hBmp);
        SelectObject(memdc, hBmp);
    }
    else
    {
        hBmp = CreateCompatibleBitmap(hdcScreen, rc.right-rc.left, rc.bottom-rc.top);
        SelectObject(memdc, hBmp);

        // set bg color
        COLORREF color = _header->bg_color;
        hBrush = CreateSolidBrush(_header->bg_color);
        color &= 0x00FFFFFF;
        color |= (((DWORD)_header->alpha) << 24);
        SelectObject(memdc, hBrush);
        FillRect(memdc, &rc, hBrush);
    }
#endif

#if 0  // move to LoadBGImage() and Brush color
    // set bitmap alpha
    Gdiplus::Color color;
    Gdiplus::ARGB argb;
    for (INT i=0; i<h; i++)
    {
        for (INT j=0; j<w; j++)
        {
            image->GetPixel(j, i, &color);
            argb = color.GetValue();
            argb &= 0x00FFFFFF;
            argb |= (((DWORD)_header->alpha) << 24);
            color.SetValue(argb);
            image->SetPixel(j, i, color);
        }
    }
#endif

    // draw text
    if (_Book && !_Book->IsLoading() && _bShowText)
    {
        hFont = CreateFontIndirect(&_header->font);
        hTextBmp = CreateAlphaTextBitmap(hFont, _header->font_color, w, h);
    }
    if (_loading && _loading->enable && _bShowText)
    {
        g = new Graphics(memdc);
        w = (UINT)(rc.right - rc.left) > _loading->image->GetWidth() ? _loading->image->GetWidth() : (UINT)(rc.right - rc.left);
        h = (UINT)(rc.bottom - rc.top) > _loading->image->GetHeight() ? _loading->image->GetHeight() : (UINT)(rc.bottom - rc.top);
        scale = ((double)_loading->image->GetWidth())/_loading->image->GetHeight();
        if (((double)w)/h > scale)
        {
            // image is too high
            w = (int)(scale * h);
        }
        else
        {
            // image is too wide
            h = (int)(w / scale);
        }

        rect.X = (rc.right - rc.left - w)/2;
        rect.Y = (rc.bottom - rc.top - h)/2;
        rect.Width = w;
        rect.Height = h;
        g->SetInterpolationMode(InterpolationModeHighQualityBicubic);
        g->DrawImage(_loading->image, rect, 0, 0, _loading->image->GetWidth(), _loading->image->GetHeight(), UnitPixel);
    }

    // alpha blend text to backgroud image
    if (hTextBmp)
    {
        HDC hTempDC = CreateCompatibleDC(hdcScreen);
        HBITMAP hOldBMP = (HBITMAP)SelectObject(hTempDC, hTextBmp);
        if (hOldBMP)
        {
#if 1
            // Fill blend function and blend new text to window 
            BLENDFUNCTION bf;
            bf.BlendOp = AC_SRC_OVER;
            bf.BlendFlags = 0;
            bf.SourceConstantAlpha = 0xFF; 
            bf.AlphaFormat = AC_SRC_ALPHA;
            AlphaBlend(memdc, 0, 0, w, h, hTempDC, 0, 0, w, h, bf);
#else
            BitBlt(memdc, 0, 0, w, h, hTempDC, 0, 0, SRCCOPY);
#endif

            // Clean up 
            SelectObject(hTempDC, hOldBMP); 
            DeleteObject(hTextBmp); 
            DeleteDC(hTempDC); 
        }
    }

    // update layered
    RECT winRect;
    GetWindowRect(hWnd, &winRect);
    BLENDFUNCTION blend = { 0 };
    blend.BlendOp = AC_SRC_OVER;
    blend.BlendFlags = 0;
    blend.SourceConstantAlpha = 0xFF;
    blend.AlphaFormat = AC_SRC_ALPHA;
    POINT ptPos = {winRect.left, winRect.top};
    SIZE sizeWnd = {winRect.right - winRect.left, winRect.bottom - winRect.top};
    POINT ptSrc = {0, 0};
    BOOL ret = UpdateLayeredWindow(hWnd, hdcScreen, &ptPos, &sizeWnd, memdc, &ptSrc, 0, &blend, ULW_ALPHA);
    

    DeleteObject(hBmp);
    DeleteObject(hBrush);
    DeleteObject(hFont);
    DeleteDC(memdc);
    ReleaseDC(NULL, hdcScreen);
    if (g)
        delete g;
    UpdateProgess();
    UpdateTitle(hWnd);
    return;
}

void ResetLayerd(HWND hWnd)
{
    static BOOL s_bHideBorder = FALSE;
    static BOOL s_bFullScreen = FALSE;

    if (s_bHideBorder != _WndInfo.bHideBorder || s_bFullScreen != _WndInfo.bFullScreen)
    {
        DWORD exstyle = GetWindowLong(hWnd, GWL_EXSTYLE);
        exstyle &= ~WS_EX_LAYERED;
        SetWindowLong(hWnd, GWL_EXSTYLE, exstyle);
        exstyle |= WS_EX_LAYERED;
        SetWindowLong(hWnd, GWL_EXSTYLE, exstyle);
        s_bHideBorder = _WndInfo.bHideBorder;
        s_bFullScreen = _WndInfo.bFullScreen;
    }
}

LRESULT OnSize(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    RECT rc;

    GetClientRectExceptStatusBar(hWnd, &rc);
    if (_Book)
        _Book->SetRect(&rc);
    SendMessage(_WndInfo.hStatusBar, message, wParam, lParam);
    return 0;
}

LRESULT OnMove(HWND hWnd)
{
    // save rect to cache file when exit app
    return 0;
}

LRESULT OnHideWin(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    ShowHideWindow(hWnd);
    return 0;
}

LRESULT OnHideBorder(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    RECT rc;
    Book *pBook = NULL;

    if (_WndInfo.bFullScreen)
        return 0;

    GetClientRectExceptStatusBar(hWnd, &rc);
    ClientToScreen(hWnd, reinterpret_cast<POINT*>(&rc.left));
    ClientToScreen(hWnd, reinterpret_cast<POINT*>(&rc.right));

    // save status
    if (!_WndInfo.bHideBorder)
    {
        _WndInfo.hbStyle = (DWORD)GetWindowLong(hWnd, GWL_STYLE);
        _WndInfo.hbExStyle = (DWORD)GetWindowLong(hWnd, GWL_EXSTYLE);
        GetWindowRect(hWnd, &_WndInfo.hbRect);
        _WndInfo.hbRect.left = rc.left - _WndInfo.hbRect.left;
        _WndInfo.hbRect.right = _WndInfo.hbRect.right - rc.right;
        _WndInfo.hbRect.top = rc.top - _WndInfo.hbRect.top;
        _WndInfo.hbRect.bottom = _WndInfo.hbRect.bottom - rc.bottom;
    }
    else
    {
        rc.left = rc.left - _WndInfo.hbRect.left;
        rc.right = rc.right + _WndInfo.hbRect.right;
        rc.top = rc.top - _WndInfo.hbRect.top;
        rc.bottom = rc.bottom + _WndInfo.hbRect.bottom;
    }

    // set new status
    _WndInfo.bHideBorder = !_WndInfo.bHideBorder;
    pBook = _Book;_Book = NULL; // lock onsize
    // hide border
    if (_WndInfo.bHideBorder)
    {
        SetWindowLong(hWnd, GWL_STYLE, _WndInfo.hbStyle & (~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU)));
        SetWindowLong(hWnd, GWL_EXSTYLE, _WndInfo.hbExStyle & ~(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE));
        SetMenu(hWnd, NULL);
        ShowWindow(_WndInfo.hStatusBar, SW_HIDE);
        SetWindowPos(hWnd, NULL, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, /*SWP_DRAWFRAME*/SWP_NOREDRAW);
    }
    // show border
    else
    {
        SetWindowLong(hWnd, GWL_STYLE, _WndInfo.hbStyle);
        SetWindowLong(hWnd, GWL_EXSTYLE, _WndInfo.hbExStyle);
        SetMenu(hWnd, _WndInfo.hMenu);
        ShowWindow(_WndInfo.hStatusBar, SW_SHOW);
        SetWindowPos(hWnd, NULL, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, /*SWP_DRAWFRAME*/SWP_NOREDRAW);
    }

    _Book = pBook;

    // for alpha
    if (_WndInfo.bHideBorder || _WndInfo.bFullScreen)
    {
        ResetLayerd(hWnd);
        OnDraw(hWnd);
    }
    else
    {
        ResetLayerd(hWnd);
        SetLayeredWindowAttributes(hWnd, 0, _header->alpha < MIN_ALPHA_VALUE ? MIN_ALPHA_VALUE : _header->alpha, LWA_ALPHA);
        InvalidateRect(hWnd, NULL, TRUE);
    }
    return 0;
}

LRESULT OnFullScreen(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    MONITORINFO mi;
    RECT rc;

    if (!_WndInfo.bFullScreen)
    {
        _WndInfo.fsStyle = (DWORD)GetWindowLong(hWnd, GWL_STYLE);
        _WndInfo.fsExStyle = (DWORD)GetWindowLong(hWnd, GWL_EXSTYLE);
        GetWindowRect(hWnd, &_WndInfo.fsRect);
    }

    _WndInfo.bFullScreen = !_WndInfo.bFullScreen;

    if (_WndInfo.bFullScreen)
    {
        SetWindowLong(hWnd, GWL_STYLE, _WndInfo.fsStyle & (~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU)));
        SetWindowLong(hWnd, GWL_EXSTYLE, _WndInfo.fsExStyle & ~(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE));
        SetMenu(hWnd, NULL);
        ShowWindow(_WndInfo.hStatusBar, SW_HIDE);


        mi.cbSize = sizeof(mi);
        GetMonitorInfo(MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST),&mi);
        rc = mi.rcMonitor;
        SetWindowPos(hWnd, NULL, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, SWP_DRAWFRAME);
    }
    else
    {
        SetWindowLong(hWnd, GWL_STYLE, _WndInfo.fsStyle);
        SetWindowLong(hWnd, GWL_EXSTYLE, _WndInfo.fsExStyle);
        if (!_WndInfo.bHideBorder)
        {
            SetMenu(hWnd, _WndInfo.hMenu);
            ShowWindow(_WndInfo.hStatusBar, SW_SHOW);
        }
        SetWindowPos(hWnd, NULL, _WndInfo.fsRect.left, _WndInfo.fsRect.top, 
            _WndInfo.fsRect.right-_WndInfo.fsRect.left, _WndInfo.fsRect.bottom-_WndInfo.fsRect.top, SWP_DRAWFRAME);
    }

    // for alpha
    if (_WndInfo.bHideBorder || _WndInfo.bFullScreen)
    {
        ResetLayerd(hWnd);
        OnDraw(hWnd);
    }
    else
    {
        ResetLayerd(hWnd);
        SetLayeredWindowAttributes(hWnd, 0, _header->alpha < MIN_ALPHA_VALUE ? MIN_ALPHA_VALUE : _header->alpha, LWA_ALPHA);
        InvalidateRect(hWnd, NULL, TRUE);
    }
    return 0;
}

LRESULT OnTopmost(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static bool isTopmost = false;
    SetWindowPos(hWnd, isTopmost ? HWND_NOTOPMOST : HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE);
    isTopmost = !isTopmost;
    return 0;
}

LRESULT OnOpenFile(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (IsWindow(_hFindDlg))
    {
        DestroyWindow(_hFindDlg);
        _hFindDlg = NULL;
    }
    TCHAR szFileName[MAX_PATH] = {0};
    TCHAR szTitle[MAX_PATH] = {0};
    OPENFILENAME ofn = {0};
    ofn.lStructSize = sizeof(ofn);  
    ofn.hwndOwner = hWnd;  
    ofn.lpstrFilter = _T("Files (*.txt;*.epub)\0*.txt;*.epub\0\0");
    ofn.lpstrInitialDir = NULL;
    ofn.lpstrFile = szFileName; 
    ofn.nMaxFile = sizeof(szFileName)/sizeof(*szFileName);  
    ofn.nFilterIndex = 0;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;
    BOOL bSel = GetOpenFileName(&ofn);
    if (!bSel)
    {
        return 0;
    }

    OnOpenBook(hWnd, szFileName, FALSE);
    return 0;
}

LRESULT OnAddMark(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int ret;
    if (_Book && !_Book->IsLoading())
    {
        ret = MessageBox(hWnd, _T("确认添加书签？"), _T("书签"), MB_ICONINFORMATION|MB_YESNO);
        if (ret == IDYES)
        {
            if (_Cache.add_mark(_item, _item->index))
                OnUpdateBookMark(hWnd);
        }
    }
    return 0;
}

LRESULT OnAutoPage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (_Book && !_Book->IsLoading())
    {
        if (_IsAutoPage)
        {
            StopAutoPage(hWnd);
        }
        else
        {
            StartAutoPage(hWnd);
        }
    }
    return 0;
}

LRESULT OnSearch(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (_Book && !_Book->IsLoading())
        OnFindText(hWnd, message, wParam, lParam);
    return 0;
}

LRESULT OnJump(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (_Book && !_Book->IsLoading())
        DialogBox(hInst, MAKEINTRESOURCE(IDD_JUMP_PROGRESS), hWnd, JumpProgress);
    return 0;
}

int Editwordbreakproc(
    LPTSTR lpch,
    int ichCurrent,
    int cch,
    int code
    )
{
    if (WB_ISDELIMITER == code && cch == 0x0a)
    {
        return TRUE;
    }
    return FALSE;
}

LRESULT OnEditMode(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    TCHAR *text = NULL;

    if (EC_IsEditMode())
    {
        EC_LeaveEditMode();
        return 0;
    }

    if (_Book && !_Book->IsLoading())
    {
        if (_Book->GetCurPageText(&text))
        {
            if (_WndInfo.bHideBorder)
            {
                OnHideBorder(hWnd, message, wParam, lParam);
            }

            EC_EnterEditMode(hInst, hWnd, &_header->font, text, _Book->GetBookType() == book_epub);

            free(text);
        }
    }

    return 0;
}

LRESULT OnPageUp(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (_Book && !_Book->IsLoading())
        _Book->PageUp(hWnd);
    return 0;
}

LRESULT OnPageDown(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (_Book && !_Book->IsLoading())
        _Book->PageDown(hWnd);
    return 0;
}

LRESULT OnLineUp(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (_Book && !_Book->IsLoading())
        _Book->LineUp(hWnd, _header->wheel_speed);
    return 0;
}

LRESULT OnLineDown(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (_Book && !_Book->IsLoading())
        _Book->LineDown(hWnd, _header->wheel_speed);
    return 0;
}

LRESULT OnChapterUp(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (_Book && !_Book->IsLoading())
        _Book->JumpPrevChapter(hWnd);
    return 0;
}

LRESULT OnChapterDown(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (_Book && !_Book->IsLoading())
        _Book->JumpNextChapter(hWnd);
    return 0;
}

LRESULT OnDropFiles(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HDROP hDropInfo = (HDROP)wParam;
    UINT  nFileCount = ::DragQueryFile(hDropInfo, (UINT)-1, NULL, 0);
    TCHAR szFileName[MAX_PATH] = _T("");
    DWORD dwAttribute;
    TCHAR *ext = NULL;

    for (UINT i = 0; i < nFileCount; i++)
    {
        ::DragQueryFile(hDropInfo, i, szFileName, sizeof(szFileName));
        dwAttribute = ::GetFileAttributes(szFileName);

        if (dwAttribute & FILE_ATTRIBUTE_DIRECTORY)
        {          
            continue;
        }

        // check is txt file
        ext = PathFindExtension(szFileName);
        if (ext && _tcscmp(ext, _T(".txt")) && _tcscmp(ext, _T(".epub")))
        {
            continue;
        }

        // open file
        OnOpenBook(hWnd, szFileName, FALSE);

        // just open first file
        break;
    }

    ::DragFinish(hDropInfo);
    return 0;
}

LRESULT OnFindText(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static FINDREPLACE fr;       // common dialog box structure
    static TCHAR szFindWhat[80] = {0}; // buffer receiving string

    if (message == _uFindReplaceMsg)
    {
        // do search
        if (!_Book)
            return 0;
        int len = _tcslen(szFindWhat);
        if (fr.Flags & FR_DIALOGTERM)
        {
            // close dlg
            DestroyWindow(_hFindDlg);
            _hFindDlg = NULL;
        }
        else if (fr.Flags & FR_DOWN) // back search
        {
            for (int i=_item->index+1; i<_Book->GetTextLength()-len+1; i++)
            {
                if (0 == memcmp(szFindWhat, _Book->GetText()+i, len*sizeof(TCHAR)))
                {
                    _item->index = i;
                    _Book->Reset(hWnd);
                    break;
                }
            }
        }
        else // front search
        {
            for (int i=_item->index-1; i>=0; i--)
            {
                if (0 == memcmp(szFindWhat, _Book->GetText()+i, len*sizeof(TCHAR)))
                {
                    _item->index = i;
                    _Book->Reset(hWnd);
                    break;
                }
            }
        }
    }
    else
    {
        if (!IsWindow(_hFindDlg))
        {
            // Initialize FINDREPLACE
            ZeroMemory(&fr, sizeof(fr));
            fr.lStructSize = sizeof(fr);
            fr.hwndOwner = hWnd;
            fr.hInstance = hInst;
            fr.lpstrFindWhat = szFindWhat;
            fr.wFindWhatLen = 80;
            fr.Flags = FR_DOWN;

            _hFindDlg = FindText(&fr);
        }
    }

    return 0;
}

LRESULT OnUpdateChapters(HWND hWnd)
{
    chapters_t *chapters;
    chapters_t::iterator itor;
#if USING_MENU_CHAPTERS
    HMENU hMenuBar = NULL;
    HMENU hView = NULL;

    hMenuBar = GetMenu(hWnd);
    hView = CreateMenu();
    if (_Book)
    {
        chapters = _Book->GetChapters();
        for (itor = chapters->begin(); itor != chapters->end(); itor++)
        {
            AppendMenu(hView, MF_STRING, itor->first, itor->second.title.c_str());
        }
    }
    DeleteMenu(hMenuBar, 1, MF_BYPOSITION);
    InsertMenu(hMenuBar, 1, MF_BYPOSITION | MF_STRING | MF_POPUP, (UINT_PTR)hView, L"&View");
    DrawMenuBar(hWnd);
#else
    TVITEM tvi = {0};
    TVINSERTSTRUCT tvins = {0};
    HTREEITEM hPrev = (HTREEITEM)TVI_FIRST;
    
    TreeView_DeleteAllItems(_hTreeView);
    if (_Book)
    {
        tvi.mask = TVIF_TEXT /*| TVIF_IMAGE | TVIF_SELECTEDIMAGE */| TVIF_PARAM;

        chapters = _Book->GetChapters();
        for (itor = chapters->begin(); itor != chapters->end(); itor++)
        {
            tvi.pszText = (TCHAR *)itor->second.title.c_str(); 
            tvi.cchTextMax = sizeof(tvi.pszText)/sizeof(tvi.pszText[0]);
            tvi.lParam = (LPARAM)itor->first; 
            tvins.item = tvi; 
            tvins.hInsertAfter = hPrev;
            tvins.hParent = TVI_ROOT;

            // Add the item to the tree-view control. 
            hPrev = (HTREEITEM)SendMessage(_hTreeView, TVM_INSERTITEM, 0, (LPARAM)(LPTVINSERTSTRUCT)&tvins);
        }
    }
#endif

    return 0;
}

LRESULT OnUpdateBookMark(HWND hWnd)
{
    const int MAX_MARK_TEXT = 256;
    TVITEM tvi = {0};
    TVINSERTSTRUCT tvins = {0};
    HTREEITEM hPrev = (HTREEITEM)TVI_FIRST;
    int i;
    TCHAR szText[MAX_MARK_TEXT] = {0};
    int len;

    TreeView_DeleteAllItems(_hTreeMark);
    if (_Book && !_Book->IsLoading() && _item)
    {
        tvi.mask = TVIF_TEXT /*| TVIF_IMAGE | TVIF_SELECTEDIMAGE */| TVIF_PARAM;

        for (i=0; i<_item->mark_size; i++)
        {
            len = _item->mark[i] + (MAX_MARK_TEXT - 1) > _Book->GetTextLength() ? _Book->GetTextLength() - _item->index : (MAX_MARK_TEXT - 1);
            memcpy(szText, _Book->GetText()+_item->mark[i], sizeof(TCHAR)*len);
            szText[len] = 0;

            tvi.pszText = szText; 
            tvi.cchTextMax = MAX_MARK_TEXT;
            tvi.lParam = (LPARAM)i; 
            tvins.item = tvi; 
            tvins.hInsertAfter = hPrev;
            tvins.hParent = TVI_ROOT;

            // Add the item to the tree-view control. 

            hPrev = (HTREEITEM)SendMessage(_hTreeMark, TVM_INSERTITEM, 0, (LPARAM)(LPTVINSERTSTRUCT)&tvins);
        }
    }

    return 0;
}

LRESULT OnOpenBookResult(HWND hWnd, BOOL result)
{
    item_t *item = NULL;
    RECT rect;

    StopLoadingImage(hWnd);
    //EnableWindow(hWnd, TRUE);
    if (!result)
    {
        delete _Book;
        _Book = NULL;
        MessageBox(hWnd, _T("Open file failed."), _T("Error"), MB_OK|MB_ICONERROR);
        return FALSE;
    }

    // create new item
#if ENABLE_MD5
    item = _Cache.find_item(_Book->GetMd5(), _Book->GetFileName());
#else
    item = _Cache.find_item(_Book->GetFileName());
#endif
    if (NULL == item)
    {
#if ENABLE_MD5
        item = _Cache.new_item(_Book->GetMd5(), _Book->GetFileName());
#else
        item = _Cache.new_item(_Book->GetFileName());
#endif
        _header = _Cache.get_header(); // after realloc the header address has been changed
    }

    // open item
    _item = _Cache.open_item(item->id);

    // set param
    GetClientRectExceptStatusBar(hWnd, &rect);
#if ENABLE_TAG
    _Book->Setting(hWnd, &_item->index, &_header->line_gap, &_header->internal_border, _header->tags);
#else
    _Book->Setting(hWnd, &_item->index, &_header->line_gap, &_header->internal_border);
#endif
    _Book->SetRect(&rect);
    _Book->SetChapterRule(&(_header->chapter_rule));

    // update menu
    OnUpdateMenu(hWnd);

    // Update title
    UpdateTitle(hWnd);

    // update chapter
    PostMessage(hWnd, WM_UPDATE_CHAPTERS, 0, NULL);

    // repaint
    _Book->ReDraw(hWnd);

    return 0;
}

LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    LPMOUSEHOOKSTRUCT mh = NULL;
    RECT rect;

    if (nCode == HC_ACTION)
    {
        switch (wParam)
        {
        case WM_MOUSEMOVE:
            mh = (LPMOUSEHOOKSTRUCT)lParam;
            GetWindowRect(_hWnd, &rect);
            if (PtInRect(&rect, mh->pt))
            {
                if (!IsWindowVisible(_hWnd))
                    ShowWindow(_hWnd, SW_SHOW);
            }
            else
            {
                if (IsWindowVisible(_hWnd))
                    ShowWindow(_hWnd, SW_HIDE);
            }
            break;
        default:
            break;
        }
    }
    
    return CallNextHookEx(_hMouseHook, nCode, wParam, lParam);
}

void ShowHideWindow(HWND hWnd)
{
    // show or hide window
    BOOL isShow = IsWindowVisible(hWnd);
    ShowWindow(hWnd, isShow ? SW_HIDE : SW_SHOW);
    if (!isShow)
    {
        if (IsIconic(hWnd))
            ShowWindow(hWnd, SW_RESTORE);
        SetForegroundWindow(hWnd);
    }
}

void OnOpenBook(HWND hWnd, TCHAR *filename, BOOL forced)
{
    item_t *item = NULL;
    TCHAR *ext = NULL;
#if ENABLE_MD5
    u128_t md5;
    char *data = NULL;
#else
    FILE *fp = NULL;
#endif
    int size = 0;
    TCHAR szFileName[MAX_PATH] = {0};

    _tcscpy(szFileName, filename);
    ext = PathFindExtension(szFileName);
    if (_tcscmp(ext, _T(".txt")) && _tcscmp(ext, _T(".epub")))
    {
        MessageBox(hWnd, _T("Unknown file format."), _T("Error"), MB_OK|MB_ICONERROR);
        return;
    }

#if ENABLE_MD5
    if (!Book::CalcMd5(szFileName, &md5, &data, &size))
    {
        MessageBox(hWnd, _T("Open file failed."), _T("Error"), MB_OK|MB_ICONERROR);
        return;
    }
#else
    fp = _tfopen(szFileName, _T("rb"));
    if (!fp)
    {
        MessageBox(hWnd, _T("Open file failed."), _T("Error"), MB_OK|MB_ICONERROR);
        return;
    }
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fclose(fp);
#endif

    if (size == 0)
    {
#if ENABLE_MD5
        free(data);
#endif
        MessageBox(hWnd, _T("Open file failed.\r\nThis is a empty file."), _T("Error"), MB_OK|MB_ICONERROR);
        return;
    }

    // check md5, is already exist
#if ENABLE_MD5
    item = _Cache.find_item(&md5, szFileName);
#else
    item = _Cache.find_item(szFileName);
#endif
    if (item)
    {
        if (!forced)
        {
            if (_item && item->id == _item->id && !_Book->IsLoading()) // current is opened
            {
#if ENABLE_MD5
                free(data);
#endif
                OnUpdateMenu(hWnd);
                return;
            }
        }
    }

    if (_Book)
    {
        delete _Book;
        _Book = NULL;
    }

    if (_tcscmp(ext, _T(".txt")) == 0)
    {
#if ENABLE_MD5
        free(data);
        data = NULL;
#endif
        _Book = new TextBook;
#if ENABLE_MD5
        _Book->SetMd5(&md5);
#endif
        _Book->SetFileName(szFileName);
        _Book->SetChapterRule(&(_header->chapter_rule));
        _Book->OpenBook(NULL, size, hWnd);
    }
    else
    {
#if ENABLE_MD5
        free(data);
        data = NULL;
#endif
        _Book = new EpubBook;
#if ENABLE_MD5
        _Book->SetMd5(&md5);
#endif
        _Book->SetFileName(szFileName);
        _Book->OpenBook(hWnd);
    }
    
    //EnableWindow(hWnd, FALSE);
    PlayLoadingImage(hWnd);
}

UINT GetCacheVersion(void)
{
    std::wstring version;
    UINT ver = 0;

    version = _Upgrade.GetApplicationVersion();

    unsigned int v1 = 0, v2 = 0, v3 = 0, v4 = 0;
    swscanf(version.c_str(), L"%d.%d.%d.%d", &v1, &v2, &v3, &v4);    

    ver = v1 << 24 | v2 << 16 | v3 << 8 | v4;
    return ver;
}

BOOL Init(void)
{
    if (!_Cache.init())
    {
        MessageBox(NULL, _T("Init Cache fail."), _T("Error"), MB_OK);
        return FALSE;
    }

    _header = _Cache.get_header();

    // delete not exist items
    std::vector<int> delVec;
    for (int i=0; i<_header->item_count; i++)
    {
        item_t* item = _Cache.get_item(i);
        if (!PathFileExists(item->file_name))
        {
            delVec.push_back(i);
        }
    }
    for (size_t i=0; i<delVec.size(); i++)
    {
        _Cache.delete_item(delVec[i]);
    }

    return TRUE;
}

void Exit(void)
{
    if (_Book)
    {
        delete _Book;
        _Book = NULL;
    }

    if (!_Cache.exit())
    {
        MessageBox(NULL, _T("Save Cache fail."), _T("Error"), MB_OK);
    }
}

void UpdateProgess(void)
{
    TCHAR progress[256] = {0};
    double dprog = 0.0;
    int nprog = 0;

    if (EC_IsEditMode())
    {
        SendMessage(_WndInfo.hStatusBar, SB_SETTEXT, (WPARAM)0, (LPARAM)_T("当前为【编辑模式】，再次使用快捷键可退出"));
        return;
    }

    if (_item && _Book && !_Book->IsLoading())
    {
        dprog = (double)_Book->GetProgress();
        nprog = (int)(dprog * 100);
        dprog = (double)nprog / 100.0;
#if 0
        _stprintf(progress, _T("Progress: %.2f%%"), dprog);
#else
        _stprintf(progress, _T("Progress: %.2f%%    ( %d / %d )"), dprog, _item->index + _Book->GetCurPageSize(), _Book->GetTextLength());
#endif
        SendMessage(_WndInfo.hStatusBar, SB_SETTEXT, (WPARAM)0, (LPARAM)progress);
    }
    else
    {
        SendMessage(_WndInfo.hStatusBar, SB_SETTEXT, (WPARAM)0, (LPARAM)_T(""));
    }
}

void UpdateTitle(HWND hWnd)
{
    TCHAR szOldTitle[MAX_LOADSTRING + MAX_PATH + MAX_PATH] = { 0 };
    TCHAR szNewTitle[MAX_LOADSTRING + MAX_PATH + MAX_PATH] = { 0 };
    TCHAR szFileName[MAX_PATH] = { 0 };
    TCHAR szChapter[MAX_PATH] = { 0 };

    if (!_item || !_Book || _Book->IsLoading())
        return;

    memcpy(szFileName, _item->file_name, sizeof(szFileName));
    PathRemoveExtension(szFileName);

    if (_Book && !_Book->IsLoading())
    {
        if (_Book->GetChapterTitle(szChapter, MAX_PATH))
        {
            _stprintf(szNewTitle, _T("%s - %s - %s"), szTitle, PathFindFileName(szFileName), szChapter);
        }
    }
    
    if (!szNewTitle[0])
    {
        _stprintf(szNewTitle, _T("%s - %s"), szTitle, PathFindFileName(szFileName));
    }

    GetWindowText(hWnd, szOldTitle, MAX_LOADSTRING + MAX_PATH + MAX_PATH - 1);
    if (_tcscmp(szOldTitle, szNewTitle))
        SetWindowText(hWnd, szNewTitle);
}

void RemoveMenuById(HMENU hMenu, BOOL separator, int id)
{
    HMENU hSubMenu;
    int count;
    int i,j;
    int menuid;

    if (!hMenu)
        return;

    count = GetMenuItemCount(hMenu);
    for (i=0; i<count; i++)
    {
        hSubMenu = GetSubMenu(hMenu, i);
        if (hSubMenu)
            RemoveMenuById(hSubMenu, separator, id);
        menuid = GetMenuItemID(hMenu, i);
        if (id == menuid)
        {
            if (separator)
            {
                j = i+1;
                RemoveMenu(hMenu, j, MF_BYPOSITION);
            }
            RemoveMenu(hMenu, id, MF_BYCOMMAND);
            return;
        }
    }
}

void RemoveMenus(HWND hWnd, BOOL redraw)
{
    HMENU hMenu;

    hMenu = GetMenu(hWnd);

#if !ENABLE_NETWORK
    RemoveMenuById(hMenu, TRUE, IDM_PROXY);
#endif

#if !ENABLE_TAG
    RemoveMenuById(hMenu, FALSE, IDM_TAGSET);
#endif

    if (redraw)
        DrawMenuBar(hWnd);
}

BOOL GetClientRectExceptStatusBar(HWND hWnd, RECT* rc)
{
    RECT rect;
    GetClientRect(_WndInfo.hStatusBar, &rect);
    GetClientRect(hWnd, rc);
    if (IsWindowVisible(_WndInfo.hStatusBar))
    {
        rc->bottom -= rect.bottom;
    }
    return TRUE;
}

void WheelSpeedInit(HWND hDlg)
{
    int i;
    HWND hWnd = NULL;
    TCHAR buf[8] = {0};

    hWnd = GetDlgItem(hDlg, IDC_COMBO_SPEED);
    if (_Book)
    {
        for (i=1; i<=_Book->GetOnePageLineCount(); i++)
        {
            _itot(i, buf, 10);
            SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)buf);
        }
    }
    else
    {
        for (i=1; i<=_header->wheel_speed; i++)
        {
            _itot(i, buf, 10);
            SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)buf);
        }
    }
    SendMessage(hWnd, CB_SETCURSEL, _header->wheel_speed - 1, NULL);
}

Bitmap* LoadBGImage(int w, int h, BYTE alpha)
{
    static Bitmap *bgimg = NULL;
    static TCHAR curfile[MAX_PATH] = {0};
    static int curWidth = 0;
    static int curHeight = 0;
    static int curmode = Stretch;
    static BYTE s_alpha = 0xFF;
    Bitmap *image = NULL;
    Graphics *graphics = NULL;
    ImageAttributes ImgAtt;
    RectF rcDrawRect;

    if (!_header->bg_image.enable || !_header->bg_image.file_name[0])
    {
        return NULL;
    }

    if (_tcscmp(_header->bg_image.file_name, curfile) == 0 && curWidth == w && curHeight == h 
        && curmode == _header->bg_image.mode && bgimg && alpha == s_alpha)
    {
        return bgimg;
    }

    if (!FileExists(_header->bg_image.file_name))
    {
        _header->bg_image.file_name[0] = 0;
        return NULL;
    }
    
    if (bgimg)
    {
        delete bgimg;
        bgimg = NULL;
    }
    _tcscpy(curfile, _header->bg_image.file_name);
    curWidth = w;
    curHeight = h;
    curmode = _header->bg_image.mode;
    s_alpha = alpha;
    
    // load image file
    image = Bitmap::FromFile(curfile);
    if (image == NULL)
        return NULL;
    if (Gdiplus::Ok != image->GetLastStatus())
    {
        delete image;
        image = NULL;
        return NULL;
    }

    // create bg image
    bgimg = new Bitmap(curWidth, curHeight, PixelFormat32bppARGB);
    rcDrawRect.X=0.0;
    rcDrawRect.Y=0.0;
    rcDrawRect.Width=(float)curWidth;
    rcDrawRect.Height=(float)curHeight;
    graphics = Graphics::FromImage(bgimg);
    graphics->SetInterpolationMode(InterpolationModeHighQualityBicubic);

    ColorMatrix colorMatrix = {
        1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, alpha/255.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f, 1.0f
    };
    ImgAtt.SetColorMatrix(&colorMatrix, ColorMatrixFlagsDefault, ColorAdjustTypeBitmap);
    switch (curmode)
    {
    case Stretch:
        graphics->DrawImage(image,rcDrawRect,0.0,0.0,(float)image->GetWidth(),(float)image->GetHeight(),UnitPixel,&ImgAtt);
        break;
    case Tile:
        ImgAtt.SetWrapMode(WrapModeTile);
        graphics->DrawImage(image,rcDrawRect,0.0,0.0,(float)curWidth,(float)curHeight,UnitPixel,&ImgAtt);
        break;
    case TileFlip:
        ImgAtt.SetWrapMode(WrapModeTileFlipXY);
        graphics->DrawImage(image,rcDrawRect,0.0,0.0,(float)curWidth,(float)curHeight,UnitPixel,&ImgAtt);
        break;
    default:
        graphics->DrawImage(image,rcDrawRect,0.0,0.0,(float)image->GetWidth(),(float)image->GetHeight(),UnitPixel,&ImgAtt);
        break;
    }

    delete image;
    delete graphics;
    return bgimg;
}

BOOL FileExists(TCHAR *file)
{
    DWORD dwAttrib = GetFileAttributes(file);
    return (dwAttrib != INVALID_FILE_ATTRIBUTES && 
        !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

void StartAutoPage(HWND hWnd)
{
    if (_item && !_IsAutoPage)
    {
        SetTimer(hWnd, IDT_TIMER_PAGE, _header->uElapse, NULL);
        _IsAutoPage = TRUE;
    }
}

void StopAutoPage(HWND hWnd)
{
    if (_IsAutoPage)
    {
        KillTimer(hWnd, IDT_TIMER_PAGE);
        _IsAutoPage = FALSE;
    }
}

void PauseAutoPage(HWND hWnd)
{
    if (_IsAutoPage)
    {
        KillTimer(hWnd, IDT_TIMER_PAGE);
    }
}

void ResumeAutoPage(HWND hWnd)
{
    if (_item && _IsAutoPage)
    {
        SetTimer(hWnd, IDT_TIMER_PAGE, _header->uElapse, NULL);
    }
}

#if ENABLE_NETWORK
void CheckUpgrade(HWND hWnd)
{
    // set proxy & check upgrade
    _Upgrade.SetProxy(&_header->proxy);
    _Upgrade.SetIngoreVersion(_header->ingore_version);
    _Upgrade.Check(UpgradeCallback, hWnd);
    SetTimer(hWnd, IDT_TIMER_UPGRADE, 24 * 60 * 60 * 1000 /*one day*/, NULL);
}

bool UpgradeCallback(void *param, json_item_data_t *item)
{
    HWND hWnd = (HWND)param;

    PostMessage(hWnd, WM_NEW_VERSION, 0, NULL);
    return true;
}
#endif

bool PlayLoadingImage(HWND hWnd)
{
    UINT count;
    GUID *pDimensionIDs = NULL;
    bool ret = false;
    WCHAR strGuid[39];
    UINT size;
    GUID Guid = FrameDimensionTime;
    RECT rect;

    if (!_loading)
    {
        // create loading data
        _loading = (loading_data_t *)malloc(sizeof(loading_data_t));
        memset(_loading, 0, sizeof(loading_data_t));

        // load image from resource
        {
            HRSRC hResource;
            DWORD imageSize;
            const void* pResourceData = NULL;
            void * buffer;
            IStream* pStream = NULL;

            hResource = ::FindResource(hInst, MAKEINTRESOURCE(IDR_GIF_LOADING), _T("GIF"));
            if (!hResource)
                return false;

            imageSize = ::SizeofResource(hInst, hResource);
            if (!imageSize)
                return false;

            pResourceData = ::LockResource(::LoadResource(hInst, hResource));
            if (!pResourceData)
                return false;

            _loading->hMemory = ::GlobalAlloc(GMEM_MOVEABLE, imageSize);
            if (!_loading->hMemory)
                return false;

            buffer = ::GlobalLock(_loading->hMemory);
            if (!buffer)
            {
                ::GlobalFree(_loading->hMemory);
                return false;
            }

            CopyMemory(buffer, pResourceData, imageSize);
            if (::CreateStreamOnHGlobal(_loading->hMemory, FALSE, &pStream) == S_OK)
            {
                _loading->image = Gdiplus::Bitmap::FromStream(pStream);
                pStream->Release();
            }
            ::GlobalUnlock(_loading->hMemory);
            //::GlobalFree(_loading->hMemory);
        }

        if (!_loading->image)
        {
            //::GlobalFree(_loading->hMemory);
            //free(_loading);
            //_loading = NULL;
            return false;
        }
        if (Gdiplus::Ok != _loading->image->GetLastStatus())
        {
            //::GlobalFree(_loading->hMemory);
            //delete _loading->image;
            //free(_loading);
            //_loading = NULL;
            return false;
        }
        count = _loading->image->GetFrameDimensionsCount();
        pDimensionIDs = (GUID*)malloc(sizeof(GUID)*count);
        _loading->image->GetFrameDimensionsList(pDimensionIDs, count);
        StringFromGUID2(pDimensionIDs[0], strGuid, 39);
        _loading->frameCount = _loading->image->GetFrameCount(&pDimensionIDs[0]);
        free(pDimensionIDs);
        size = _loading->image->GetPropertyItemSize(PropertyTagFrameDelay);
        _loading->item = (PropertyItem*)malloc(size);
        _loading->image->GetPropertyItem(PropertyTagFrameDelay, size, _loading->item);
    }

    _loading->enable = TRUE;
    _loading->frameIndex = 0;
    Guid = FrameDimensionTime;
    _loading->image->SelectActiveFrame(&Guid, _loading->frameIndex);

    SetTimer(hWnd, IDT_TIMER_LOADING, ((UINT*)_loading->item[0].value)[_loading->frameIndex] * 10, NULL);
    _loading->frameIndex = (++ _loading->frameIndex) % _loading->frameCount;
    GetClientRectExceptStatusBar(hWnd, &rect);
    InvalidateRect(hWnd, &rect, FALSE);
    return true;
}

bool StopLoadingImage(HWND hWnd)
{
    RECT rect;

    if (!_loading)
        return false;
    _loading->enable = FALSE;
    KillTimer(hWnd, IDT_TIMER_LOADING);
    GetClientRectExceptStatusBar(hWnd, &rect);
    InvalidateRect(hWnd, &rect, FALSE);
    return true;
}

ULONGLONG GetDllVersion(LPCTSTR lpszDllName)
{
    ULONGLONG ullVersion = 0;
    HINSTANCE hinstDll;
    hinstDll = LoadLibrary(lpszDllName);
    if(hinstDll)
    {
        DLLGETVERSIONPROC pDllGetVersion;
        pDllGetVersion = (DLLGETVERSIONPROC)GetProcAddress(hinstDll, "DllGetVersion");
        if(pDllGetVersion)
        {
            DLLVERSIONINFO dvi;
            HRESULT hr;
            ZeroMemory(&dvi, sizeof(dvi));
            dvi.cbSize = sizeof(dvi);
            hr = (*pDllGetVersion)(&dvi);
            if(SUCCEEDED(hr))
                ullVersion = MAKEDLLVERULL(dvi.dwMajorVersion, dvi.dwMinorVersion,0,0);
        }
        FreeLibrary(hinstDll);
    }
    return ullVersion;
}

BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam)
{
    DWORD ProcessId;
    TCHAR szProcessName[MAX_PATH] = _T("<unknown>");
    int *flag = (int*)lParam;
    
    GetWindowThreadProcessId(hWnd, &ProcessId);
    
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, ProcessId);
    if (hProcess)
    {
        HMODULE hMod;
        DWORD cbNeeded;

        if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded))
        {
            GetModuleBaseName(hProcess, hMod, szProcessName, sizeof(szProcessName)/sizeof(TCHAR));
            if (_tcscmp(_T("Reader.exe"), szProcessName) == 0)
            {
                ShowWindow(hWnd, SW_SHOW);
                SetForegroundWindow(hWnd);
                (*flag) += 1;
                return (*flag) > 1 ? FALSE : TRUE;
            }
        }
    }
    return TRUE;
}

void ShowInTaskbar(HWND hWnd, BOOL bShow)
{
    HRESULT hr;
    ITaskbarList* pTaskbarList;
    hr = CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, IID_ITaskbarList, (void**)&pTaskbarList );

    if(SUCCEEDED(hr))
    {
        pTaskbarList->HrInit();
        if(bShow)
            pTaskbarList->AddTab(hWnd);
        else
            pTaskbarList->DeleteTab(hWnd);

        pTaskbarList->Release();
    }
}

void ShowSysTray(HWND hWnd, BOOL bShow)
{
    if (bShow)
    {
        if (!_nid.hWnd)
        {
            // create a systray
            ULONGLONG ullVersion = GetDllVersion(_T("Shell32.dll"));
            ZeroMemory(&_nid, sizeof(NOTIFYICONDATA));
            if(ullVersion >= MAKEDLLVERULL(6,0,0,0))
                _nid.cbSize = sizeof(NOTIFYICONDATA);
            else if(ullVersion >= MAKEDLLVERULL(5,0,0,0))
                _nid.cbSize = NOTIFYICONDATA_V2_SIZE;
            else
                _nid.cbSize = NOTIFYICONDATA_V1_SIZE;
            _nid.hWnd = hWnd;
            _nid.uID = IDI_SYSTRAY;
            _nid.uCallbackMessage = WM_SYSTRAY;
            _nid.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_BOOK));
            _tcscpy(_nid.szTip, szTitle);
        }
        if (_nid.uFlags == 0)
        {
            _nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE | NIF_INFO;
            Shell_NotifyIcon(NIM_ADD, &_nid);
        }
    }
    else
    {
        if (_nid.uFlags)
        {
            _nid.uFlags = 0;
            Shell_NotifyIcon(NIM_DELETE, &_nid);
        }
    }
}

HBITMAP CreateAlphaTextBitmap(HFONT inFont, COLORREF inColour, int width, int height)
{
    // Create DC and select font into it
    HDC hTextDC = CreateCompatibleDC(NULL);
    HFONT hOldFont = (HFONT)SelectObject(hTextDC, inFont);
    HBITMAP hDIB = NULL;
    BITMAPINFOHEADER BMIH;
    void *pvBits = NULL;
    HBITMAP hOldBMP = NULL;
    BYTE* DataPtr = NULL;
    BYTE FillR,FillG,FillB,ThisA;
    int x,y;

    // Specify DIB setup
    memset(&BMIH, 0x0, sizeof(BITMAPINFOHEADER));
    BMIH.biSize = sizeof(BMIH);
    BMIH.biWidth = width;
    BMIH.biHeight = height;
    BMIH.biPlanes = 1;
    BMIH.biBitCount = 32;
    BMIH.biCompression = BI_RGB;

    // Create and select DIB into DC
    hDIB = CreateDIBSection(hTextDC, (LPBITMAPINFO)&BMIH, 0, (LPVOID*)&pvBits, NULL, 0); 
    hOldBMP = (HBITMAP)SelectObject(hTextDC, hDIB);
    if (hOldBMP)
    {
        // Set up DC properties 
        SetTextColor(hTextDC, 0x00FFFFFF);
        SetBkColor(hTextDC, 0x00000000);
        SetBkMode(hTextDC, OPAQUE);

        // Draw text to buffer
        _Book->DrawPage(hTextDC);
        if (!_Book->IsCoverPage())
        {
            DataPtr = (BYTE*)pvBits;
            FillR = GetRValue(inColour);
            FillG = GetGValue(inColour);
            FillB = GetBValue(inColour);
            for (y = 0; y < BMIH.biHeight; y++)
            { 
                for (x = 0; x < BMIH.biWidth; x++)
                { 
                    ThisA = *DataPtr; // Move alpha and pre-multiply with RGB 
                    *DataPtr++ = (FillB * ThisA) >> 8; 
                    *DataPtr++ = (FillG * ThisA) >> 8; 
                    *DataPtr++ = (FillR * ThisA) >> 8;
#if 0
                    *DataPtr++ = ThisA; // Set Alpha 
#else
                    *DataPtr++ = ThisA > _textAlpha ? _textAlpha : ThisA;
#endif
                }
            }
        }

        // De-select bitmap
        SelectObject(hTextDC, hOldBMP);
    }

    // De-select font and destroy temp DC
    SelectObject(hTextDC, hOldFont);
    DeleteDC(hTextDC);

    // Return DIBSection 
    return hDIB;
}

void SetTreeviewFont()
{
    static HFONT s_hFont = NULL;

    if (s_hFont)
    {
        DeleteObject(s_hFont);
        s_hFont = NULL;
    }

    if (_header->meun_font)
    {
        s_hFont = CreateFontIndirect(&_header->font);
        int height = abs((int)(_header->font.lfHeight * 1.5f));
        SendMessage(_hTreeView, WM_SETFONT, (WPARAM)s_hFont, NULL);
        SendMessage(_hTreeView, TVM_SETITEMHEIGHT, height, NULL);
        SendMessage(_hTreeMark, WM_SETFONT, (WPARAM)s_hFont, NULL);
        SendMessage(_hTreeMark, TVM_SETITEMHEIGHT, height, NULL);
    }
    else
    {
        NONCLIENTMETRICS theMetrics = {0};
        theMetrics.cbSize = sizeof(NONCLIENTMETRICS);
        if (Utils::isWindowsXP())
            theMetrics.cbSize = sizeof(NONCLIENTMETRICS) - sizeof(theMetrics.iPaddedBorderWidth);
        SystemParametersInfo(SPI_GETNONCLIENTMETRICS,sizeof(NONCLIENTMETRICS), (PVOID) &theMetrics,0);
        theMetrics.iMenuHeight = abs((int)(theMetrics.iMenuHeight * 1.25f)); // 1.25 line height
        if (theMetrics.iMenuHeight == 0)
        {
            theMetrics.iMenuHeight = 20;
        }
        s_hFont = CreateFontIndirect(&(theMetrics.lfMenuFont));
        SendMessage(_hTreeView, WM_SETFONT, (WPARAM)s_hFont, NULL);
        SendMessage(_hTreeView, TVM_SETITEMHEIGHT, theMetrics.iMenuHeight, NULL);
        SendMessage(_hTreeMark, WM_SETFONT, (WPARAM)s_hFont, NULL);
        SendMessage(_hTreeMark, TVM_SETITEMHEIGHT, theMetrics.iMenuHeight, NULL);
    }
}
