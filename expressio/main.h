/*******************************************************************************

	Expressio
	Mihail Dunaev,
	Politehnica University of Bucharest
	14 Sep 2014

	Expressio - A system that can identify the user's predominant emotion
	            (from the 6 basic ones), analyzing frames received from a
				video camera.

	main.h    - header file for main.cpp

*******************************************************************************/

#ifndef _MAIN_H_
#define _MAIN_H_

#define BIG_IMAGE 0
#define SMALL_IMAGE 1

#define H_MAIN 0
#define H_TRAIN 1
#define H_EMOREC 2

/* draws mat image to panel or crop zone from hwnd as bitmap */
void draw_bitmap(HWND hwndDlg, cv::Mat frame, int flags);

/* draws image in IDC_PANEL from hwndDlg - only works with intel sdk */
void draw_bitmap(HWND,PXCImage*);

/* called after draw_bitmap to update dialog */
void update_panel(HWND hwndDlg, int flags);

/* set main window status */
void set_status(HWND hwndDlg, pxcCHAR *line);

/* set train window status */
void set_status_train(pxcCHAR *line);

/* displays gender */
void set_gender(HWND hwndDlg, int gender);

/* displays emotion */
void set_emotion(HWND hwndDlg, int emo);

/* checks if the "face" checkbox is checked or not */
bool is_face_checked(HWND hwndDlg);

/* checks if the "emotion" checkbox is checked or not */
bool is_emo_checked(HWND hwndDlg);

/* checks if the "eyes" checkbox is checked or not */
bool is_eyes_checked(HWND hwndDlg);

/* checks if the "nose" checkbox is checked or not */
bool is_nose_checked(HWND hwndDlg);

/* checks if the "mouth" checkbox is checked or not */
bool is_mouth_checked(HWND hwndDlg);

/* checks if the "upper face" checkbox is checked or not */
bool is_upface_checked(HWND hwndDlg);

/* checks if the "lower face" checkbox is checked or not */
bool is_loface_checked(HWND hwndDlg);

/* checks if "male" is checked in train mode */
bool train_male(void);

/* checks if "male" is checked in train mode */
bool train_female(void);

/* checks if "happiness" is checked in train mode */
bool train_happiness(void);

/* checks if "surprise" is checked in train mode */
bool train_surprise(void);

/* checks if "fear" is checked in train mode */
bool train_fear(void);

/* checks if "anger" is checked in train mode */
bool train_anger(void);

/* checks if "disgust" is checked in train mode */
bool train_disgust(void);

/* checks if "sadness" is checked in train mode */
bool train_sadness(void);

/* disables face detection */
void disable_face(HWND hwndDlg);

/* disables eyes detection */
void disable_eyes(HWND hwndDlg);

/* disables nose detection */
void disable_nose(HWND hwndDlg);

/* disables mouth detection */
void disable_mouth(HWND hwndDlg);

/* disables emotion detection */
void disable_emo(HWND hwndDlg);


/* used in main.cpp only */

/* main window handler function */
INT_PTR CALLBACK main_window_handler(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM);

/* train window handler function */
INT_PTR CALLBACK train_dialog_handler(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM);

/* emorec window handler function */
INT_PTR CALLBACK emorec_dialog_handler(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM);

/* thread function, exectued when button start is clicked */
static DWORD WINAPI thread_func(LPVOID arg);

/* returns which item from menu is checked (i from 0 to n) */
int get_checked(HMENU menu);

/* aspect ratio */
static RECT get_resize_rect(RECT rc, BITMAP bm);

/* saves layout for window */
void save_layout(HWND hwndDlg, int flags);

/* updates layout after a resize */
void redo_layout(HWND hwndDlg, int flags);

/* finds out avail devices using intel sdk and fills in menu */
static void populate_device(HMENU menu);

/* returns the name of the device checked in "device" menu */
pxcCHAR* get_checked_device(HWND hwndDlg);

/* returns the name of the module checked in "module" menu */
pxcCHAR *GetCheckedModule(HWND hwndDlg);

/* true if playback is checked */
bool GetPlaybackState(HWND hwndDlg);

/* load menu for playback option I guess */
static void GetPlaybackFile(void);

/* true if record is checked */
bool GetRecordState(HWND hwndDlg);

/* save menu for record file I guess */
static void GetRecordFile(void);

#endif /* _MAIN_H_ */