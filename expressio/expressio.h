/*******************************************************************************

	Expressio
	Mihail Dunaev,
	Politehnica University of Bucharest
	14 Sep 2014

	Expressio - A system that can identify the user's predominant emotion
	            (from the 6 basic ones), analyzing frames received from a
				video camera.

	expressio.h - header file for expressio.cpp

*******************************************************************************/

#ifndef _EXPRESSIO_H_
#define _EXPRESSIO_H_

#define MALE 0
#define FEMALE 1

#define HAPPINESS 1
#define SADNESS 2
#define ANGER 3
#define FEAR 4
#define DISGUST 5
#define SURPRISE 6

/* VIDEO CAPTURE */

/* loads parameters into memory */
int init_models(HWND hwndDlg);

/* captures video from screen and displays it to hwnd */
int capture_video_opencv(HWND hwndDlg);

/* captures video from creative camera and displays it on screen using util capture (2) & face render interfaces */
int capture_video_util_capture(HWND hwndDlg);

/* captures video from the screen using util capture; original func */
void advanced_pipeline(HWND hwndDlg);

/* captures video from the screen using util pipeline; original func */
void simple_pipeline(HWND hwndDlg);

/* captures video from creative camera and displays it on screen using util pipeline (1) & face render interfaces 
 * doesn't work atm */
int capture_video_util_pipeline(HWND hwndDlg);


/* EMOTION */

/* trains emotion model with everything under "dbase/" */
void train_emotions(void);

/* shows emotion graph (emorec) */
void show_emotion_graph(void);

/* saves emotion history to file */
void save_emotion_graph(void);


/* UTIL */

/* crops the face from "src" to "dst"; the resulting image will have "width" x "height"; if face is not specified it will use already trained classifier or NULL (load from file) */
int get_face(cv::Mat* src, cv::Mat* dst, int width, int height, int method, cv::Rect* face, cv::CascadeClassifier* face_cascade);

/* initialize face analysis module to detect both face and landmark with 7 points */
void init_face_analysis(PXCFaceAnalysis** face, PXCSession* ses);

/* converts opencv image to pxc image (bgr) */
void convert_cv_to_pxc(cv::Mat src, PXCImage** dst, PXCSession* ses);

/* converts pxc image to opencv image (bgr) */
void convert_pxc_to_cv(PXCImage* src, cv::Mat** dst);

/* gets current time in ms */
static long long get_ms_now(void);


#endif /* _EXPRESSIO_H_ */