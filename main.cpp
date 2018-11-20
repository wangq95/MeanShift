#include "opencv2/opencv.hpp"

using namespace std;
using namespace cv;

bool roiDefined = false;
CvRect drawing_box;

void init_target(Mat& hist, Mat& weight, Mat current, double& sum)
{
	int t_h, t_w, t_x, t_y;
	int q_r, q_g, q_b, q_temp;

	t_h = drawing_box.height;
	t_w = drawing_box.width;
	t_x = drawing_box.x;
	t_y = drawing_box.y;

	cout << "BoundingBox, t_x=" << t_x << ", t_y=" << t_y << ", t_w=" << t_w << ", t_h=" << t_h << endl;

	hist = Mat(4096, 1, CV_64FC1);
	weight = Mat(t_h, t_w, CV_64FC1);

	double r_2 = pow(((double)t_w) / 2, 2) + pow(((double)t_h) / 2, 2);            
	hist = Scalar::all(0);
	sum = 0;
	for (int i = 0; i < t_h; i++) {
		for (int j = 0; j < t_w; j++) {
			double dist = pow(i - (double)t_h / 2, 2) + pow(j - (double)t_w / 2, 2);
			weight.at<double>(i, j) = 1 - dist / r_2;  // center has weight 1, corner has weight 0
			sum += weight.at<double>(i, j);
		}
	}
	cout << "weight calculated" << endl;

	for (int i = t_y; i < t_y + t_h; i++) {
		for (int j = t_x; j < t_x + t_w; j++) {
			int q_r = current.at<Vec3b>(i, j)[0] / 16;
			int q_g = current.at<Vec3b>(i, j)[1] / 16;
			int q_b = current.at<Vec3b>(i, j)[2] / 16;
			int q_temp = q_r * 256 + q_g * 16 + q_b; //4 bit for each r,g,b, total = 12 bit = 4096.
			hist.at<double>(q_temp) = hist.at<double>(q_temp) + weight.at<double>(i - t_y, j - t_x);
		}
	}

	cout << "histogram created. Sum=" << sum << endl;
	// normalize to total number of pixel
	for (int i = 0; i < 4096; i++) {
		hist.at<double>(i) = hist.at<double>(i) / sum;
	}

	cout << "histogram" << endl;
	double temp_max = 0.0;
	for (int i = 0; i < 4096; i++) {
		if (hist.at<double>(i) != 0)
			cout << "i=" << i << " hist=" << hist.at<double>(i) << " " << endl;
		if (temp_max < hist.at<double>(i)) {
			temp_max = hist.at<double>(i);
		}
	}
	cout << endl;

	CvPoint p1, p2;
	Mat pic_hist = Mat(300, 200, CV_8UC3);  
	pic_hist = Scalar::all(0);

	double bin_width = (double)pic_hist.cols / 4096;
	double bin_unith = (double)pic_hist.rows / temp_max;
	cout << "reday to draw histogram" << endl;
	for (int i = 0; i < 4096; i++) {
		p1.x = i * bin_width;
		p1.y = pic_hist.rows;
		p2.x = (i + 1) * bin_width;
		p2.y = (int)(pic_hist.rows - hist.at<double>(i) * bin_unith);
		rectangle(pic_hist, p1, p2, cvScalar(0, 255, 0), 2, 8, 0);
	}
	/*cout << "Show Histogram" << endl;
	imshow("hist", pic_hist);
	waitKey(0);
	destroyWindow("hist");*/
}

void MeanShift_Tracking(Mat current, Mat& hist, Mat& weight, double& sum) {

	cout << "tracking: " << endl;

	int t_w = 0, t_h = 0;
	t_w = drawing_box.width;
	t_h = drawing_box.height;

	Mat histT = Mat(4096, 1, CV_64FC1);
	Mat w = Mat(4096, 1, CV_64FC1);
	Mat q_temp = Mat(t_h, t_w, CV_32SC1);

	//mean shift:
	double sum_w = 0;
    double xshift = 12;
	double yshift = 12;

	//loop number:
	int num = 0;

	while ((pow(xshift, 2) + pow(yshift, 2) > 2) && (num < 20))
	{
		int t_x = drawing_box.x;
		int t_y = drawing_box.y;
		
		num++;
		
		histT = Scalar::all(0);
		w = Scalar::all(0);
		q_temp = Scalar::all(0);

		// calculate current histogram:
		for (int i = t_y; i < t_y + t_h; i++) {
			for (int j = t_x; j < t_x + t_w; j++) {
				int q_r = current.at<Vec3b>(i, j)[0] / 16;
				int q_g = current.at<Vec3b>(i, j)[1] / 16;
				int q_b = current.at<Vec3b>(i, j)[2] / 16;
				int q = q_r * 256 + q_g * 16 + q_b; //4 bit for each r,g,b, total = 12 bit = 4096.
				q_temp.at<int>(i - t_y, j - t_x) = q;
				histT.at<double>(q) = histT.at<double>(q) + weight.at<double>(i - t_y, j - t_x);
			}
		}

		for (int i = 0; i < 4096; i++) {
			histT.at<double>(i) = histT.at<double>(i) / sum;
		}

		for (int i = 0; i < 4096; i++) {
			if (histT.at<double>(i) != 0) {
				double f = histT.at<double>(i);
				if (f == 0)
					w.at<double>(i) = 0;
				else {
					w.at<double>(i) = sqrt((double)hist.at<double>(i) / f);
				}
			}
		}

		sum_w = 0.0;
		xshift = 0.0;
		yshift = 0.0;

		for (int i = 0; i < t_h; i++) {
			for (int j = 0; j < t_w; j++) {
				//cout << q_temp.at<int>(i, j) << ":";
				double r = w.at<double>(q_temp.at<int>(i, j));  //match ratio
				sum_w = sum_w + r;
				xshift = xshift + r * (j - t_w / 2);
				yshift = yshift + r * (i - t_h / 2);
			}
		}
		xshift = xshift / sum_w;
		yshift = yshift / sum_w;
		
		drawing_box.x += (int)xshift;
		drawing_box.y += (int)yshift;
		drawing_box.x = max(0, drawing_box.x);
		drawing_box.y = max(0, drawing_box.y);

		drawing_box.x = min(current.cols - drawing_box.width - 1, drawing_box.x);
		drawing_box.y = min(current.rows - drawing_box.height - 1, drawing_box.y);
	}

	cout << "sum_w: " << sum_w << endl;
	cout << "xshift: " << xshift << ", yshift: " << yshift << endl;
	cout << "after shift: " << endl;
	cout << "BoundingBox, t_x=" << drawing_box.x << ", t_y=" << drawing_box.y << ", t_w=" << drawing_box.width << ", t_h=" << drawing_box.height << endl;

	//histT.copyTo(hist);
}

void onMouse(int event, int x, int y, int flags, void *param)
{
	static bool buttonDown = false;

	switch (event)
	{
		case CV_EVENT_LBUTTONDOWN:
			cout << "Button Down" << endl;
			roiDefined = false;
			buttonDown = true;
			//the left up point of the rect  
			drawing_box.x = x;
			drawing_box.y = y;
			drawing_box.width = 0;
			drawing_box.height = 0;
			break;
		case CV_EVENT_MOUSEMOVE:
			if (buttonDown) {
				drawing_box.width = x - drawing_box.x;
				drawing_box.height = y - drawing_box.y;
			}
			break;
		case CV_EVENT_LBUTTONUP:
			cout << "Button Up" << endl;
			buttonDown = false;
			drawing_box.width = x - drawing_box.x;
			drawing_box.height = y - drawing_box.y;
			roiDefined = true;
			//waitKey(10);
			break;
	}
	return;
}

int main(int argc, char* argv[])
{
	Mat current;
	VideoCapture capture;
	capture.open(CAP_ANY);
	namedWindow("Meanshift");
	capture.read(current);
	cvSetMouseCallback("Meanshift", onMouse, 0);

	int n = 0;
	bool initialized = false;
	Mat hist, weight;
	double sum = 0;
	while (true) {
		capture.read(current);
		//cvSetMouseCallback("Meanshift", onMouse, &current);
		
		if (roiDefined) {
			//cout << "roi defined!" << endl;
			if (!initialized) {
				cout << "Initialize..." << endl;
				hist = Mat(4096, 1, CV_64FC1); //4096 = r16 x g16 x b16
				weight = Mat(drawing_box.height, drawing_box.width, CV_64FC1);
				init_target(hist, weight, current, sum);
				initialized = true;
			}
			MeanShift_Tracking(current, hist, weight, sum);
		}
		else
			initialized = false;

		cout << "Loop: " << n++ << endl;
		rectangle(current, cvPoint(drawing_box.x, drawing_box.y), cvPoint(drawing_box.x + drawing_box.width, drawing_box.y + drawing_box.height), CV_RGB(255, 0, 0), 2);
		imshow("Meanshift", current);
		if(waitKey(100) == 27)
			break;
	}
	capture.release();
	cvDestroyWindow("Meanshift");
	return 0;
}
