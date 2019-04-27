#include "pch.h"
#include <opencv2/video/tracking.hpp>
using namespace cv;
using namespace std;
using namespace std::chrono;

mutex mu;
Mat templ = Mat::zeros(5, 5, CV_8UC1);
int xS, yS, frame, match_method = 3;
char win[] = "output";
float scale = 1.7f, thresh = 0.999f;

time_point<system_clock> refz;
vector<Point> pt;
bool enough = false;

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
	if (event == EVENT_LBUTTONDOWN && !enough)
	{
		pt.push_back(Point(x, y));
		cout << x << " " << y;
		if (pt.size() == 2)
			enough = true;
	}

}

void drawSelection(Mat img)
{
	if (enough)
	{
		templ.release();
		cvtColor(img, img, CV_BGR2GRAY);
		templ = img(Rect(pt[0], pt[1]));
		pt.clear();
		enough = false;
	}
}

void TimePrint()
{
	if (duration_cast<milliseconds>(system_clock::now() - refz).count() >= 1000)
	{
		cout << "FPS: " << frame << endl;
		frame = 0;
		refz = system_clock::now();
	}
}

void MatchingMethod()
{
	int y, x;
	Mat img, img2;
	Mat result = Mat(0, 0, CV_32FC1);
	HWND hwndDesktop = GetDesktopWindow();

	while (true) {
		try {
			img2 = hwnd2mat(hwndDesktop);
			cvtColor(img2, img, CV_BGR2GRAY);			
			matchTemplate(img, templ, result, match_method);
			//	normalize(result, result, 0, 1, NORM_MINMAX, -1, Mat());

			vector<Point> max_values;
			
			for (y = 0; y < result.rows; y++)
				for (x = 0; x < result.cols; x++) 
				{
					if (result.at<float>(y, x) >= thresh)
						max_values.push_back(Point(x, y));
					if (max_values.size() > 150)
					{
						templ = Mat::zeros(Size(5, 5), CV_8UC1);						
						y = result.rows - 1;
						x = result.cols - 1;
						max_values.clear();
						cout << "---------------LIMIT EXCEEDED => CLEARED SCREEN------------" << endl;
					}

				}
			drawSelection(img2);
			for (Point matchLoc : max_values) {
				rectangle(img2, matchLoc, Point(matchLoc.x + templ.cols, matchLoc.y + templ.rows), Scalar(0,0,255), 2, 6, 0);
				//cout << matchLoc << endl;
			}
			mu.lock();
			imshow(win, img2);
			frame++;
			waitKey(1);
			img.release();
			result.release();
			mu.unlock();

		}
		catch (...) {}
	}
	return;
}

void MatchStart() {

	xS = (int)(GetSystemMetrics(SM_CXSCREEN) / scale);
	yS = (int)(GetSystemMetrics(SM_CYSCREEN) / scale);

	vector<future<void>> tasks;
	int key = 0, i;
	refz = system_clock::now();

	while (key != 27)
	{
		if (tasks.empty())
		{
			for (i = 0; i < 3; i++) {
				tasks.push_back(async(launch::async, MatchingMethod));
				Sleep(50);
			}
		}
		key = waitKey(1);
		TimePrint();
	}
	exit(0);
}

int main()
{
	namedWindow(win, WINDOW_AUTOSIZE);
	setMouseCallback("output", mouse_callback);
	MatchStart();
	return 0;
}

