#include "pch.h"

using namespace cv;
using namespace std;
using namespace std::chrono;

mutex mu;
Mat templ = Mat::zeros(1, 1, CV_8UC1);
int xS, yS, frame, f2, match_method = 3;		
char win[] = "output";
float scale = 1.0f, thresh = 0.995f;

vector<Point> pt;
bool stop = false;
time_point<system_clock> refz;

struct MaxVals {
	Point point;
	float val;
};

Mat hwnd2mat(HWND hwnd)
{
	HDC hwindowDC, hwindowCompatibleDC;

	int height, width, srcheight, srcwidth;
	HBITMAP hbwindow;
	Mat src;
	BITMAPINFOHEADER  bi;

	hwindowDC = GetDC(hwnd);
	hwindowCompatibleDC = CreateCompatibleDC(hwindowDC);
	SetStretchBltMode(hwindowCompatibleDC, COLORONCOLOR);

	RECT windowsize;    // get the height and width of the screen
	GetClientRect(hwnd, &windowsize);

	srcheight = windowsize.bottom;
	srcwidth = windowsize.right;
	height = windowsize.bottom / 1;  //change this to whatever size you want to resize to
	width = windowsize.right / 1;

	src.create(height, width, CV_8UC4);

	// create a bitmap
	hbwindow = CreateCompatibleBitmap(hwindowDC, width, height);
	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = width;
	bi.biHeight = -height;  //this is the line that makes it draw upside down or not
	bi.biPlanes = 1;
	bi.biBitCount = 32;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed = 0;
	bi.biClrImportant = 0;

	// use the previously created device context with the bitmap
	SelectObject(hwindowCompatibleDC, hbwindow);
	// copy from the window device context to the bitmap device context
	StretchBlt(hwindowCompatibleDC, 0, 0, width, height, hwindowDC, 0, 0, srcwidth, srcheight, SRCCOPY); //change SRCCOPY to NOTSRCCOPY for wacky colors !
	GetDIBits(hwindowCompatibleDC, hbwindow, 0, height, src.data, (BITMAPINFO *)&bi, DIB_RGB_COLORS);  //copy from hwindowCompatibleDC to hbwindow

	// avoid memory leak
	DeleteObject(hbwindow);
	DeleteDC(hwindowCompatibleDC);
	ReleaseDC(hwnd, hwindowDC);
	
	resize(src, src, Size(xS, yS));
	cvtColor(src, src, CV_BGRA2BGR);
	return src;
}

void mouse_callback(int  event, int  x, int  y, int  flag, void *param)
{
	if (event == EVENT_LBUTTONDOWN)	
		pt.push_back(Point(x, y));
	
}

void newTemplate(Mat img)
{
	if (pt.size() == 2)
	{
		templ.release();
		templ = img(Rect(pt[0], pt[1]));
		pt.clear();
	}
}

void FPSPrint(Mat imgBGR)
{
	if (duration_cast<milliseconds>(system_clock::now() - refz).count() >= 1000){	
		f2 = frame;
		frame = 0;
		refz = system_clock::now();
	}
	putText(imgBGR, "FPS:" + to_string(f2) + " Thresh:" + to_string(thresh).substr(0, 5), Point(5, 20), 
		FONT_HERSHEY_DUPLEX, 0.7, Scalar(0, 0, 0), 3, 11);
	putText(imgBGR, "FPS:" + to_string(f2) + " Thresh:" + to_string(thresh).substr(0, 5), Point(5, 20), 
		FONT_HERSHEY_DUPLEX, 0.7, Scalar(255, 255, 255), 2, 11);
}

bool compareMethod(MaxVals x, MaxVals y) {
	return (x.val < y.val);
}

void MatchingMethod()
{
	int y, x, pos = 0;
	Mat grey, imgBGR;
	string outFloat;
	Mat result = Mat(0, 0, CV_32FC1);
	HWND hwndDesktop = GetDesktopWindow();
	MaxVals crt;
	vector<MaxVals> max;	

	while (!stop) {
		try {
			imgBGR = hwnd2mat(hwndDesktop);
			cvtColor(imgBGR, grey, CV_BGR2GRAY);
			matchTemplate(grey, templ, result, match_method);
						
			for (y = 0; y < result.rows; y++)
				for (x = 0; x < result.cols; x++) 
				{
					if ((crt.val = result.at<float>(y, x)) >= thresh)
					{
						crt.point = Point(x, y);
						max.push_back(crt);
					}	
				}
			sort(max.begin(), max.end(), compareMethod);
			if(max.size() > 30)
				max.erase(max.begin(),max.end()-15);
			for(pos = 0;pos<max.size();pos++)
				cout << max[pos].val << " " << max[pos].point << endl;
			if(pos != 0)
				cout << endl;
		
			newTemplate(grey);
			FPSPrint(imgBGR);
			for (MaxVals matchLoc : max)
			{	
				outFloat = to_string(matchLoc.val).substr(0, 6);
				rectangle(imgBGR, matchLoc.point, Point(matchLoc.point.x + templ.cols, matchLoc.point.y + templ.rows), Scalar(0,0,255), 2, 6);
				putText(imgBGR, outFloat, matchLoc.point, FONT_HERSHEY_DUPLEX, 0.65, Scalar(0, 0, 0), 3, 7);
				putText(imgBGR, outFloat, matchLoc.point, FONT_HERSHEY_DUPLEX, 0.65, Scalar(255, 0, 255), 2, 7);				
			}	

			mu.lock();
			imshow(win, imgBGR);
			frame++;
			mu.unlock();	

			grey.release();
			imgBGR.release();
			result.release();
			max.clear();
					
		}
		catch (...) { cout << "EXCEPTION!!!!!!!!!!!"; }
	}
	return;
}

void changeThresh() {
	while (!stop) 
	{
		if (GetAsyncKeyState(0x57) & 1)		// ----------- W key pressed
			thresh += 0.001; 		
		else if (GetAsyncKeyState(0x53) & 1)  // ----------- S key pressed
			thresh -= 0.001; 				
	}
}
void MatchStart() {
	xS = (int)(GetSystemMetrics(SM_CXSCREEN) / scale);
	yS = (int)(GetSystemMetrics(SM_CYSCREEN) / scale);

	vector<future<void>> tasks;	
	refz = system_clock::now();

	while (!(GetAsyncKeyState(VK_ESCAPE) & 1))
	{
		if (tasks.empty())
		{
			for (int i = 0; i < 4; i++) {
				tasks.push_back(async(launch::async, MatchingMethod));
				Sleep(50);				
			}
			tasks.push_back(async(launch::async, changeThresh));
		}
		waitKey(1);
	}
	stop = true;
	
}

int main()
{
	namedWindow(win, WINDOW_AUTOSIZE);
	setMouseCallback("output", mouse_callback);
	MatchStart();
	destroyAllWindows();
	system("pause");
	return 0;
}

