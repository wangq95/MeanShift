#include <opencv2\opencv.hpp>
using namespace std;
using namespace cv;

#define  u_char unsigned char  
#define  DIST 0.5  
#define  NUM 20  

//????  
bool pause = false;
bool is_tracking = false;
CvRect drawing_box;
Mat current;
double *hist1, *hist2;
double *m_wei;                                                                  //????  
double C = 0.0;                                                                //?????  



void init_target(double *hist1, double *m_wei, Mat current)
{
	IplImage *pic_hist = 0;
	int t_h, t_w, t_x, t_y;
	double h, dist;
	int i, j;
	int q_r, q_g, q_b, q_temp;

	t_h = drawing_box.height;
	t_w = drawing_box.width;
	t_x = drawing_box.x;
	t_y = drawing_box.y;

	h = pow(((double)t_w) / 2, 2) + pow(((double)t_h) / 2, 2);            //??  
	pic_hist = cvCreateImage(cvSize(300, 200), IPL_DEPTH_8U, 3);     //???????  

																	 //?????????????  
	for (i = 0; i < t_w*t_h; i++)
	{
		m_wei[i] = 0.0;
	}

	for (i = 0; i<4096; i++)
	{
		hist1[i] = 0.0;
	}

	for (i = 0; i < t_h; i++)
	{
		for (j = 0; j < t_w; j++)
		{
			dist = pow(i - (double)t_h / 2, 2) + pow(j - (double)t_w / 2, 2);
			m_wei[i * t_w + j] = 1 - dist / h;
			//printf("%f\n",m_wei[i * t_w + j]);  
			C += m_wei[i * t_w + j];
		}
	}

	uint8_t* ptr = (uint8_t*)current.data;
	for (i = t_y; i < t_y + t_h; i++)
	{
		for (j = t_x; j < t_x + t_w; j++)
		{
			
			//rgb???????16*16*16 bins  
			q_r = (ptr[i * current.step + j * 3 + 2]) / 16;
			q_g = (ptr[i * current.step + j * 3 + 1]) / 16;
			q_b = (ptr[i * current.step + j * 3 + 0]) / 16;
			q_temp = q_r * 256 + q_g * 16 + q_b;
			hist1[q_temp] = hist1[q_temp] + m_wei[(i - t_y) * t_w + (j - t_x)];
		}
	}

	//??????  
	for (i = 0; i<4096; i++)
	{
		hist1[i] = hist1[i] / C;
		//printf("%f\n",hist1[i]);  
	}

	//???????  
	double temp_max = 0.0;

	for (i = 0; i < 4096; i++)         //???????,?????  
	{
		//printf("%f\n",val_hist[i]);  
		if (temp_max < hist1[i])
		{
			temp_max = hist1[i];
		}
	}
	//????  
	CvPoint p1, p2;
	double bin_width = (double)pic_hist->width / 4096;
	double bin_unith = (double)pic_hist->height / temp_max;

	for (i = 0; i < 4096; i++)
	{
		p1.x = i * (int)bin_width;
		p1.y = pic_hist->height;
		p2.x = (i + 1) * (int)bin_width;
		p2.y = (int)(pic_hist->height - hist1[i] * bin_unith);
		//printf("%d,%d,%d,%d\n",p1.x,p1.y,p2.x,p2.y);  
		cvRectangle(pic_hist, p1, p2, cvScalar(0, 255, 0), -1, 8, 0);
	}
	cvSaveImage("hist1.jpg", pic_hist);
	cvReleaseImage(&pic_hist);
}

void MeanShift_Tracking(Mat current)
{
	int num = 0, i = 0, j = 0;
	int t_w = 0, t_h = 0, t_x = 0, t_y = 0;
	double *w = 0, *hist2 = 0;
	double sum_w = 0, x1 = 0, x2 = 0, y1 = 2.0, y2 = 2.0;
	int q_r, q_g, q_b;
	int *q_temp;
	IplImage *pic_hist = 0;

	t_w = drawing_box.width;
	t_h = drawing_box.height;

	pic_hist = cvCreateImage(cvSize(300, 200), IPL_DEPTH_8U, 3);     //???????  
	hist2 = (double *)malloc(sizeof(double) * 4096);
	w = (double *)malloc(sizeof(double) * 4096);
	q_temp = (int *)malloc(sizeof(int)*t_w*t_h);

	while ((pow(y2, 2) + pow(y1, 2) > 0.5) && (num < NUM))
	{
		num++;
		t_x = drawing_box.x;
		t_y = drawing_box.y;
		memset(q_temp, 0, sizeof(int)*t_w*t_h);
		for (i = 0; i<4096; i++)
		{
			w[i] = 0.0;
			hist2[i] = 0.0;
		}

		uint8_t* ptr = (uint8_t*)current.data;
		for (i = t_y; i < t_h + t_y; i++)
		{
			for (j = t_x; j < t_w + t_x; j++)
			{
				//rgb???????16*16*16 bins  
				q_r = (ptr[i * current.step + j * 3 + 2]) / 16;
				q_g = (ptr[i * current.step + j * 3 + 1]) / 16;
				q_b = (ptr[i * current.step + j * 3 + 0]) / 16;
				q_temp[(i - t_y) *t_w + j - t_x] = q_r * 256 + q_g * 16 + q_b;
				hist2[q_temp[(i - t_y) *t_w + j - t_x]] = hist2[q_temp[(i - t_y) *t_w + j - t_x]] + m_wei[(i - t_y) * t_w + j - t_x];
			}
		}

		//??????  
		for (i = 0; i<4096; i++)
		{
			hist2[i] = hist2[i] / C;
			//printf("%f\n",hist2[i]);  
		}
		//???????  
		double temp_max = 0.0;

		for (i = 0; i<4096; i++)         //???????,?????  
		{
			if (temp_max < hist2[i])
			{
				temp_max = hist2[i];
			}
		}
		//????  
		CvPoint p1, p2;
		double bin_width = (double)pic_hist->width / (4368);
		double bin_unith = (double)pic_hist->height / temp_max;

		for (i = 0; i < 4096; i++)
		{
			p1.x = (int)(i * bin_width);
			p1.y = (int)(pic_hist->height);
			p2.x = (int)((i + 1)*bin_width);
			p2.y = (int)(pic_hist->height - hist2[i] * bin_unith);
			cvRectangle(pic_hist, p1, p2, cvScalar(0, 255, 0), -1, 8, 0);
		}
		cvSaveImage("hist2.jpg", pic_hist);

		for (i = 0; i < 4096; i++)
		{
			if (hist2[i] != 0)
			{
				w[i] = sqrt(hist1[i] / hist2[i]);
			}
			else
			{
				w[i] = 0;
			}
		}

		sum_w = 0.0;
		x1 = 0.0;
		x2 = 0.0;

		for (i = 0; i < t_h; i++)
		{
			for (j = 0; j < t_w; j++)
			{
				//printf("%d\n",q_temp[i * t_w + j]);  
				sum_w = sum_w + w[q_temp[i * t_w + j]];
				x1 = x1 + w[q_temp[i * t_w + j]] * (i - t_h / 2);
				x2 = x2 + w[q_temp[i * t_w + j]] * (j - t_w / 2);
			}
		}
		y1 = x1 / sum_w;
		y2 = x2 / sum_w;

		//???????  
		drawing_box.x += (int)y2;
		drawing_box.y += (int)y1;

		//printf("%d,%d\n",drawing_box.x,drawing_box.y);  
	}
	free(hist2);
	free(w);
	free(q_temp);
	//??????  
	rectangle(current, cvPoint(drawing_box.x, drawing_box.y), cvPoint(drawing_box.x + drawing_box.width, drawing_box.y + drawing_box.height), CV_RGB(255, 0, 0), 2);
	imshow("Meanshift", current);
	//cvSaveImage("result.jpg",current);  
	cvReleaseImage(&pic_hist);
}

void onMouse(int event, int x, int y, int flags, void *param)
{
	if (pause)
	{
		switch (event)
		{
		case CV_EVENT_LBUTTONDOWN:
			//the left up point of the rect  
			drawing_box.x = x;
			drawing_box.y = y;
			break;
		case CV_EVENT_LBUTTONUP:
			//finish drawing the rect (use color green for finish)  
			drawing_box.width = x - drawing_box.x;
			drawing_box.height = y - drawing_box.y;
			rectangle(current, cvPoint(drawing_box.x, drawing_box.y), cvPoint(drawing_box.x + drawing_box.width, drawing_box.y + drawing_box.height), CV_RGB(255, 0, 0), 2);
			imshow("Meanshift", current);

			//?????  
			hist1 = (double *)malloc(sizeof(double) * 16 * 16 * 16);
			m_wei = (double *)malloc(sizeof(double)*drawing_box.height*drawing_box.width);
			init_target(hist1, m_wei, current);
			is_tracking = true;
			break;
		}
		return;
	}
}

int main(int argc, char* argv[])
{
	VideoCapture capture;
	capture.open(CAP_ANY);
	capture.read(current);
	int nframe = 0;

	while (1)
	{
		/*  sprintf(res,"result%d.jpg",nframe);
		cvSaveImage(res,current);
		nframe++;*/
		if (is_tracking)
		{
			MeanShift_Tracking(current);
		}

		int c = cvWaitKey(1);
		//??  
		if (c == 'p')
		{
			pause = true;
			cvSetMouseCallback("Meanshift", onMouse, 0);
		}
		while (pause) {
			if (cvWaitKey(0) == 'p')
				pause = false;
		}
		imshow("Meanshift", current);
		capture.read(current);
	}

	cvNamedWindow("Meanshift", 1);
	capture.release();
	cvDestroyWindow("Meanshift");
	return 0;
}
