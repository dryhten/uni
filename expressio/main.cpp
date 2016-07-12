/*******************************************************************************

	Expressio
	Mihail Dunaev,
	Politehnica University of Bucharest
	14 Sep 2014

	Expressio - A system that can identify the user's predominant emotion
	            (from the 6 basic ones), analyzing frames received from a
				video camera.

	main.cpp  - main procedure; GUI code

*******************************************************************************/

#include <Windows.h>
#include <WindowsX.h>
#include <commctrl.h>
#include "resource1.h"
#include "pxcface.h"
#include "pxcsmartptr.h"

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/contrib/contrib.hpp>

#include "expressio.h"
#include "main.h"

#define ID_DEVICEX   21000
#define ID_LANDMARKX 23000

#define IDC_STATUS			10000
#define IDC_STATUS_TRAIN	10001

HINSTANCE   g_hInst = 0;
PXCSession *g_session = 0;
pxcCHAR     g_file[1024] = {0};

/* panel bitmaps */
HBITMAP     g_bitmap_main = 0;
HBITMAP     g_bitmap_main_mini = 0;
HBITMAP     g_bitmap_train = 0;
HBITMAP     g_bitmap_train_mini = 0;
HBITMAP     g_bitmap_emorec = 0;

/* none gesture */
HBITMAP     g_none = 0;

/* threading control */
volatile bool g_running = false;
volatile bool g_stop = true;

/* control layout */
int g_controls_main[] = {IDC_SCALE, IDC_MIRROR, ID_START, ID_STOP, IDC_TEXT_EMOTIONS,
	IDC_HAPPY, IDC_SURPRISE, IDC_FEAR, IDC_ANGER, IDC_DISGUST, IDC_SAD,
	IDC_PRO_HAPPY, IDC_PRO_SURPRISE, IDC_PRO_FEAR, IDC_PRO_ANGER, IDC_PRO_DISGUST, IDC_PRO_SAD,
	IDC_DELAY, IDC_DELAY_SLIDER, IDC_CHECK_EMO, IDC_CHECK_FACE, IDC_CHECK_EYES, IDC_CHECK_NOSE,
	IDC_CHECK_MOUTH, IDC_CHECK_UPFACE, IDC_CHECK_LOFACE, IDC_BUTTON_TRAIN, IDC_BUTTON_EMOREC,
	IDC_CROP, IDC_TEXT_GENDER};
RECT g_layout_main[3+sizeof(g_controls_main)/sizeof(g_controls_main[0])];

int g_controls_train[] = {IDC_PANEL_TRAIN, IDC_CROP_TRAIN, IDC_BUTTON_SNAPSHOT, ID_DONE, IDCANCEL,
	ID_TEXT_GENDER_TRAIN, IDC_TEXT_EMO_TRAIN, IDC_RADIO_MALE, IDC_RADIO_FEMALE, IDC_RADIO_HAPPY,
	IDC_RADIO_HAPPY, IDC_RADIO_SURPRISE, IDC_RADIO_FEAR, IDC_RADIO_ANGER, IDC_RADIO_DISGUST,
	IDC_RADIO_SAD};
RECT g_layout_train[3+sizeof(g_controls_train)/sizeof(g_controls_train[0])];

int g_controls_emorec[] = {IDC_PANEL_EMOREC, ID_START_EMOREC, ID_STOP_EMOREC,
	IDC_DELAY_SLIDER_EMOREC, IDC_DELAY_EMOREC, IDC_TEXT_EMOTIONS_EMOREC,
	IDC_HAPPY_EMOREC, IDC_SURPRISE_EMOREC, IDC_FEAR_EMOREC, IDC_ANGER_EMOREC, IDC_DISGUST_EMOREC,
	IDC_SAD_EMOREC, IDC_PRO_HAPPY, IDC_PRO_SURPRISE, IDC_PRO_FEAR,
	IDC_PRO_ANGER, IDC_PRO_DISGUST, IDC_PRO_SAD, IDC_TEXT_GENDER,
	IDC_VIEW_EMOREC, IDC_SAVE_EMOREC};
RECT g_layout_emorec[3+sizeof(g_controls_emorec)/sizeof(g_controls_emorec[0])];

extern bool take_snapshot, emorec_save, jaffe_db, expressio_db, dynamic_db;
extern long long delay, delay_emorec;
extern std::vector<int> emorec_emotions;

int current_emotion = -1;
HWND h_main, h_train, h_emorec, h_current;

#pragma warning(disable:4706) /* assignment within conditional */
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPTSTR, int) {

	RECT screen_rect;
	HWND h_status_bar, h_status_train, h_screen;
	int ret, wx_mw, wy_mw, wx_tw, wy_tw, width_mw, height_mw, width_tw, height_tw;

	/* use this obsolete function to register & init window; should try with Ex */
	InitCommonControls();

	/* save instance to a global variable */
	g_hInst = hInstance;

	/* create pxc session */
	pxcStatus sts = PXCSession_Create(&g_session);
	if (sts < PXC_STATUS_NO_ERROR) {
        MessageBoxW(0,L"Failed to create an SDK session",L"Error",MB_ICONEXCLAMATION|MB_OK);
        return -1;
    }

	/* create main dialog window */
    h_main = CreateDialogW(hInstance,MAKEINTRESOURCE(IDD_MAINFRAME),0,main_window_handler);
    if (!h_main)  {
        MessageBoxW(0,L"Failed to create main window",L"Error",MB_ICONEXCLAMATION|MB_OK);
        return -1;
    }

	/* main window status bar */
	h_status_bar = CreateStatusWindow(WS_CHILD|WS_VISIBLE,L"OK",h_main,IDC_STATUS);
	if (!h_status_bar) {
        MessageBoxW(0,L"Failed to create main status bar",L"Error",MB_ICONEXCLAMATION|MB_OK);
        return -1;
	}

	/* create train dialog window */
    h_train = CreateDialogW(hInstance,MAKEINTRESOURCE(IDD_TRAIN),0,train_dialog_handler);
    if (!h_train)  {
        MessageBoxW(0,L"Failed to create train window",L"Error",MB_ICONEXCLAMATION|MB_OK);
        return -1;
    }

	/* train window status bar */
	h_status_train = CreateStatusWindow(WS_CHILD|WS_VISIBLE,L"OK",h_train,IDC_STATUS_TRAIN);
	if (!h_status_train) {
        MessageBoxW(0,L"Failed to create a status bar",L"Error",MB_ICONEXCLAMATION|MB_OK);
        return -1;
	}

	/* emorec window */
	h_emorec = CreateDialogW(hInstance,MAKEINTRESOURCE(IDD_EMOREC),0,emorec_dialog_handler);
    if (!h_emorec)  {
        MessageBoxW(0,L"Failed to create emorec window",L"Error",MB_ICONEXCLAMATION|MB_OK);
        return -1;
    }

	/* size & position */
	width_mw = 1148, height_mw = 575,
	width_tw = 948, height_tw = 575;

	h_screen = GetDesktopWindow();
	GetWindowRect(h_screen,&screen_rect);
	wx_mw = (screen_rect.right - screen_rect.left)/2 - (width_mw/2);
	wy_mw = (screen_rect.bottom - screen_rect.top)/2 - (height_mw/2);
	wx_tw = wx_mw + 100;
	wy_tw = wy_mw;

	SetWindowPos(h_main,HWND_TOP,wx_mw,wy_mw,width_mw,height_mw,SWP_NOZORDER);
	SetWindowPos(h_train,HWND_TOP,wx_tw,wy_tw,width_tw,height_tw,SWP_NOZORDER);
	ShowWindow(h_main,SW_SHOW);

	/* update dialog with the status (child) - kinda useless */
	UpdateWindow(h_main);

	/* load models (training data) here */
	ret = init_models(h_main);
	if (ret < 0) {
		MessageBoxW(0,L"Failed to load models",L"Error",MB_ICONEXCLAMATION|MB_OK);
		return -1;
	}

	/* main loop */
    MSG msg;
	int status;
	h_current = h_main;
	while (true) {
		status = GetMessageW(&msg,h_current,0,0);
		if (status == -1)
			return status;
		TranslateMessage(&msg);
        DispatchMessage(&msg);
	}

	/* window was closed */
	g_stop = true;
	while (g_running) Sleep(5);
	g_session->Release();
    return (int)msg.wParam;
}

/* main window handler function */
INT_PTR CALLBACK main_window_handler(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM) { 

	HMENU menu = GetMenu(hwndDlg);
	HMENU menu1;
	DWORD slider_pos;
	wchar_t delay_text[100];

    switch (message) { 
    case WM_INITDIALOG:
		SetWindowPos(GetDlgItem(hwndDlg,IDC_CROP),HWND_TOP,750,52,128,175,SWP_NOZORDER);
		CheckDlgButton(hwndDlg,IDC_MIRROR,BST_CHECKED);
		CheckDlgButton(hwndDlg,IDC_SCALE,BST_CHECKED);
		CheckDlgButton(hwndDlg,IDC_CHECK_EMO,BST_CHECKED);
		populate_device(menu);
		SendMessage(GetDlgItem(hwndDlg,IDC_DELAY_SLIDER), TBM_SETPOS, TRUE, 20);
		save_layout(hwndDlg,H_MAIN);
        return TRUE;
	case WM_HSCROLL:
		slider_pos = SendMessage(GetDlgItem(hwndDlg,IDC_DELAY_SLIDER), TBM_GETPOS, 0, 0);
		delay = slider_pos * 10;
		if (delay == 1000)
			swprintf_s(delay_text, L"Emotion Sample Rate : 1 s");
		else
			swprintf_s(delay_text, L"Emotion Sample Rate : %lld ms", delay);
		SetWindowText(GetDlgItem(hwndDlg,IDC_DELAY), delay_text);
		return TRUE;
    case WM_COMMAND:

		/* "device" */
		menu1 = GetSubMenu(menu,1);
		if (LOWORD(wParam)>=ID_DEVICEX && LOWORD(wParam)<ID_DEVICEX+GetMenuItemCount(menu1)) {
			CheckMenuRadioItem(menu1,0,GetMenuItemCount(menu1),LOWORD(wParam)-ID_DEVICEX,MF_BYPOSITION);
			return TRUE;
		}

        switch (LOWORD(wParam)) {
        case IDCANCEL:
			g_stop = true;
			if (g_running) {
				PostMessage(hwndDlg,WM_COMMAND,IDCANCEL,0);
			} else {
				DestroyWindow(hwndDlg); 
				PostQuitMessage(0);
			}
            return TRUE;
		case ID_INPUT_INTELPXCSDK:
			CheckMenuItem(menu,ID_INPUT_INTELPXCSDK,MF_CHECKED);
			CheckMenuItem(menu,ID_INPUT_OPENCV,MF_UNCHECKED);
			return TRUE;
		case ID_INPUT_OPENCV:
			CheckMenuItem(menu,ID_INPUT_INTELPXCSDK,MF_UNCHECKED);
			CheckMenuItem(menu,ID_INPUT_OPENCV,MF_CHECKED);
			return TRUE;
		case ID_EMOTIONS_EXPRESSIO:
			if (expressio_db) {
				CheckMenuItem(menu,ID_EMOTIONS_EXPRESSIO,MF_UNCHECKED);
				expressio_db = false;
			}
			else {
				CheckMenuItem(menu,ID_EMOTIONS_EXPRESSIO,MF_CHECKED);
				expressio_db = true;
			}
			return TRUE;
		case ID_EMOTIONS_DYNAMIC:
			if (dynamic_db) {
				CheckMenuItem(menu,ID_EMOTIONS_DYNAMIC,MF_UNCHECKED);
				dynamic_db = false;
			}
			else {
				CheckMenuItem(menu,ID_EMOTIONS_DYNAMIC,MF_CHECKED);
				dynamic_db = true;
			}
			return TRUE;
		case ID_EMOTIONS_JAFFE:
			if (jaffe_db) {
				CheckMenuItem(menu,ID_EMOTIONS_JAFFE,MF_UNCHECKED);
				jaffe_db = false;
			}
			else {
				CheckMenuItem(menu,ID_EMOTIONS_JAFFE,MF_CHECKED);
				jaffe_db = true;
			}
			return TRUE;
		case ID_START:
			Button_Enable(GetDlgItem(hwndDlg,ID_START),false);
			Button_Enable(GetDlgItem(hwndDlg,ID_STOP),true);
			for (int i=0;i<GetMenuItemCount(menu);i++)
				EnableMenuItem(menu,i,MF_BYPOSITION|MF_GRAYED);
			DrawMenuBar(hwndDlg);
			g_stop = false;
			g_running = true;
			CreateThread(0,0,thread_func,hwndDlg,0,0);
			Sleep(0);
			return TRUE;
		case ID_STOP:
			g_stop = true;
			if (g_running) {
				PostMessage(hwndDlg,WM_COMMAND,ID_STOP,0);
			} else {
				for (int i=0;i<GetMenuItemCount(menu);i++)
					EnableMenuItem(menu,i,MF_BYPOSITION|MF_ENABLED);
				DrawMenuBar(hwndDlg);
				Button_Enable(GetDlgItem(hwndDlg,ID_START),true);
				Button_Enable(GetDlgItem(hwndDlg,ID_STOP),false);
			}
			return TRUE;
		case ID_MODE_LIVE:
			CheckMenuItem(menu,ID_MODE_LIVE,MF_CHECKED);
			CheckMenuItem(menu,ID_MODE_PLAYBACK,MF_UNCHECKED);
			CheckMenuItem(menu,ID_MODE_RECORD,MF_UNCHECKED);
			return TRUE;
		case ID_MODE_PLAYBACK:
			CheckMenuItem(menu,ID_MODE_LIVE,MF_UNCHECKED);
			CheckMenuItem(menu,ID_MODE_PLAYBACK,MF_CHECKED);
			CheckMenuItem(menu,ID_MODE_RECORD,MF_UNCHECKED);
			GetPlaybackFile();
			return TRUE;
		case ID_MODE_RECORD:
			CheckMenuItem(menu,ID_MODE_LIVE,MF_UNCHECKED);
			CheckMenuItem(menu,ID_MODE_PLAYBACK,MF_UNCHECKED);
			CheckMenuItem(menu,ID_MODE_RECORD,MF_CHECKED);
			GetRecordFile();
			return TRUE;
		case IDC_BUTTON_TRAIN:
			ShowWindow(h_train,SW_SHOW);
			h_current = h_train;
			return TRUE;
		case IDC_BUTTON_EMOREC:
			ShowWindow(h_emorec,SW_SHOW);
			h_current = h_emorec;
			return TRUE;
        }
		break;
	case WM_SIZE:  /* resize */
		redo_layout(hwndDlg,H_MAIN);
		return TRUE;
    } 
    return FALSE;
}

/* train window handler function */
INT_PTR CALLBACK train_dialog_handler(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM) {

    switch (message) { 
    case WM_INITDIALOG:
		SetWindowPos(GetDlgItem(hwndDlg,IDC_PANEL_TRAIN),HWND_TOP,-238,10,640,480,SWP_NOZORDER); // wtf
		SetWindowPos(GetDlgItem(hwndDlg,ID_TEXT_GENDER_TRAIN),HWND_TOP,422,10,0,0,SWP_NOZORDER|SWP_NOSIZE);
		SetWindowPos(GetDlgItem(hwndDlg,IDC_RADIO_MALE),HWND_TOP,422,30,0,0,SWP_NOZORDER|SWP_NOSIZE);
		SetWindowPos(GetDlgItem(hwndDlg,IDC_RADIO_FEMALE),HWND_TOP,422,50,0,0,SWP_NOZORDER|SWP_NOSIZE);
		SetWindowPos(GetDlgItem(hwndDlg,IDC_TEXT_EMO_TRAIN),HWND_TOP,422,100,0,0,SWP_NOZORDER|SWP_NOSIZE);
		SetWindowPos(GetDlgItem(hwndDlg,IDC_RADIO_HAPPY),HWND_TOP,422,120,0,0,SWP_NOZORDER|SWP_NOSIZE);
		SetWindowPos(GetDlgItem(hwndDlg,IDC_RADIO_SURPRISE),HWND_TOP,422,140,0,0,SWP_NOZORDER|SWP_NOSIZE);
		SetWindowPos(GetDlgItem(hwndDlg,IDC_RADIO_FEAR),HWND_TOP,422,160,0,0,SWP_NOZORDER|SWP_NOSIZE);
		SetWindowPos(GetDlgItem(hwndDlg,IDC_RADIO_ANGER),HWND_TOP,422,180,0,0,SWP_NOZORDER|SWP_NOSIZE);
		SetWindowPos(GetDlgItem(hwndDlg,IDC_RADIO_DISGUST),HWND_TOP,422,200,0,0,SWP_NOZORDER|SWP_NOSIZE);
		SetWindowPos(GetDlgItem(hwndDlg,IDC_RADIO_SAD),HWND_TOP,422,220,0,0,SWP_NOZORDER|SWP_NOSIZE);
		SetWindowPos(GetDlgItem(hwndDlg,IDC_CROP_TRAIN),HWND_TOP,550,10,128,175,SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hwndDlg,IDC_BUTTON_SNAPSHOT),HWND_TOP,560,200,0,0,SWP_NOZORDER|SWP_NOSIZE);
		SetWindowPos(GetDlgItem(hwndDlg,ID_DONE),HWND_TOP,570,440,0,0,SWP_NOZORDER|SWP_NOSIZE);
		SetWindowPos(GetDlgItem(hwndDlg,IDCANCEL),HWND_TOP,570,473,0,0,SWP_NOZORDER|SWP_NOSIZE);
		save_layout(hwndDlg,H_TRAIN);
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDCANCEL:
			ShowWindow(h_train,SW_HIDE);
			h_current = h_main;
            return TRUE;
		case ID_DONE:
			train_emotions();
			ShowWindow(h_train,SW_HIDE);
			EnableWindow(h_train, TRUE);
			h_current = h_main;
			return TRUE;
		case IDC_BUTTON_SNAPSHOT:
			if(g_running)
				take_snapshot = true;
			return TRUE;
        } 
		break;
	case WM_SIZE:  /* resize */
		redo_layout(hwndDlg,H_TRAIN);
		return TRUE;
    } 

	return FALSE;
}

/* emorec window handler function */
INT_PTR CALLBACK emorec_dialog_handler(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM) {
	
	DWORD slider_pos;
	wchar_t delay_text[100];

    switch (message) { 
    case WM_INITDIALOG:
		SendMessage(GetDlgItem(hwndDlg,IDC_DELAY_SLIDER_EMOREC), TBM_SETPOS, TRUE, 20);
        return TRUE;
	case WM_HSCROLL:
		slider_pos = SendMessage(GetDlgItem(hwndDlg,IDC_DELAY_SLIDER_EMOREC), TBM_GETPOS, 0, 0);
		delay_emorec = slider_pos * 10;
		if (delay_emorec == 1000)
			swprintf_s(delay_text, L"Emotion Sample Rate : 1 s");
		else
			swprintf_s(delay_text, L"Emotion Sample Rate : %lld ms", delay_emorec);
		SetWindowText(GetDlgItem(hwndDlg,IDC_DELAY_EMOREC), delay_text);
		return TRUE;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDCANCEL:
			ShowWindow(h_emorec,SW_HIDE);
			h_current = h_main;
            return TRUE;
		case ID_START_EMOREC:
			Button_Enable(GetDlgItem(hwndDlg,ID_START_EMOREC),false);
			Button_Enable(GetDlgItem(hwndDlg,ID_STOP_EMOREC),true);
			ScrollBar_Enable(GetDlgItem(hwndDlg,IDC_DELAY_SLIDER_EMOREC),false);
			emorec_emotions.clear();
			emorec_save = true;
			return TRUE;
		case ID_STOP_EMOREC:
			Button_Enable(GetDlgItem(hwndDlg,ID_START_EMOREC),true);
			Button_Enable(GetDlgItem(hwndDlg,ID_STOP_EMOREC),false);
			ScrollBar_Enable(GetDlgItem(hwndDlg,IDC_DELAY_SLIDER_EMOREC),true);
			emorec_save = false;
			return TRUE;
		case IDC_VIEW_EMOREC:
			show_emotion_graph();
			return TRUE;
		case IDC_SAVE_EMOREC:
			save_emotion_graph();
			return TRUE;
        }
		break;
	case WM_SIZE:  /* resize */
		return TRUE;
    } 
	return FALSE;
}

/* thread function, exectued when button start is clicked */
static DWORD WINAPI thread_func(LPVOID arg) {

	int ret = 0;

	if (GetMenuState(GetMenu((HWND)arg),ID_INPUT_INTELPXCSDK,MF_BYCOMMAND)&MF_CHECKED) {
		ret = capture_video_util_capture((HWND)arg);  // if anything, change to advanced_pipeline
	} else {
		ret = capture_video_opencv((HWND)arg);
	}

	PostMessage((HWND)arg,WM_COMMAND,ID_STOP,0);
	g_running = false;
	return ret;
}

/* draws mat image to panel or crop zone from hwnd as bitmap */
void draw_bitmap(HWND hwndDlg, cv::Mat frame, int flags) {

	HWND hwndPanel;

	if (h_current == h_main) {
		if (flags == BIG_IMAGE) {
			if (g_bitmap_main) {
				DeleteObject(g_bitmap_main);
				g_bitmap_main = 0;
			}
			hwndPanel = GetDlgItem(h_main, IDC_PANEL);
		}
		else if (flags == SMALL_IMAGE) {
			if (g_bitmap_main_mini) {
				DeleteObject(g_bitmap_main_mini);
				g_bitmap_main_mini = 0;
			}
			hwndPanel = GetDlgItem(h_main, IDC_CROP);
		}
		else {
			MessageBoxW(0,L"Bad arguments in draw_bitmap func",L"Error",MB_ICONEXCLAMATION|MB_OK);
			return;
		}
	}
	else if (h_current == h_train) {
		if (flags == BIG_IMAGE) {
			if (g_bitmap_train) {
				DeleteObject(g_bitmap_train);
				g_bitmap_train = 0;
			}
			hwndPanel = GetDlgItem(h_train, IDC_PANEL_TRAIN);
		}
		else if (flags == SMALL_IMAGE) {
			if (g_bitmap_train_mini) {
				DeleteObject(g_bitmap_train_mini);
				g_bitmap_train_mini = 0;
			}
			hwndPanel = GetDlgItem(h_train, IDC_CROP_TRAIN);
		}
		else {
			MessageBoxW(0,L"Bad arguments in draw_bitmap func",L"Error",MB_ICONEXCLAMATION|MB_OK);
			return;
		}
	}
	else if (h_current == h_emorec) {
		if (flags == BIG_IMAGE) {
			if (g_bitmap_emorec) {
				DeleteObject(g_bitmap_emorec);
				g_bitmap_emorec = 0;
			}
			hwndPanel = GetDlgItem(h_emorec, IDC_PANEL_EMOREC);
		}
		else {
			// this can happen, move to debug
			return;
		}
	}

	HDC dc = GetDC(hwndPanel);
	BITMAPINFO binfo;
	memset(&binfo,0,sizeof(binfo));
	binfo.bmiHeader.biWidth = frame.cols;
	binfo.bmiHeader.biHeight = - frame.rows;
	binfo.bmiHeader.biBitCount = 24;
	binfo.bmiHeader.biPlanes = 1;
	binfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	binfo.bmiHeader.biCompression = BI_RGB;

	if (h_current == h_main) {
		if (flags == BIG_IMAGE) 
			g_bitmap_main = CreateDIBitmap(dc, &binfo.bmiHeader, CBM_INIT, frame.data, &binfo, DIB_RGB_COLORS);
		else if (flags == SMALL_IMAGE)
			g_bitmap_main_mini = CreateDIBitmap(dc, &binfo.bmiHeader, CBM_INIT, frame.data, &binfo, DIB_RGB_COLORS);
	}
	else if (h_current == h_train) {
		if (flags == BIG_IMAGE) 
			g_bitmap_train = CreateDIBitmap(dc, &binfo.bmiHeader, CBM_INIT, frame.data, &binfo, DIB_RGB_COLORS);
		else if (flags == SMALL_IMAGE)
			g_bitmap_train_mini = CreateDIBitmap(dc, &binfo.bmiHeader, CBM_INIT, frame.data, &binfo, DIB_RGB_COLORS);
	}
	else if (h_current == h_emorec) {
		g_bitmap_emorec = CreateDIBitmap(dc, &binfo.bmiHeader, CBM_INIT, frame.data, &binfo, DIB_RGB_COLORS);
	}
	ReleaseDC(hwndPanel, dc);
}

/* called after draw_bitmap to update dialog */
void update_panel(HWND hwndDlg, int flags) {

	HWND panel;

	if (h_current == h_main) {
		if (flags == BIG_IMAGE) {
			if (!g_bitmap_main)
				return;
			panel = GetDlgItem(h_main,IDC_PANEL);
		}
		else if (flags == SMALL_IMAGE) {
			if (!g_bitmap_main_mini)
				return;
			panel = GetDlgItem(h_main,IDC_CROP);
		}
		
	}
	else if (h_current == h_train) {
		if (flags == BIG_IMAGE) {
			if (!g_bitmap_train)
				return;
			panel = GetDlgItem(h_train,IDC_PANEL_TRAIN);
		}
		else if (flags == SMALL_IMAGE) {
			if (!g_bitmap_train_mini)
				return;
			panel = GetDlgItem(h_train,IDC_CROP_TRAIN);
		}
	}
	else if (h_current == h_emorec) {
		if (flags == BIG_IMAGE) {
			if (!g_bitmap_emorec)
				return;
			panel = GetDlgItem(h_emorec,IDC_PANEL_EMOREC);
		}
	}

	RECT rc;
	GetClientRect(panel,&rc);

	HDC dc = GetDC(panel);
	HBITMAP bitmap = CreateCompatibleBitmap(dc,rc.right,rc.bottom);
	HDC dc2 = CreateCompatibleDC(dc);
	SelectObject(dc2,bitmap);
	FillRect(dc2,&rc,(HBRUSH)GetStockObject(GRAY_BRUSH));
	SetStretchBltMode(dc2, HALFTONE);

	/* draw the main window */
	HDC dc3=CreateCompatibleDC(dc);
	BITMAP bm;
	bool scale, mirror;

	if (h_current == h_main) {
		if (flags == BIG_IMAGE) {
			SelectObject(dc3,g_bitmap_main);
			GetObject(g_bitmap_main,sizeof(BITMAP),&bm);
		}
		else if (flags == SMALL_IMAGE) {
			SelectObject(dc3,g_bitmap_main_mini);
			GetObject(g_bitmap_main_mini,sizeof(BITMAP),&bm);
		}
	}
	else if (h_current == h_train) {
		if (flags == BIG_IMAGE) {
			SelectObject(dc3,g_bitmap_train);
			GetObject(g_bitmap_train,sizeof(BITMAP),&bm);
		}
		else if (flags == SMALL_IMAGE) {
			SelectObject(dc3,g_bitmap_train_mini);
			GetObject(g_bitmap_train_mini,sizeof(BITMAP),&bm);
		}
	}
	else if (h_current == h_emorec) {
		if (flags == BIG_IMAGE) {
			SelectObject(dc3,g_bitmap_emorec);
			GetObject(g_bitmap_emorec,sizeof(BITMAP),&bm);
		}
	}

	if (h_current == h_emorec) {
		scale = true;
		mirror = true;
	}
	else {
		scale = Button_GetState(GetDlgItem(h_main,IDC_SCALE))&BST_CHECKED;
		mirror = Button_GetState(GetDlgItem(h_main,IDC_MIRROR))&BST_CHECKED;
	}
	if (mirror) {
		if (scale) {
			RECT rc1 = get_resize_rect(rc,bm);
			StretchBlt(dc2,rc1.left+rc1.right-1,rc1.top,-rc1.right,rc1.bottom,dc3,0,0,bm.bmWidth,bm.bmHeight,SRCCOPY);
		} else {
			StretchBlt(dc2,bm.bmWidth-1,0,-bm.bmWidth,bm.bmHeight,dc3,0,0,bm.bmWidth,bm.bmHeight,SRCCOPY);
		}
	} else {
		if (scale) {
			RECT rc1 = get_resize_rect(rc,bm);
			StretchBlt(dc2,rc1.left,rc1.top,rc1.right,rc1.bottom,dc3,0,0,bm.bmWidth,bm.bmHeight,SRCCOPY);
		} else {
			BitBlt(dc2,0,0,rc.right,rc.bottom,dc3,0,0,SRCCOPY);
		}
	}

	DeleteObject(dc3);
	DeleteObject(dc2);
	ReleaseDC(h_current,dc);

	HBITMAP bitmap2 = (HBITMAP)SendMessage(panel,STM_GETIMAGE,0,0);
	if (bitmap2) DeleteObject(bitmap2);
	SendMessage(panel,STM_SETIMAGE,IMAGE_BITMAP,(LPARAM)bitmap);
	InvalidateRect(panel,0,TRUE);
}


/* draws image in IDC_PANEL from hwndDlg - only works with intel sdk */
void draw_bitmap(HWND hwndDlg, PXCImage *image) {

    if (g_bitmap_main) {
        DeleteObject(g_bitmap_main);
		g_bitmap_main=0;
    }

    PXCImage::ImageInfo info;
    image->QueryInfo(&info);
    PXCImage::ImageData data;

	/* 32 sau 24 */
    if (image->AcquireAccess(PXCImage::ACCESS_READ,PXCImage::COLOR_FORMAT_RGB24, &data)>=PXC_STATUS_NO_ERROR) {

		HWND hwndPanel = GetDlgItem(hwndDlg,IDC_PANEL);
        HDC dc = GetDC(hwndPanel);
		BITMAPINFO binfo;

		memset(&binfo,0,sizeof(binfo));
		binfo.bmiHeader.biWidth = info.width;
		binfo.bmiHeader.biHeight = - info.height;
		binfo.bmiHeader.biBitCount = 24;
		binfo.bmiHeader.biPlanes = 1;
		binfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		binfo.bmiHeader.biCompression = BI_RGB;
        g_bitmap_main = CreateDIBitmap(dc, &binfo.bmiHeader, CBM_INIT, data.planes[0], &binfo, DIB_RGB_COLORS);
        ReleaseDC(hwndPanel, dc);
		image->ReleaseAccess(&data);
    }
}

/* set main window status */
void set_status(HWND hwndDlg, pxcCHAR *line) {
	HWND hwndStatus = GetDlgItem(hwndDlg,IDC_STATUS);
	SetWindowText(hwndStatus,line);
}

/* set train window status */
void set_status_train(pxcCHAR *line) {
	HWND hwndStatus = GetDlgItem(h_train,IDC_STATUS_TRAIN);
	SetWindowText(hwndStatus,line);
}

/* displays gender */
void set_gender(HWND hwndDlg, int gender) {
	if (gender == MALE)
		SetWindowText(GetDlgItem(hwndDlg,IDC_TEXT_GENDER), L"Gender : Male");
	else if (gender == FEMALE)
		SetWindowText(GetDlgItem(hwndDlg,IDC_TEXT_GENDER), L"Gender : Female");
	else
		SetWindowText(GetDlgItem(hwndDlg,IDC_TEXT_GENDER), L"Gender : N/A");
}

/* makes emotion progress bar full */
static void turn_emotion_on(HWND hwndDlg, int emo){
	if (emo == HAPPINESS)
		SendMessage(GetDlgItem(hwndDlg,IDC_PRO_HAPPY),PBM_SETPOS,100,0);
	else if (emo == SADNESS)
		SendMessage(GetDlgItem(hwndDlg,IDC_PRO_SAD),PBM_SETPOS,100,0);
	else if (emo == ANGER)
		SendMessage(GetDlgItem(hwndDlg,IDC_PRO_ANGER),PBM_SETPOS,100,0);
	else if (emo == FEAR)
		SendMessage(GetDlgItem(hwndDlg,IDC_PRO_FEAR),PBM_SETPOS,100,0);
	else if (emo == DISGUST)
		SendMessage(GetDlgItem(hwndDlg,IDC_PRO_DISGUST),PBM_SETPOS,100,0);
	else if (emo == SURPRISE)
		SendMessage(GetDlgItem(hwndDlg,IDC_PRO_SURPRISE),PBM_SETPOS,100,0);
}

/* makes emotion progress bar empty */
static void turn_emotion_off(HWND hwndDlg, int emo){
	if (emo == HAPPINESS)
		SendMessage(GetDlgItem(hwndDlg,IDC_PRO_HAPPY),PBM_SETPOS,0,0);
	else if (emo == SADNESS)
		SendMessage(GetDlgItem(hwndDlg,IDC_PRO_SAD),PBM_SETPOS,0,0);
	else if (emo == ANGER)
		SendMessage(GetDlgItem(hwndDlg,IDC_PRO_ANGER),PBM_SETPOS,0,0);
	else if (emo == FEAR)
		SendMessage(GetDlgItem(hwndDlg,IDC_PRO_FEAR),PBM_SETPOS,0,0);
	else if (emo == DISGUST)
		SendMessage(GetDlgItem(hwndDlg,IDC_PRO_DISGUST),PBM_SETPOS,0,0);
	else if (emo == SURPRISE)
		SendMessage(GetDlgItem(hwndDlg,IDC_PRO_SURPRISE),PBM_SETPOS,0,0);
}

/* displays emotion */
void set_emotion(HWND hwndDlg, int emo) {
	if (current_emotion > 0)
		turn_emotion_off(hwndDlg,current_emotion);
	turn_emotion_on(hwndDlg,emo);
	current_emotion = emo;
}

/* checks if the "face" checkbox is checked or not */
bool is_face_checked(HWND hwndDlg) {
	return (Button_GetState(GetDlgItem(hwndDlg,IDC_CHECK_FACE)) & BST_CHECKED);
}

/* checks if the "emotion" checkbox is checked or not */
bool is_emo_checked(HWND hwndDlg) {
	return (Button_GetState(GetDlgItem(hwndDlg,IDC_CHECK_EMO)) & BST_CHECKED);
}

/* checks if the "eyes" checkbox is checked or not */
bool is_eyes_checked(HWND hwndDlg) {
	return (Button_GetState(GetDlgItem(hwndDlg,IDC_CHECK_EYES)) & BST_CHECKED);
}

/* checks if the "nose" checkbox is checked or not */
bool is_nose_checked(HWND hwndDlg) {
	return (Button_GetState(GetDlgItem(hwndDlg,IDC_CHECK_NOSE)) & BST_CHECKED);
}

/* checks if the "mouth" checkbox is checked or not */
bool is_mouth_checked(HWND hwndDlg) {
	return (Button_GetState(GetDlgItem(hwndDlg,IDC_CHECK_MOUTH)) & BST_CHECKED);
}

/* checks if the "upper face" checkbox is checked or not */
bool is_upface_checked(HWND hwndDlg) {
	return (Button_GetState(GetDlgItem(hwndDlg,IDC_CHECK_UPFACE)) & BST_CHECKED);
}

/* checks if the "lower face" checkbox is checked or not */
bool is_loface_checked(HWND hwndDlg) {
	return (Button_GetState(GetDlgItem(hwndDlg,IDC_CHECK_LOFACE)) & BST_CHECKED);
}

/* checks if "male" is checked in train mode */
bool train_male(void) {
	return (Button_GetState(GetDlgItem(h_train,IDC_RADIO_MALE)) & BST_CHECKED);
}

/* checks if "male" is checked in train mode */
bool train_female(void) {
	return (Button_GetState(GetDlgItem(h_train,IDC_RADIO_FEMALE)) & BST_CHECKED);
}

/* checks if "happiness" is checked in train mode */
bool train_happiness(void) {
	return (Button_GetState(GetDlgItem(h_train,IDC_RADIO_HAPPY)) & BST_CHECKED);
}

/* checks if "surprise" is checked in train mode */
bool train_surprise(void) {
	return (Button_GetState(GetDlgItem(h_train,IDC_RADIO_SURPRISE)) & BST_CHECKED);
}

/* checks if "fear" is checked in train mode */
bool train_fear(void) {
	return (Button_GetState(GetDlgItem(h_train,IDC_RADIO_FEAR)) & BST_CHECKED);
}

/* checks if "anger" is checked in train mode */
bool train_anger(void) {
	return (Button_GetState(GetDlgItem(h_train,IDC_RADIO_ANGER)) & BST_CHECKED);
}

/* checks if "disgust" is checked in train mode */
bool train_disgust(void) {
	return (Button_GetState(GetDlgItem(h_train,IDC_RADIO_DISGUST)) & BST_CHECKED);
}

/* checks if "sadness" is checked in train mode */
bool train_sadness(void) {
	return (Button_GetState(GetDlgItem(h_train,IDC_RADIO_SAD)) & BST_CHECKED);
}

/* disables face detection */
void disable_face(HWND hwndDlg) {
	CheckDlgButton(hwndDlg,IDC_CHECK_FACE,BST_UNCHECKED);
	EnableWindow(GetDlgItem(hwndDlg,IDC_CHECK_FACE),false);
}

/* disables eyes detection */
void disable_eyes(HWND hwndDlg) {
	CheckDlgButton(hwndDlg,IDC_CHECK_EYES,BST_UNCHECKED);
	EnableWindow(GetDlgItem(hwndDlg,IDC_CHECK_EYES),false);
}

/* disables nose detection */
void disable_nose(HWND hwndDlg) {
	CheckDlgButton(hwndDlg,IDC_CHECK_NOSE,BST_UNCHECKED);
	EnableWindow(GetDlgItem(hwndDlg,IDC_CHECK_NOSE),false);
}

/* disables mouth detection */
void disable_mouth(HWND hwndDlg) {
	CheckDlgButton(hwndDlg,IDC_CHECK_MOUTH,BST_UNCHECKED);
	EnableWindow(GetDlgItem(hwndDlg,IDC_CHECK_MOUTH),false);
}

/* disables emotion detection */
void disable_emo(HWND hwndDlg) {
	CheckDlgButton(hwndDlg,IDC_CHECK_EMO,BST_UNCHECKED);
	EnableWindow(GetDlgItem(hwndDlg,IDC_CHECK_EMO),false);
}

/* gets checked item from menu TODO megapenis*/
int get_checked(HMENU menu) {
	for (int i = 0; i < GetMenuItemCount(menu); i++)
		if (GetMenuState(menu,i,MF_BYPOSITION) & MF_CHECKED)
			return i;
	return -1;
}

/* keep the aspect ratio */
static RECT get_resize_rect(RECT rc, BITMAP bm) {
	RECT rc1;
	float sx = (float)rc.right/(float)bm.bmWidth;
	float sy = (float)rc.bottom/(float)bm.bmHeight;
	float sxy = (sx < sy) ? sx : sy;
	rc1.right = (int)(bm.bmWidth*sxy);
	rc1.left = (rc.right - rc1.right)/2 + rc.left;
	rc1.bottom = (int)(bm.bmHeight*sxy);
	rc1.top = (rc.bottom - rc1.bottom)/2 + rc.top;
	return rc1;
}

/* saves layout for window */
void save_layout(HWND hwndDlg, int flags) {

	RECT* g_layout;
	int* g_controls, num_controls, wnd, wnds;

	if (flags == H_MAIN) {
		wnd = IDC_PANEL;
		wnds = IDC_STATUS;
		g_layout = g_layout_main;
		g_controls = g_controls_main;
		num_controls = sizeof(g_controls_main)/sizeof(g_controls[0]);
	}
	else if (flags == H_TRAIN) {
		wnd = IDC_PANEL_TRAIN;
		wnds = IDC_STATUS_TRAIN;
		g_layout = g_layout_train;
		g_controls = g_controls_train;
		num_controls = sizeof(g_controls_train)/sizeof(g_controls[0]);
	}
	else if (flags == H_EMOREC) {
		return;
	}

	GetClientRect(hwndDlg,&g_layout[0]);
	ClientToScreen(hwndDlg,(LPPOINT)&g_layout[0].left);
	ClientToScreen(hwndDlg,(LPPOINT)&g_layout[0].right);
	GetWindowRect(GetDlgItem(hwndDlg,wnd),&g_layout[1]);
	GetWindowRect(GetDlgItem(hwndDlg,wnds),&g_layout[2]);
	for (int i = 0; i < num_controls; i++)
		GetWindowRect(GetDlgItem(hwndDlg,g_controls[i]),&g_layout[3+i]);
}

/* updates layout after a resize */
void redo_layout(HWND hwndDlg, int flags) {

	RECT* g_layout;
	int* g_controls, num_controls, wnd, wnds;
	RECT rect;
	GetClientRect(hwndDlg,&rect);

	if (flags == H_MAIN) {
		wnd = IDC_PANEL;
		wnds = IDC_STATUS;
		g_layout = g_layout_main;
		g_controls = g_controls_main;
		num_controls = sizeof(g_controls_main)/sizeof(g_controls[0]);
	}
	else if (flags == H_TRAIN) {
		wnd = IDC_PANEL_TRAIN;
		wnds = IDC_STATUS_TRAIN;
		g_layout = g_layout_train;
		g_controls = g_controls_train;
		num_controls = sizeof(g_controls_train)/sizeof(g_controls[0]);
	}
	else if (flags == H_EMOREC) {
		return;
	}

	/* status */
	SetWindowPos(GetDlgItem(hwndDlg,wnds),hwndDlg,
		0,
		rect.bottom-(g_layout[2].bottom-g_layout[2].top),
		rect.right-rect.left,
		(g_layout[2].bottom-g_layout[2].top),SWP_NOZORDER);

	/* panel */
	SetWindowPos(GetDlgItem(hwndDlg,wnd),hwndDlg,
		(g_layout[1].left-g_layout[0].left),
		(g_layout[1].top-g_layout[0].top),
		rect.right-(g_layout[1].left-g_layout[0].left)-(g_layout[0].right-g_layout[1].right),
		rect.bottom-(g_layout[1].top-g_layout[0].top)-(g_layout[0].bottom-g_layout[1].bottom),
		SWP_NOZORDER);

	/* buttons & checkboxes */
	for (int i = 0; i < num_controls; i++) {
		SetWindowPos(GetDlgItem(hwndDlg,g_controls[i]),hwndDlg,
			rect.right-(g_layout[0].right-g_layout[3+i].left),
			(g_layout[3+i].top-g_layout[0].top),
			(g_layout[3+i].right-g_layout[3+i].left),
			(g_layout[3+i].bottom-g_layout[3+i].top),
			SWP_NOZORDER);
	}
}

/* finds out avail devices using intel sdk and fills in menu */
static void populate_device(HMENU menu) {

	DeleteMenu(menu,1,MF_BYPOSITION);

	PXCSession::ImplDesc desc;
	memset (&desc,0,sizeof(desc));
	desc.group = PXCSession::IMPL_GROUP_SENSOR;
	desc.subgroup = PXCSession::IMPL_SUBGROUP_VIDEO_CAPTURE;
	HMENU menu1 = CreatePopupMenu();

	for (int i = 0,k = ID_DEVICEX;; i++) {
		PXCSession::ImplDesc desc1;
		if (g_session->QueryImpl(&desc,i,&desc1) < PXC_STATUS_NO_ERROR)
			break;

		PXCSmartPtr<PXCCapture> capture;
		if (g_session->CreateImpl<PXCCapture>(&desc1,&capture) < PXC_STATUS_NO_ERROR)
			continue;

		for (int j = 0;; j++) {
			PXCCapture::DeviceInfo dinfo;
			if (capture->QueryDevice(j,&dinfo) < PXC_STATUS_NO_ERROR)
				break;

			AppendMenu(menu1,MF_STRING,k++,dinfo.name);
		}
	}

	CheckMenuRadioItem(menu1,0,GetMenuItemCount(menu1),0,MF_BYPOSITION);
	InsertMenu(menu,1,MF_BYPOSITION|MF_POPUP,(UINT_PTR)menu1,L"Device");
}

/* returns the name of the device checked in "device" menu */
pxcCHAR* get_checked_device(HWND hwndDlg) {
	HMENU menu = GetSubMenu(GetMenu(hwndDlg),1);	// ID_DEVICE
	static pxcCHAR line[256];
	GetMenuString(menu,get_checked(menu),line,sizeof(line)/sizeof(pxcCHAR),MF_BYPOSITION);
	return line;
}

/* -------------------------------------------------------------------------------------------------------------- */

pxcCHAR *GetCheckedModule(HWND hwndDlg) {
	HMENU menu=GetSubMenu(GetMenu(hwndDlg),1);	// ID_MODULE
	static pxcCHAR line[256];
	GetMenuString(menu,get_checked(menu),line,sizeof(line)/sizeof(pxcCHAR),MF_BYPOSITION);
	return line;
}

void DrawLocation(HWND hwndDlg, PXCFaceAnalysis::Detection::Data *data) {
	if (!g_bitmap_main) return;

	HWND hwndPanel=GetDlgItem(hwndDlg,IDC_PANEL);
	HDC dc=GetDC(hwndPanel);
	HDC dc2=CreateCompatibleDC(dc);
	SelectObject(dc2,g_bitmap_main);

	BITMAP bm;
	GetObject(g_bitmap_main,sizeof(bm),&bm);

    if (Button_GetState(GetDlgItem(hwndDlg,IDC_LOCATION))&BST_CHECKED) {
		HPEN cyan=CreatePen(PS_SOLID,3,RGB(255,255,0));
		SelectObject(dc2,cyan);

		MoveToEx(dc2,data->rectangle.x,data->rectangle.y,0);
		LineTo(dc2,data->rectangle.x,data->rectangle.y+data->rectangle.h);
		LineTo(dc2,data->rectangle.x+data->rectangle.w,data->rectangle.y+data->rectangle.h);
		LineTo(dc2,data->rectangle.x+data->rectangle.w,data->rectangle.y);
		LineTo(dc2,data->rectangle.x,data->rectangle.y);

		WCHAR line[64];
		swprintf_s<sizeof(line)/sizeof(pxcCHAR)>(line,L"%d",data->fid);
		TextOut(dc2,data->rectangle.x,data->rectangle.y,line,wcslen(line));
		DeleteObject(cyan);
    }
	DeleteObject(dc2);
	ReleaseDC(hwndPanel,dc);
}

bool GetPlaybackState(HWND hwndDlg) {
	return (GetMenuState(GetMenu(hwndDlg),ID_MODE_PLAYBACK,MF_BYCOMMAND)&MF_CHECKED)!=0;
}

static void GetPlaybackFile (void) {
	OPENFILENAME ofn;
	memset(&ofn,0,sizeof(ofn));
	ofn.lStructSize=sizeof(ofn);
	ofn.lpstrFilter=L"All Files\0*.*\0\0";
	ofn.lpstrFile=g_file; g_file[0]=0;
	ofn.nMaxFile=sizeof(g_file)/sizeof(pxcCHAR);
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER;
	if (!GetOpenFileName(&ofn)) g_file[0]=0;
}

bool GetRecordState (HWND hwndDlg) {
	return (GetMenuState(GetMenu(hwndDlg),ID_MODE_RECORD,MF_BYCOMMAND)&MF_CHECKED)!=0;
}

static void GetRecordFile (void) {
	OPENFILENAME ofn;
	memset(&ofn,0,sizeof(ofn));
	ofn.lStructSize=sizeof(ofn);
	ofn.lpstrFilter=L"All Files\0*.*\0\0";
	ofn.lpstrFile=g_file; g_file[0]=0;
	ofn.nMaxFile=sizeof(g_file)/sizeof(pxcCHAR);
	ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_EXPLORER;
	if (!GetSaveFileName(&ofn)) g_file[0]=0;
}


