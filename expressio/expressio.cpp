/*******************************************************************************

	Expressio
	Mihail Dunaev,
	Politehnica University of Bucharest
	14 Sep 2014

	Expressio - A system that can identify the user's predominant emotion
	            (from the 6 basic ones), analyzing frames received from a
				video camera.

	expressio.cpp - code that deals with emotion and object detection

*******************************************************************************/

#include <Windows.h>
#include <vector>
#include <ctime>
#include "util_pipeline.h"

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/contrib/contrib.hpp>

#include "expressio.h"
#include "main.h"

extern volatile bool g_stop;
extern pxcCHAR g_file[1024];
extern PXCSession *g_session;
extern HWND h_main, h_train, h_emorec, h_current;
volatile bool g_disconnected = false;

cv::CascadeClassifier face_cascade;
cv::CascadeClassifier eye_cascade;
cv::CascadeClassifier nose_cascade;
cv::CascadeClassifier mouth_cascade;

cv::Ptr<cv::FaceRecognizer> gender_model;
cv::Ptr<cv::FaceRecognizer> emo_model;

long long delay, start, delay_emorec, start_emorec;
bool take_snapshot, jaffe_db, expressio_db, dynamic_db, emorec_save;
int count_dynamic_male[6], count_dynamic_female[6];

std::vector<int> emorec_emotions;

/* loads parameters into memory */
int init_models (HWND hwndDlg) {

	/* face detection */
	if (!face_cascade.load("models/face.xml"))
		disable_face(hwndDlg);

	/* eyes detection */
	if (!eye_cascade.load("models/eye.xml"))
		disable_eyes(hwndDlg);

	/* nose detection */
	if (!nose_cascade.load("models/nose.xml"))
		disable_nose(hwndDlg);

	/* mouth detection */
	if (!mouth_cascade.load("models/mouth.xml"))
		disable_mouth(hwndDlg);

	/* gender classification */ 
	gender_model = cv::createFisherFaceRecognizer();
	gender_model->load("models/gender.yml");

	/* emotion classification */
	emo_model = cv::createFisherFaceRecognizer();
	emo_model->load("models/emo.yml");

	/* dynamic index */
	HANDLE hfind;
	WIN32_FIND_DATA ffd;
	TCHAR dir_name[200];
	TCHAR aux_txt[200];

	/* happiness male */
	count_dynamic_male[HAPPINESS-1] = 0;
	swprintf_s(dir_name, L"dbase\\dynamic\\male\\happiness\\*");
	hfind = FindFirstFile(dir_name, &ffd);
	do {
		if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			count_dynamic_male[HAPPINESS-1]++;
		}
	} while (FindNextFile(hfind, &ffd) != 0);

	/* sadness male */
	count_dynamic_male[SADNESS-1] = 0;
	swprintf_s(dir_name, L"dbase\\dynamic\\male\\sadness\\*");
	hfind = FindFirstFile(dir_name, &ffd);
	do {
		if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			count_dynamic_male[SADNESS-1]++;
		}
	} while (FindNextFile(hfind, &ffd) != 0);

	/* anger male */
	count_dynamic_male[ANGER-1] = 0;
	swprintf_s(dir_name, L"dbase\\dynamic\\male\\anger\\*");
	hfind = FindFirstFile(dir_name, &ffd);
	do {
		if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			count_dynamic_male[ANGER-1]++;
		}
	} while (FindNextFile(hfind, &ffd) != 0);

	/* fear male */
	count_dynamic_male[FEAR-1] = 0;
	swprintf_s(dir_name, L"dbase\\dynamic\\male\\fear\\*");
	hfind = FindFirstFile(dir_name, &ffd);
	do {
		if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			count_dynamic_male[FEAR-1]++;
		}
	} while (FindNextFile(hfind, &ffd) != 0);

	/* disgust male */
	count_dynamic_male[DISGUST-1] = 0;
	swprintf_s(dir_name, L"dbase\\dynamic\\male\\disgust\\*");
	hfind = FindFirstFile(dir_name, &ffd);
	do {
		if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			count_dynamic_male[DISGUST-1]++;
		}
	} while (FindNextFile(hfind, &ffd) != 0);

	/* surprise male */
	count_dynamic_male[SURPRISE-1] = 0;
	swprintf_s(dir_name, L"dbase\\dynamic\\male\\surprise\\*");
	hfind = FindFirstFile(dir_name, &ffd);
	do {
		if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			count_dynamic_male[SURPRISE-1]++;
		}
	} while (FindNextFile(hfind, &ffd) != 0);

	/* happiness female */
	count_dynamic_female[HAPPINESS-1] = 0;
	swprintf_s(dir_name, L"dbase\\dynamic\\female\\happiness\\*");
	hfind = FindFirstFile(dir_name, &ffd);
	do {
		if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			count_dynamic_female[HAPPINESS-1]++;
		}
	} while (FindNextFile(hfind, &ffd) != 0);

	/* sadness female */
	count_dynamic_female[SADNESS-1] = 0;
	swprintf_s(dir_name, L"dbase\\dynamic\\female\\sadness\\*");
	hfind = FindFirstFile(dir_name, &ffd);
	do {
		if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			count_dynamic_female[SADNESS-1]++;
		}
	} while (FindNextFile(hfind, &ffd) != 0);

	/* anger female */
	count_dynamic_female[ANGER-1] = 0;
	swprintf_s(dir_name, L"dbase\\dynamic\\female\\anger\\*");
	hfind = FindFirstFile(dir_name, &ffd);
	do {
		if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			count_dynamic_female[ANGER-1]++;
		}
	} while (FindNextFile(hfind, &ffd) != 0);

	/* fear female */
	count_dynamic_female[FEAR-1] = 0;
	swprintf_s(dir_name, L"dbase\\dynamic\\female\\fear\\*");
	hfind = FindFirstFile(dir_name, &ffd);
	do {
		if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			count_dynamic_female[FEAR-1]++;
		}
	} while (FindNextFile(hfind, &ffd) != 0);

	/* disgust female */
	count_dynamic_female[DISGUST-1] = 0;
	swprintf_s(dir_name, L"dbase\\dynamic\\female\\disgust\\*");
	hfind = FindFirstFile(dir_name, &ffd);
	do {
		if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			count_dynamic_female[DISGUST-1]++;
		}
	} while (FindNextFile(hfind, &ffd) != 0);

	/* surprise female */
	count_dynamic_female[SURPRISE-1] = 0;
	swprintf_s(dir_name, L"dbase\\dynamic\\female\\surprise\\*");
	hfind = FindFirstFile(dir_name, &ffd);
	do {
		if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			count_dynamic_female[SURPRISE-1]++;
		}
	} while (FindNextFile(hfind, &ffd) != 0);

	delay = 200;
	delay_emorec = 200;
	take_snapshot = false;
	emorec_save = false;

	/* emo dbs */
	jaffe_db = false;
	dynamic_db = true;
	expressio_db = true;
	return 0;
}

/* processes frame and updates it */
static int process_frame(HWND hwndDlg, cv::Mat frame) {

	char filename_buff[100];
	int gender_predict = 2, emo_predict = 2;
	double gender_confidence, emo_confidence;
	bool face_checked, emo_checked, eyes_checked, nose_checked, mouth_checked,
		 upface_checked, loface_checked, any;

	cv::Point p1, p2;
	cv::Scalar rectangle_color(242,186,122);
	cv::Mat frame_gray, face_image, face_image_bitmap;
	int thickness = 1, width, height, method;

	std::vector<cv::Rect> faces;
	std::vector<cv::Rect> eyes;
	std::vector<cv::Rect> noses;
	std::vector<cv::Rect> mouths;

	faces.reserve(10);
	eyes.reserve(10);
	noses.reserve(10);
	mouths.reserve(10);

	/* check current window */
	if (h_current == h_main) {

		/* what to display */
		face_checked   = is_face_checked(hwndDlg);
		emo_checked    = is_emo_checked(hwndDlg);
		eyes_checked   = is_eyes_checked(hwndDlg);
		nose_checked   = is_nose_checked(hwndDlg);
		mouth_checked  = is_mouth_checked(hwndDlg);
		upface_checked = is_upface_checked(hwndDlg);
		loface_checked = is_loface_checked(hwndDlg);
		any = face_checked || emo_checked || eyes_checked || nose_checked ||
			  mouth_checked || upface_checked || loface_checked;

		if (any) {

			/* get face */
			cv::cvtColor(frame, frame_gray, CV_BGR2GRAY);
			cv::equalizeHist(frame_gray, frame_gray);
			face_cascade.detectMultiScale(frame_gray, faces, 1.1, 3, 0, cv::Size(127,175), cv::Size(1280,720));
			for (int i = 0; i < faces.size(); i++) {
				face_image = frame_gray(faces[i]);
				if (face_checked) {
					p1.x = faces[i].x;
					p1.y = faces[i].y;
					p2.x = faces[i].x + faces[i].width;
					p2.y = faces[i].y + faces[i].height;
					rectangle(frame, p1, p2, rectangle_color, thickness);
				}
				if (eyes_checked) {
					eye_cascade.detectMultiScale(face_image, eyes, 1.4, 3, 0, cv::Size(45,45), cv::Size(60,60));
					for (int j = 0; j < eyes.size(); j++) {
						p1.x = eyes[j].x + faces[i].x;
						p1.y = eyes[j].y + faces[i].y;
						p2.x = p1.x + eyes[j].width;
						p2.y = p1.y + eyes[j].height;
						rectangle(frame, p1, p2, rectangle_color, thickness);
					}
				}
				if (nose_checked) {
					nose_cascade.detectMultiScale(face_image, noses, 1.1, 3, 0, cv::Size(55,55), cv::Size(70,70));
					if (noses.size() > 0) {  // some false positives
						p1.x = noses[0].x + faces[i].x;
						p1.y = noses[0].y + faces[i].y;
						p2.x = p1.x + noses[0].width;
						p2.y = p1.y + noses[0].height;
						rectangle(frame, p1, p2, rectangle_color, thickness);
					}
				}
				if (mouth_checked) {
					mouth_cascade.detectMultiScale(face_image, mouths, 1.1,40, 0, cv::Size(80,40), cv::Size(130,80));
					if (mouths.size() > 0) {  // too many false positives
						p1.x = mouths[0].x + faces[0].x;
						p1.y = mouths[0].y + faces[0].y;
						p2.x = p1.x + mouths[0].width;
						p2.y = p1.y + mouths[0].height;
						rectangle(frame, p1, p2, rectangle_color, thickness);
					}
				}
			}

			/* small face */
			if (face_checked && (faces.size() > 0)) {
				width = 128, height = 175, method = 1;
				if (get_face(&frame_gray, &face_image, width, height, method, &faces[0], &face_cascade) >= 0) {
					cv::cvtColor(face_image, face_image, CV_GRAY2BGR);
					draw_bitmap(hwndDlg, face_image, SMALL_IMAGE);
					update_panel(hwndDlg, SMALL_IMAGE);
				}
			}

			/* emotion */
			if (emo_checked && (faces.size() > 0)) {
				if ((get_ms_now() - start) >= delay) {
					width = 127, height = 175, method = 1;
					if (get_face(&frame_gray, &face_image, width, height, method, &faces[0], &face_cascade) >= 0) {
						gender_model->predict(face_image,gender_predict,gender_confidence);
						set_gender(h_current,gender_predict);
						// should take gender into account
						emo_model->predict(face_image,emo_predict,emo_confidence);
						set_emotion(h_current,emo_predict);
					}
					start = get_ms_now();
				}
			}
		}
	}
	else if (h_current == h_train) {

		/* get face */
		cv::cvtColor(frame, frame_gray, CV_BGR2GRAY);
		cv::equalizeHist(frame_gray, frame_gray);
		face_cascade.detectMultiScale(frame_gray, faces, 1.1, 3, 0, cv::Size(127,175), cv::Size(1280,720));

		/* draw a rectangle */
		face_checked = is_face_checked(hwndDlg);
		if (face_checked) {
			for (int i = 0; i < faces.size(); i++) {
				face_image = frame_gray(faces[i]);			
				p1.x = faces[i].x;
				p1.y = faces[i].y;
				p2.x = faces[i].x + faces[i].width;
				p2.y = faces[i].y + faces[i].height;
				rectangle(frame, p1, p2, rectangle_color, thickness);
			}
		}

		/* draw small face */
		if (faces.size() > 0) {
			width = 128, height = 175, method = 1;
			if (get_face(&frame_gray, &face_image, width, height, method, &faces[0], &face_cascade) >= 0) {
				cv::cvtColor(face_image, face_image, CV_GRAY2BGR);
				draw_bitmap(hwndDlg, face_image, SMALL_IMAGE);
				update_panel(hwndDlg, SMALL_IMAGE);
			}
		}

		/* save image to file */
		if (take_snapshot && (faces.size() > 0)) {
			width = 127, height = 175, method = 1;
			if (get_face(&frame_gray, &face_image, width, height, method, &faces[0], &face_cascade) >= 0) {
				if (train_male()) {
					if (train_happiness()){
						sprintf(filename_buff, "dbase/dynamic/male/happiness/%d.png", count_dynamic_male[HAPPINESS-1]);
						imwrite(filename_buff, face_image);
						count_dynamic_male[HAPPINESS-1]++;
					}
					else if (train_surprise()){
						sprintf(filename_buff, "dbase/dynamic/male/surprise/%d.png", count_dynamic_male[SURPRISE-1]);
						imwrite(filename_buff, face_image);
						count_dynamic_male[SURPRISE-1]++;
					}
					else if (train_fear()){
						sprintf(filename_buff, "dbase/dynamic/male/fear/%d.png", count_dynamic_male[FEAR-1]);
						imwrite(filename_buff, face_image);
						count_dynamic_male[FEAR-1]++;
					}
					else if (train_anger()){
						sprintf(filename_buff, "dbase/dynamic/male/anger/%d.png", count_dynamic_male[ANGER-1]);
						imwrite(filename_buff, face_image);
						count_dynamic_male[ANGER-1]++;
					}
					else if (train_disgust()){
						sprintf(filename_buff, "dbase/dynamic/male/disgust/%d.png", count_dynamic_male[DISGUST-1]);
						imwrite(filename_buff, face_image);
						count_dynamic_male[DISGUST-1]++;
					}
					else if (train_sadness()){
						sprintf(filename_buff, "dbase/dynamic/male/sadness/%d.png", count_dynamic_male[SADNESS-1]);
						imwrite(filename_buff, face_image);
						count_dynamic_male[SADNESS-1]++;
					}
				}
				else if (train_female()) {
					if (train_happiness()){
						sprintf(filename_buff, "dbase/dynamic/female/happiness/%d.png", count_dynamic_female[HAPPINESS-1]);
						imwrite(filename_buff, face_image);
						count_dynamic_female[HAPPINESS-1]++;
					}
					else if (train_surprise()){
						sprintf(filename_buff, "dbase/dynamic/female/surprise/%d.png", count_dynamic_female[SURPRISE-1]);
						imwrite(filename_buff, face_image);
						count_dynamic_female[SURPRISE-1]++;
					}
					else if (train_fear()){
						sprintf(filename_buff, "dbase/dynamic/female/fear/%d.png", count_dynamic_female[FEAR-1]);
						imwrite(filename_buff, face_image);
						count_dynamic_female[FEAR-1]++;
					}
					else if (train_anger()){
						sprintf(filename_buff, "dbase/dynamic/female/anger/%d.png", count_dynamic_female[ANGER-1]);
						imwrite(filename_buff, face_image);
						count_dynamic_female[ANGER-1]++;
					}
					else if (train_disgust()){
						sprintf(filename_buff, "dbase/dynamic/female/disgust/%d.png", count_dynamic_female[DISGUST-1]);
						imwrite(filename_buff, face_image);
						count_dynamic_female[DISGUST-1]++;
					}
					else if (train_sadness()){
						sprintf(filename_buff, "dbase/dynamic/female/sadness/%d.png", count_dynamic_female[SADNESS-1]);
						imwrite(filename_buff, face_image);
						count_dynamic_female[SADNESS-1]++;
					}
				}
				take_snapshot = false;
			}
		}
	}
	else if (h_current == h_emorec) {

		/* get face */
		cv::cvtColor(frame, frame_gray, CV_BGR2GRAY);
		cv::equalizeHist(frame_gray, frame_gray);
		face_cascade.detectMultiScale(frame_gray, faces, 1.1, 3, 0, cv::Size(127,175), cv::Size(1280,720));

		if (faces.size() > 0) {
			if ((get_ms_now() - start) >= delay_emorec) {
				width = 127, height = 175, method = 1;
				if (get_face(&frame_gray, &face_image, width, height, method, &faces[0], &face_cascade) >= 0) {

					// todo this can be deleted
					gender_model->predict(face_image,gender_predict,gender_confidence);
					set_gender(h_current,gender_predict); //hwndDlg

					// should take gender into account
					emo_model->predict(face_image,emo_predict,emo_confidence);
					set_emotion(h_current,emo_predict);

					// also save it
					if (emorec_save)
						emorec_emotions.push_back(emo_predict);
				}
				start = get_ms_now();
			}
		}
	}

	// todo change sintax; hwndDlg is not needed
	draw_bitmap(hwndDlg, frame, BIG_IMAGE);
	update_panel(hwndDlg, BIG_IMAGE);

	return 0;
}

/* captures video from screen and displays it to hwnd */
int capture_video_opencv(HWND hwndDlg) {

	int device;
	cv::Mat frame;

	device = get_checked(GetSubMenu(GetMenu(hwndDlg),1));

	// cheap hack - change this later TODO
	if ((GetMenuItemCount(GetSubMenu(GetMenu(hwndDlg),1)) > 1) && (device < 2))
		device = 1 - device;

	set_status(hwndDlg,L"Initializing ...");

	/* video capture */
	cv::VideoCapture capture(device);
	if (!capture.isOpened())
		return -1;

	g_disconnected = false;
	set_status(hwndDlg,L"Streaming");
	start = get_ms_now() - delay;

	/* main loop */
	while (!g_stop) {

		capture >> frame;
		if (frame.empty())
			return -1;
		
		/* process it */
		process_frame(hwndDlg,frame);
	}

	set_status(hwndDlg,L"Stopped");
	return 0;
}

/* helper function for train_emotions */
static void read_images(std::string base_filename, const TCHAR* dir_name, int label, cv::vector<cv::Mat> &images, cv::vector<int> &labels) {

	HANDLE hfind;
	WIN32_FIND_DATA ffd;

	cv::Mat aux;
	std::wstring aux_filename;
	std::string filename;

	hfind = FindFirstFile(dir_name, &ffd);
	do {
		if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			aux_filename = std::wstring(ffd.cFileName);
			filename = base_filename + std::string(aux_filename.begin(), aux_filename.end());
			aux = cv::imread(filename, CV_LOAD_IMAGE_GRAYSCALE);
			if (aux.empty())
				continue;
			images.push_back(aux);
			labels.push_back(label);
			filename.clear();
			aux_filename.clear();
		}
	} while (FindNextFile(hfind, &ffd) != 0);
}

/* trains emotion model with everything under "dbase/" */
void train_emotions(void) {
	
	cv::vector<cv::Mat> images;
	cv::vector<int> labels;

	/* read images */

	/* jaffe */
	if (jaffe_db) {
		read_images("dbase/jaffe/anger/",L"dbase\\jaffe\\anger\\*", ANGER, images, labels);
		read_images("dbase/jaffe/disgust/",L"dbase\\jaffe\\disgust\\*", DISGUST, images, labels);
		read_images("dbase/jaffe/fear/",L"dbase\\jaffe\\fear\\*", FEAR, images, labels);
		read_images("dbase/jaffe/happiness/",L"dbase\\jaffe\\happiness\\*", HAPPINESS, images, labels);
		read_images("dbase/jaffe/sadness/",L"dbase\\jaffe\\sadness\\*", SADNESS, images, labels);
		read_images("dbase/jaffe/surprise/",L"dbase\\jaffe\\surprise\\*", SURPRISE, images, labels);
	}

	/* expressio - female */
	if (expressio_db) {
		read_images("dbase/expressio/female/anger/",L"dbase\\expressio\\female\\anger\\*", ANGER, images, labels);
		read_images("dbase/expressio/female/disgust/",L"dbase\\expressio\\female\\disgust\\*", DISGUST, images, labels);
		read_images("dbase/expressio/female/fear/",L"dbase\\expressio\\female\\fear\\*", FEAR, images, labels);
		read_images("dbase/expressio/female/happiness/",L"dbase\\expressio\\female\\happiness\\*", HAPPINESS, images, labels);
		read_images("dbase/expressio/female/sadness/",L"dbase\\expressio\\female\\sadness\\*", SADNESS, images, labels);
		read_images("dbase/expressio/female/surprise/",L"dbase\\expressio\\female\\surprise\\*", SURPRISE, images, labels);
	}

	/* dynamic - female */
	if (dynamic_db) {
		read_images("dbase/dynamic/female/anger/",L"dbase\\dynamic\\female\\anger\\*", ANGER, images, labels);
		read_images("dbase/dynamic/female/disgust/",L"dbase\\dynamic\\female\\disgust\\*", DISGUST, images, labels);
		read_images("dbase/dynamic/female/fear/",L"dbase\\dynamic\\female\\fear\\*", FEAR, images, labels);
		read_images("dbase/dynamic/female/happiness/",L"dbase\\dynamic\\female\\happiness\\*", HAPPINESS, images, labels);
		read_images("dbase/dynamic/female/sadness/",L"dbase\\dynamic\\female\\sadness\\*", SADNESS, images, labels);
		read_images("dbase/dynamic/female/surprise/",L"dbase\\dynamic\\female\\surprise\\*", SURPRISE, images, labels);
	}


	/* males */

	/* expressio - male */
	if (expressio_db) {
		read_images("dbase/expressio/male/anger/",L"dbase\\expressio\\male\\anger\\*", ANGER, images, labels);
		read_images("dbase/expressio/male/disgust/",L"dbase\\expressio\\male\\disgust\\*", DISGUST, images, labels);
		read_images("dbase/expressio/male/fear/",L"dbase\\expressio\\male\\fear\\*", FEAR, images, labels);
		read_images("dbase/expressio/male/happiness/",L"dbase\\expressio\\male\\happiness\\*", HAPPINESS, images, labels);
		read_images("dbase/expressio/male/sadness/",L"dbase\\expressio\\male\\sadness\\*", SADNESS, images, labels);
		read_images("dbase/expressio/male/surprise/",L"dbase\\expressio\\male\\surprise\\*", SURPRISE, images, labels);
	}

	/* dynamic - male */
	if (dynamic_db) {
		read_images("dbase/dynamic/male/anger/",L"dbase\\dynamic\\male\\anger\\*", ANGER, images, labels);
		read_images("dbase/dynamic/male/disgust/",L"dbase\\dynamic\\male\\disgust\\*", DISGUST, images, labels);
		read_images("dbase/dynamic/male/fear/",L"dbase\\dynamic\\male\\fear\\*", FEAR, images, labels);
		read_images("dbase/dynamic/male/happiness/",L"dbase\\dynamic\\male\\happiness\\*", HAPPINESS, images, labels);
		read_images("dbase/dynamic/male/sadness/",L"dbase\\dynamic\\male\\sadness\\*", SADNESS, images, labels);
		read_images("dbase/dynamic/male/surprise/",L"dbase\\dynamic\\male\\surprise\\*", SURPRISE, images, labels);
	}

	/* train model */
	set_status_train(L"Training ...");
	EnableWindow(h_train, FALSE);
	emo_model->train(images,labels);

	/* save it */
	set_status_train(L"Saving ...");
	emo_model->save("models/emo.yml");
	set_status_train(L"OK");
}

/* captures video from creative camera and displays it on screen using util capture (2) & face render interfaces */
int capture_video_util_capture(HWND hwndDlg) {

	/* get frames from selected device in menu */
	UtilCapture uc(g_session);
	uc.SetFilter(get_checked_device(hwndDlg));

	/* enable video in rgb24 */
	PXCCapture::VideoStream::DataDesc req;
	memset(&req,0,sizeof(req));
	req.streams[0].format = PXCImage::COLOR_FORMAT_RGB24;
	uc.LocateStreams(&req);

	/* initialize face analysis module */
	PXCFaceAnalysis *face = NULL;
	init_face_analysis(&face, g_session);

	cv::Mat* image_cv;
	PXCImage* image;
	PXCSmartSP sp;

	g_disconnected = false;
	set_status(hwndDlg,L"Streaming");
	start = get_ms_now() - delay;
	while (!g_stop) {

		/* get frame */
		uc.QueryVideoStream(0)->ReadStreamAsync(&image,&sp);
		sp->Synchronize();
		
		/* convert it to cv format & process it */
		convert_pxc_to_cv(image, &image_cv);
		process_frame(hwndDlg, (*image_cv));
		
		/* free memory */
		delete image_cv;
		image->Release();
	}

	set_status(hwndDlg,L"Stopped");
	return 0;
}

/* crops the face from "src" to "dst"; the resulting image will have "width" x "height"; if face is not specified it will use already trained classifier or NULL (load from file) */
int get_face(cv::Mat* src, cv::Mat* dst, int width, int height, int method, cv::Rect* face, cv::CascadeClassifier* face_cascade) {

	bool null_cascade, null_face;
	cv::Mat src_gray;

	if (src == NULL || dst == NULL || width <= 0 || height <= 0)
		return -1;

	if ((*src).channels() == 1)
		src_gray = (*src);
	else if ((*src).channels() == 3) {
		cv::cvtColor((*src), src_gray, CV_BGR2GRAY);
		cv::equalizeHist(src_gray, src_gray);
	}
	else
		return -1;

	null_cascade = null_face = false;

	if (face == NULL) {  /* not recommended on video */

		std::vector<cv::Rect> faces;
		faces.reserve(10);
		null_face = true;

		if (face_cascade == NULL) {
			face_cascade = new cv::CascadeClassifier();
			null_cascade = true;
			if (!face_cascade->load("models/face.xml")) {
				delete face_cascade;
				return -1;
			}
		}
		face_cascade->detectMultiScale(src_gray, faces, 1.1, 3, 0, cv::Size(width,height), cv::Size(1280,720));
		if (faces.size() < 1) {
			if (null_cascade == true)
				delete face_cascade;
			return -1;
		}

		/* just first face */
		face = new cv::Rect();
		face->x = faces[0].x;
		face->y = faces[0].y;
		face->width = faces[0].width;
		face->height = faces[0].height;
	}

	int mid_x, mid_y;
	cv::Rect face_rect;

	mid_x = (*face).x + ((*face).width/2);
	mid_y = (*face).y + ((*face).height/2);

	if (method == 1) {  /* take height, then keep ratio */
		face_rect.height = (*face).height;
		face_rect.width = (int)(((float)width/height) * face_rect.height);
		face_rect.x = mid_x - (face_rect.width/2);
		face_rect.y = mid_y - (face_rect.height/2);
		(*dst) = src_gray(face_rect);
		resize ((*dst), (*dst), cv::Size(width,height), 0, 0, cv::INTER_LINEAR);	
	}
	
	else if (method == 2) {  /* take closest multiple */
		int k = (*face).height / height;
		face_rect.height = k * height;
		face_rect.width = k * width;
		face_rect.x = mid_x - (face_rect.width/2);
		face_rect.y = mid_y - (face_rect.height/2);
		(*dst) = src_gray(face_rect);
		resize ((*dst), (*dst), cv::Size(width,height), 0, 0, cv::INTER_LINEAR);	
	}

	else if (method == 3) {  /* start from nose, get exactly width x height */
		face_rect.height = height;
		face_rect.width = width;
		face_rect.x = mid_x - (face_rect.width/2);
		face_rect.y = mid_y - (face_rect.height/2);
		(*dst) = src_gray(face_rect);
	}

	else return -1;

	if (null_cascade == true)
		delete face_cascade;
	if (null_face == true)
		delete face;

	return 0;
}

/* converts pxc image to opencv image (bgr) */
void convert_pxc_to_cv(PXCImage* src, cv::Mat** dst) {
	
	int i, row, col;
	cv::Mat* image_cv;

	/* get pxc info */
	PXCImage::ImageInfo info;
	PXCImage::ImageData data;

	src->QueryInfo(&info);
	if (info.format == PXCImage::COLOR_FORMAT_RGB24)
		image_cv = new cv::Mat(info.height,info.width,CV_8UC3);
	else if (info.format == PXCImage::COLOR_FORMAT_GRAY)
		image_cv = new cv::Mat(info.height,info.width,CV_8UC1);
	else {
		MessageBoxW(0,L"Unsupported src format (must be RGB24 or Grayscale)",L"Error",MB_ICONEXCLAMATION|MB_OK);
		return;
	}

	/* get pxc pixel values */
	if (info.format == PXCImage::COLOR_FORMAT_RGB24)
		src->AcquireAccess(PXCImage::ACCESS_READ, PXCImage::COLOR_FORMAT_RGB24, &data);
	else if (info.format == PXCImage::COLOR_FORMAT_GRAY)
		src->AcquireAccess(PXCImage::ACCESS_READ, PXCImage::COLOR_FORMAT_GRAY, &data);

	i = 0;
	for (row = 0; row < image_cv->rows; row++)
		for (col = 0; col < image_cv->cols; col++) {
			if (info.format == PXCImage::COLOR_FORMAT_RGB24) {
				image_cv->at<cv::Vec3b>(row,col).val[0] = data.planes[0][i];
				image_cv->at<cv::Vec3b>(row,col).val[1] = data.planes[0][i+1];
				image_cv->at<cv::Vec3b>(row,col).val[2] = data.planes[0][i+2];
				i = i + 3;
			}
			else if (info.format == PXCImage::COLOR_FORMAT_GRAY) {
				image_cv->at<uchar>(row,col) = data.planes[0][i];
				i++;
			}
		}

	src->ReleaseAccess(&data);
	(*dst) = image_cv;
}

/* maps emotion to color */
static cv::Scalar emotion_color(int emotion) {
	if (emotion == HAPPINESS)
		return cv::Scalar(84,255,255);
	if (emotion == FEAR)
		return cv::Scalar(0,150,0);
	if (emotion == SURPRISE)
		return cv::Scalar(255,189,89);
	if (emotion == SADNESS)
		return cv::Scalar(255,81,81);
	if (emotion == ANGER)
		return cv::Scalar(0,0,255);
	if (emotion == DISGUST)
		return cv::Scalar(255,84,255);
	return cv::Scalar(0,0,0);
}

/* draws legend to graph image */
static void draw_legend(cv::Mat& graph_image) {
	
	cv::Point p1, p2;

	/* happiness */
	p1.x = 20;
	p1.y = 20;
	p2.x = 35;
	p2.y = 35;
	cv::rectangle(graph_image, p1, p2, emotion_color(HAPPINESS), CV_FILLED);
	cv::putText(graph_image, "Happiness", cv::Point(40,35), cv::FONT_HERSHEY_SIMPLEX, .6, cv::Scalar(255,255,255), 1, 8, false);

	/* fear */
	p1.x = 20;
	p1.y = 45;
	p2.x = 35;
	p2.y = 60;
	cv::rectangle(graph_image, p1, p2, emotion_color(FEAR), CV_FILLED);
	cv::putText(graph_image, "Fear", cv::Point(40,60), cv::FONT_HERSHEY_SIMPLEX, .6, cv::Scalar(255,255,255), 1, 8, false);

	/* surprise */
	p1.x = 20;
	p1.y = 70;
	p2.x = 35;
	p2.y = 85;
	cv::rectangle(graph_image, p1, p2, emotion_color(SURPRISE), CV_FILLED);
	cv::putText(graph_image, "Surprise", cv::Point(40,85), cv::FONT_HERSHEY_SIMPLEX, .6, cv::Scalar(255,255,255), 1, 8, false);

	/* sadness */
	p1.x = 160;
	p1.y = 20;
	p2.x = 175;
	p2.y = 35;
	cv::rectangle(graph_image, p1, p2, emotion_color(SADNESS), CV_FILLED);
	cv::putText(graph_image, "Sadness", cv::Point(180,35), cv::FONT_HERSHEY_SIMPLEX, .6, cv::Scalar(255,255,255), 1, 8, false);

	/* disgust */
	p1.x = 160;
	p1.y = 45;
	p2.x = 175;
	p2.y = 60;
	cv::rectangle(graph_image, p1, p2, emotion_color(DISGUST), CV_FILLED);
	cv::putText(graph_image, "Disgust", cv::Point(180,60), cv::FONT_HERSHEY_SIMPLEX, .6, cv::Scalar(255,255,255), 1, 8, false);

	/* anger */
	p1.x = 160;
	p1.y = 70;
	p2.x = 175;
	p2.y = 85;
	cv::rectangle(graph_image, p1, p2, emotion_color(ANGER), CV_FILLED);
	cv::putText(graph_image, "Anger", cv::Point(180,85), cv::FONT_HERSHEY_SIMPLEX, .6, cv::Scalar(255,255,255), 1, 8, false);
}

/* draws graph in image */
static void draw_graph(cv::Mat& graph_image) {

	double delta = (double)graph_image.cols/emorec_emotions.size();
	for (int i = 0; i < emorec_emotions.size(); i++) {
		cv::rectangle(graph_image, cv::Point((int)(delta*i),250), cv::Point((int)(delta*(i+1)),350), emotion_color(emorec_emotions.at(i)), CV_FILLED);
	}
}

/* shows the emotion history */
void show_emotion_graph(void) {

	int graph_w = 512, graph_h = 400;
	cv::Mat graph_image(graph_h, graph_w, CV_8UC3, cv::Scalar(0,0,0));

	/* legend */
	draw_legend(graph_image);

	/* graph */
	draw_graph(graph_image);

	/* show image */
	cv::namedWindow("Emotions History", CV_WINDOW_AUTOSIZE);
	cv::imshow("Emotions History", graph_image);
	cv::waitKey(0);
}

/* saves emotion history to file */
void save_emotion_graph(void) {

	int graph_w = 512, graph_h = 400;
	cv::Mat graph_image(graph_h, graph_w, CV_8UC3, cv::Scalar(0,0,0));

	/* legend */
	draw_legend(graph_image);

	/* graph */
	draw_graph(graph_image);

	/* saves image */
	cv::imwrite("graph.png", graph_image);
}


/* returns current time in ms */
static long long get_ms_now(void) {
	static LARGE_INTEGER freq;
	static BOOL qpc = QueryPerformanceFrequency(&freq);
	if (qpc) {
		LARGE_INTEGER now;
		QueryPerformanceCounter(&now);
		return (1000LL * now.QuadPart) / freq.QuadPart;
	} 
	else {
		return GetTickCount();
	}
}

/* -------------------------------------------------------------------------------------------------------------- */

/* captures video from creative camera and displays it on screen using util pipeline (1) & face render interfaces */
int capture_video_util_pipeline(HWND hwndDlg) {

	int a, i;
	bool ret;
	pxcStatus stat;
	
	PXCImage* img;
	PXCFaceAnalysis* face;

	UtilPipeline up;

	up.EnableImage(PXCImage::COLOR_FORMAT_RGB24);
	up.EnableFaceLocation();
	up.EnableFaceLandmark();

	/* init */
	ret = up.Init();
	if (ret == false) 
		return -1;

	/* mark it as connected */
	g_disconnected = false;
	/* set detection & landmark settings */
	face = up.QueryFace();
	
	/* detection profile */
	PXCFaceAnalysis::Detection *det = face->DynamicCast<PXCFaceAnalysis::Detection>();
	PXCFaceAnalysis::Detection::ProfileInfo dinfo;
	det->QueryProfile(0,&dinfo);
	det->SetProfile(&dinfo);

	/* landmark profile */
	PXCFaceAnalysis::Landmark* landmark = face->DynamicCast<PXCFaceAnalysis::Landmark>();
	PXCFaceAnalysis::Landmark::ProfileInfo linfo;
	landmark->QueryProfile(1,&linfo);
	landmark->SetProfile(&linfo);

	/* loop */
	while (!g_stop) {

		/* acquire lock */
		ret = up.AcquireFrame(true);
		if (ret == false)
			break;

		/* process frames */
		img = up.QueryImage(PXCImage::COLOR_FORMAT_RGB24);
		face = up.QueryFace();

		/* reder face & info */
		pxcUID face_id;
		PXCFaceAnalysis::Detection* det;
		PXCFaceAnalysis::Detection::Data face_data;
		PXCFaceAnalysis::Landmark* landmark;
		
		det = face->DynamicCast<PXCFaceAnalysis::Detection>();
		landmark = face->DynamicCast<PXCFaceAnalysis::Landmark>();

		face->QueryFace(0, &face_id);
		det->QueryData(face_id, &face_data);

		draw_bitmap(hwndDlg,img);
		update_panel(hwndDlg, BIG_IMAGE);

		/* release lock */
		ret = up.ReleaseFrame();
		if (ret == false)
			break;
	}

	up.Close();
	up.Release();
	return 0;
}

/* initialize face analysis module to detect both face and landmark with 7 points */
void init_face_analysis(PXCFaceAnalysis** face, PXCSession* ses) {

	/* instantiate face analysis module */
	ses->CreateImpl<PXCFaceAnalysis>(face);
	PXCFaceAnalysis::ProfileInfo pinfo;
	(*face)->QueryProfile(0,&pinfo);
	(*face)->SetProfile(&pinfo);
	
	/* enable face detection */
	PXCFaceAnalysis::Detection *det = (*face)->DynamicCast<PXCFaceAnalysis::Detection>();
	PXCFaceAnalysis::Detection::ProfileInfo dinfo;
	det->QueryProfile(0,&dinfo);
	det->SetProfile(&dinfo);

	/* enable landmark detection */
	PXCFaceAnalysis::Landmark* landmark = (*face)->DynamicCast<PXCFaceAnalysis::Landmark>();
	PXCFaceAnalysis::Landmark::ProfileInfo linfo;
	landmark->QueryProfile(1,&linfo);
	landmark->SetProfile(&linfo);
}

/* converts opencv image to pxc image (bgr) */
void convert_cv_to_pxc(cv::Mat src, PXCImage** dst, PXCSession* ses) {

	int i;

	/* create new PXCAccelerator; atl : query from session (doesn't exist) */
	PXCSmartPtr<PXCAccelerator> accelerator;
	ses->CreateAccelerator(PXCAccelerator::ACCEL_TYPE_CPU, &accelerator);

	/* fill in basic info */
	PXCImage::ImageInfo info;
	PXCImage::ImageData data;

	memset(&info, 0, sizeof(info));
	if (src.channels() == 3)
		info.format = PXCImage::COLOR_FORMAT_RGB24;
	else if (src.channels() == 1)
		info.format = PXCImage::COLOR_FORMAT_GRAY;
	else {
		wprintf(L"Unsupported src format (not rgb24/gray)\n");
		return;
	}
	info.height = src.rows;
	info.width = src.cols;

	/* create PXCImage */
	accelerator->CreateImage(&info, 0, 0, dst);

	/* fill in pixel values */
	memset(&data, 0, sizeof(data));
	if (src.channels() == 3)
		data.format = PXCImage::COLOR_FORMAT_RGB24;
	else if (src.channels() == 1)
		data.format = PXCImage::COLOR_FORMAT_GRAY;
	data.type = PXCImage::SURFACE_TYPE_SYSTEM_MEMORY;	
	data.pitches[0] = src.cols * src.channels(); // ALIGN16(info.width*4)*info.height;
												 // __declspec(align(16))
	/* setting pixel values - one at a time */
	if (src.channels() == 3)
		(*dst)->AcquireAccess(PXCImage::ACCESS_WRITE, PXCImage::COLOR_FORMAT_RGB24, &data);
	else if (src.channels() == 1)
		(*dst)->AcquireAccess(PXCImage::ACCESS_WRITE, PXCImage::COLOR_FORMAT_GRAY, &data);
	
	int row, col, max_value, delta;
	i = 0, row = 0, col = 0;
	max_value = info.width * info.height * src.channels();
	while (true) {
		
		if (src.channels() == 3) {
			data.planes[0][i] = src.at<cv::Vec3b>(row,col).val[0];
			data.planes[0][i+1] = src.at<cv::Vec3b>(row,col).val[1];
			data.planes[0][i+2] = src.at<cv::Vec3b>(row,col).val[2];
			i = i + 3;
		}
		else if (src.channels() == 1) {
			data.planes[0][i] = src.at<uchar>(row,col);
			i++;
		}

		if (i >= max_value)
			break;

		if (col == (src.cols - 1)) {
			col = 0;
			row++;
		} else { col++; }
	}

	(*dst)->ReleaseAccess(&data);
}

/* old code */
static bool DisplayDeviceConnection(HWND hwndDlg, bool state) {
    if (state) {
        if (!g_disconnected) set_status(hwndDlg,L"Device Disconnected");
        g_disconnected = true;
    } else {
        if (g_disconnected) set_status(hwndDlg, L"Device Reconnected");
        g_disconnected = false;
    }
    return g_disconnected;
}

/* old code */
class MyUtilPipeline: public UtilPipeline {
public:
	int pidx;
	MyUtilPipeline(void):UtilPipeline() {}
	MyUtilPipeline(pxcCHAR *file,bool record):UtilPipeline(0,file,record) {}
	virtual void OnFaceLandmarkSetup(PXCFaceAnalysis::Landmark::ProfileInfo *pinfo) {
		QueryFace()->DynamicCast<PXCFaceAnalysis::Landmark>()->QueryProfile(pidx,pinfo);
	}
};

/* old util pipeline video capture */
void simple_pipeline(HWND hwndDlg) {

	MyUtilPipeline *pp = 0;

	/* Set Mode & Source */
	if (GetRecordState(hwndDlg)) {
		pp = new MyUtilPipeline (g_file,true);
		pp->QueryCapture()->SetFilter(get_checked_device(hwndDlg));
	} else if (GetPlaybackState(hwndDlg)) {
		pp = new MyUtilPipeline(g_file,false);
	} else {
		pp = new MyUtilPipeline();
		pp->QueryCapture()->SetFilter(get_checked_device(hwndDlg));
	}
	bool sts = true;

	/* Set Module */
	pp->EnableFaceLocation(GetCheckedModule(hwndDlg));
	pp->EnableFaceLandmark(GetCheckedModule(hwndDlg));

	/* Init */
	set_status(hwndDlg,L"Init Started");
	if (pp->Init()) {
		set_status(hwndDlg,L"Streaming");
		g_disconnected = false;

		while (!g_stop) {
			if (!pp->AcquireFrame(true)) break;
            if (!DisplayDeviceConnection(hwndDlg, pp->IsDisconnected())) {
				PXCFaceAnalysis *face=pp->QueryFace();
				draw_bitmap(hwndDlg,pp->QueryImage(PXCImage::IMAGE_TYPE_COLOR));
				update_panel(hwndDlg,BIG_IMAGE);
			}
			pp->ReleaseFrame();
		}

	} else {
		set_status(hwndDlg,L"Init Failed");
		sts = false;
	}

	pp->Close();
	pp->Release();
	if (sts) set_status(hwndDlg,L"Stopped");
}

/* old util capture video capture */
void advanced_pipeline(HWND hwndDlg) {

    PXCSmartPtr<PXCSession> session;
    pxcStatus sts=PXCSession_Create(&session);
    if (sts<PXC_STATUS_NO_ERROR) {
        set_status(hwndDlg,L"Failed to create an SDK session");
        return;
	}

    /* Set Module */
    PXCSession::ImplDesc desc;
	memset(&desc,0,sizeof(desc));
	wcscpy_s<sizeof(desc.friendlyName)/sizeof(pxcCHAR)>(desc.friendlyName,GetCheckedModule(hwndDlg));

    PXCSmartPtr<PXCFaceAnalysis> face;
    sts=session->CreateImpl<PXCFaceAnalysis>(&desc,&face);
    if (sts < PXC_STATUS_NO_ERROR) {
        set_status(hwndDlg,L"Failed to create the face module");
        return;
    }

    /* Set Source */
    PXCSmartPtr<UtilCapture> capture;
	if (GetRecordState(hwndDlg)) {
		capture = new UtilCaptureFile(session,g_file,true);
        capture->SetFilter(get_checked_device(hwndDlg));
	} else if (GetPlaybackState(hwndDlg)) {
		capture = new UtilCaptureFile(session,g_file,false);
	} else {
		capture = new UtilCapture(session);
        capture->SetFilter(get_checked_device(hwndDlg));
	}

	set_status(hwndDlg,L"Pair module with I/O");
    for (int i=0;;i++) {
        PXCFaceAnalysis::ProfileInfo pinfo;
        sts=face->QueryProfile(i,&pinfo);
        if (sts < PXC_STATUS_NO_ERROR) break;
        sts=capture->LocateStreams(&pinfo.inputs);
        if (sts < PXC_STATUS_NO_ERROR) continue;
        sts=face->SetProfile(&pinfo);
        if (sts >= PXC_STATUS_NO_ERROR) break;
    }
    if (sts<PXC_STATUS_NO_ERROR) {
        set_status(hwndDlg,L"Failed to pair the face module with I/O");
        return;
    }

	/* Set Detection Profile */
	PXCFaceAnalysis::Detection::ProfileInfo pinfo1;
	PXCFaceAnalysis::Detection *faced=face->DynamicCast<PXCFaceAnalysis::Detection>();
	faced->QueryProfile(0,&pinfo1);
	faced->SetProfile(&pinfo1);

	/* Set Landmark Profile */
	PXCFaceAnalysis::Landmark::ProfileInfo pinfo2;
	PXCFaceAnalysis::Landmark *facel=face->DynamicCast<PXCFaceAnalysis::Landmark>();

    set_status(hwndDlg,L"Streaming");
    PXCSmartArray<PXCImage> images;
    PXCSmartSPArray sps(2);
    while (!g_stop) {
        sts=capture->ReadStreamAsync(images.ReleaseRefs(),sps.ReleaseRef(0));
        if (DisplayDeviceConnection(hwndDlg,sts==PXC_STATUS_DEVICE_LOST)) continue;
        if (sts<PXC_STATUS_NO_ERROR) break;

        sts=face->ProcessImageAsync(images,sps.ReleaseRef(1));
        if (sts<PXC_STATUS_NO_ERROR) break;

        sps.SynchronizeEx();
		sts=sps[0]->Synchronize(0);
        if (DisplayDeviceConnection(hwndDlg,sts==PXC_STATUS_DEVICE_LOST)) continue;
        if (sts<PXC_STATUS_NO_ERROR) break;

        /* Display Results */
        draw_bitmap(hwndDlg,capture->QueryImage(images,PXCImage::IMAGE_TYPE_COLOR));
        update_panel(hwndDlg,BIG_IMAGE);
    }
    set_status(hwndDlg,L"Stopped");
}