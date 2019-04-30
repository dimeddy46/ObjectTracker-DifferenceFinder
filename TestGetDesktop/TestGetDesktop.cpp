#include "pch.h"

using namespace cv;
using namespace std;
using namespace std::chrono;

int match_method = 3, threads = 4;				
float scale = 1.0f, thresh = 0.997f;

int imgSize[2], frame[2];	//imgSize[0] = width(x) , imgSize[1] = height(y) |  frame[0] for calculation, frame[1] for output
char win[] = "output";
bool stop = false;

mutex mu;
Mat templ = Mat(10, 10, CV_8UC1);
vector<Point> pt;
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
	
	resize(src, src, Size(imgSize[0], imgSize[1]));
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
	if (duration_cast<milliseconds>(system_clock::now() - refz).count() >= 1000)
	{	
		frame[1] = frame[0];
		frame[0] = 0;
		refz = system_clock::now();
	}
	putText(imgBGR, "FPS:" + to_string(frame[1]) + " Thresh:" + to_string(thresh).substr(0, 6), Point(5, 20), 
		FONT_HERSHEY_DUPLEX, 0.7, Scalar(0, 0, 0), 3, 11);
	putText(imgBGR, "FPS:" + to_string(frame[1]) + " Thresh:" + to_string(thresh).substr(0, 6), Point(5, 20), 
		FONT_HERSHEY_DUPLEX, 0.7, Scalar(255, 255, 255), 2, 11);
}

bool distEuclid(MaxVals a, MaxVals b)	// simplified euclidean distance
{		
	if ((abs(a.point.x - b.point.x) + abs(a.point.y - b.point.y)) < 15)
		return true;
	return false;
}

void MatchingMethod()
{
	int y, x;
	Mat grey, imgBGR, result = Mat(0, 0, CV_32FC1);
	string outFloat;
	MaxVals crt;
	vector<MaxVals> max;	
	HWND hwndDesktop = GetDesktopWindow();

	while (!stop) 
	{
		try {
			imgBGR = hwnd2mat(hwndDesktop);
			cvtColor(imgBGR, grey, CV_BGR2GRAY);
			matchTemplate(grey, templ, result, match_method);
						
			for (y = 0; y < result.rows; y++)
				for (x = 0; x < result.cols; x++)
				{
					if (result.at<float>(y, x) >= thresh)	// add all brightest points to struct
					{
						crt.val = result.at<float>(y, x);
						crt.point = Point(x, y);
						max.push_back(crt);
					}
					if (max.size() > 500) {
			 			//cout << "-----------LIMIT EXCEEDED --------------" << endl;
						y = result.rows - 1;
						x = result.cols - 1;
					}
				}

			for (x = 0; x < max.size(); x++)				// get the brightest point from each cluster
				for (y = x + 1; y < max.size(); y++) 
					if (distEuclid(max[x], max[y]))
					{
						if(max[x].val < max[y].val)
							swap(max[x], max[y]);
						max.erase(max.begin() + y);
						y--;
					}
		/*	for (x = 0; x < max.size(); x++) {
				cout << max[x].val << " " << max[x].point << endl;
			}				
			if (x != 0)
				cout << "------------------------------ " << max.size() << endl;
			*/
			newTemplate(grey);
			FPSPrint(imgBGR);
			for (MaxVals matchLoc : max)
			{	
				outFloat = to_string(matchLoc.val).substr(0, 6);
				rectangle(imgBGR, matchLoc.point, Point(matchLoc.point.x + templ.cols, matchLoc.point.y + templ.rows), Scalar(0,0,255), 2, 6);
				putText(imgBGR, outFloat, matchLoc.point, FONT_HERSHEY_DUPLEX, 0.65/scale, Scalar(0, 0, 0), 3, 5);
				putText(imgBGR, outFloat, matchLoc.point, FONT_HERSHEY_DUPLEX, 0.65/scale, Scalar(255, 0, 255), 2, 5);				
			}	

		//	mu.lock();
			imshow(win, imgBGR);
			frame[0]++;
		//	mu.unlock();	
			grey.release();
			imgBGR.release();
			result.release();
			max.clear();
		}
		catch (...) { cout << "EXCEPTION!!!!!!!!!!!"; }
	}
	return;
}

void MatchStart() 
{
	vector<future<void>> tasks;
	refz = system_clock::now();

	/*templ = imread("temp.png", 0);
	resize(templ,templ,Size(round(templ.cols / scale), round(templ.rows / scale)));*/

	imgSize[0] = (int)(GetSystemMetrics(SM_CXSCREEN) / scale);
	imgSize[1] = (int)(GetSystemMetrics(SM_CYSCREEN) / scale);

	for (int i = 0; i < threads; i++) {						
		tasks.push_back(async(launch::async, MatchingMethod));
		Sleep(50);
	}

	while (!(GetAsyncKeyState(VK_ESCAPE) & 1))	
	{			
		if (GetAsyncKeyState(0x57) & 1)  // ----------- W key pressed
			thresh += 0.0001f;		
		else if (GetAsyncKeyState(0x53) & 1)  // ----------- S key pressed
			thresh -= 0.0001f;
		waitKey(1);
	}
	stop = true;
	tasks.clear();
}

int main()
{
	namedWindow(win, WINDOW_AUTOSIZE);
	setMouseCallback(win, mouse_callback);
	MatchStart();
	destroyAllWindows();
	return 0;
}

//	SetCursorPos((matchLoc.point.x*scale) + 20, (matchLoc.point.y*scale) + 20);
//  mouse_event(MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
