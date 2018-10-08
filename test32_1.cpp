#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream> 
#include <io.h>
#include <Windows.h>
#include "opencv\cv.h"
#include "opencv\highgui.h"
#include <direct.h>
using namespace cv;
using namespace std;

extern int readDir(char *dirName, vector<string> &filesName);
extern void coordinates(Point2d src, float angle, Point2d & dst);
#define M_PI 3.14159265358979323846
void readTxt(const char* anno_file, vector<string>& v_img_)
{
	ifstream ReadFile;
	ReadFile.open(anno_file, ios::in);
	if (ReadFile.fail())//文件打开失败:返回0  
	{
		printf("文件打开失败:\n");
		return;
	}
	else//文件存在  
	{
		string tmp;
		while (getline(ReadFile, tmp, '\n'))
		{
			v_img_.push_back(tmp);
		}
		ReadFile.close();

	}
}
// 旋转中心，坐标为车牌中心，旋转区域车牌区域
void J_Rotate_src(Mat src, int x1, int y1, int x2, int y2, float angle, Mat dst,
	int & l1, int & t1, int & r1, int & b1)
{
	Point center;
	center.x = (x1 + x2) / 2; center.y = (y1 + y2) / 2;
	const double cosAngle = cos(angle);
	const double sinAngle = sin(angle);

	//原图像四个角的坐标变为以旋转中心的坐标系
	Point2d leftTop(x1 - center.x, -y1 + center.y); //(x1,y1)
	Point2d rightTop(x2 - center.x, -y1 + center.y); // (x2,y1)
	Point2d leftBottom(x1 - center.x, -y2 + center.y); //(x1,y2)
	Point2d rightBottom(x2 - center.x, -y2 + center.y); // (x2,y2)

	//以center为中心旋转后四个角的坐标
	Point2d transLeftTop, transRightTop, transLeftBottom, transRightBottom;
	coordinates(leftTop, angle, transLeftTop);
	coordinates(rightTop, angle, transRightTop);
	coordinates(leftBottom, angle, transLeftBottom);
	coordinates(rightBottom, angle, transRightBottom);


	double left = min({ transLeftTop.x, transRightTop.x, transLeftBottom.x, transRightBottom.x });
	double right = max({ transLeftTop.x, transRightTop.x, transLeftBottom.x, transRightBottom.x });
	double top = min({ transLeftTop.y, transRightTop.y, transLeftBottom.y, transRightBottom.y });
	double down = max({ transLeftTop.y, transRightTop.y, transLeftBottom.y, transRightBottom.y });

	int width = static_cast<int>(abs(left - right) + 0.5);
	int height = static_cast<int>(abs(top - down) + 0.5);

	// 左上角为原点的坐标
	Point pt1, pt2;
	pt1.x = transLeftTop.x + center.x, pt1.y = -transLeftTop.y + center.y;
	pt2.x = transRightTop.x + center.x, pt2.y = -transRightTop.y + center.y;
	Point pt3, pt4;
	pt3.x = transLeftBottom.x + center.x, pt3.y = -transLeftBottom.y + center.y;
	pt4.x = transRightBottom.x + center.x, pt4.y = -transRightBottom.y + center.y;

	int left1 = min({ pt1.x, pt2.x, pt3.x, pt4.x });
	int right1 = max({ pt1.x, pt2.x, pt3.x, pt4.x });
	int top1 = min({ pt1.y, pt2.y, pt3.y, pt4.y });
	int down1 = max({ pt1.y, pt2.y, pt3.y, pt4.y });

	const double num1 = -abs(left) * cosAngle - abs(top) * sinAngle + center.x;
	const double num2 = abs(left) * sinAngle - abs(top) * cosAngle + center.y;

	Vec3b *p;
	for (int i = 0; i < height; i++)
	{
		p = dst.ptr<Vec3b>(i + top1);
		for (int j = 0; j < width; j++)
		{
			//坐标变换
			int x = static_cast<int>(j  * cosAngle + i * sinAngle + num1 + 0.5);
			int y = static_cast<int>(-j * sinAngle + i * cosAngle + num2 + 0.5);

			if (x >= 0 && y >= 0 && x < src.cols && y < src.rows)
				p[j + left1] = src.ptr<Vec3b>(y)[x];
		}
	}

	l1 = left1; t1 = top1; r1 = right1; b1 = down1;
}

void RotatePoint(int x, int y, float centerX, float centerY, float angle, Point & pt)
{
	x -= centerX;
	y -= centerY;
	float theta = angle * M_PI / 180;
	int rx = int(centerX + x * std::cos(theta) - y * std::sin(theta));
	int ry = int(centerY + x * std::sin(theta) + y * std::cos(theta));
	pt.x = rx; pt.y = ry;
}

void DrawLine(cv::Mat img, std::vector<cv::Point> pointList)
{
	int thick = 1;
	CvScalar cyan = CV_RGB(0, 255, 255);
	CvScalar blue = CV_RGB(0, 0, 255);
	cv::line(img, pointList[0], pointList[1], cyan, thick);
	cv::line(img, pointList[1], pointList[2], cyan, thick);
	cv::line(img, pointList[2], pointList[3], cyan, thick);
	cv::line(img, pointList[3], pointList[0], blue, thick);
}
void DrawFace(cv::Mat img, Point pt11,Point pt22, float angle)
{
	int x1 = pt11.x;
	int y1 = pt11.y;
	int x2 = pt22.x;
	int y2 = pt22.y;
	int centerX = (x1 + x2) / 2;
	int centerY = (y1 + y2) / 2;
	std::vector<cv::Point> pointList;
	Point pt1, pt2, pt3, pt4;
	RotatePoint(x1, y1, centerX, centerY, angle, pt1);
	RotatePoint(x1, y2, centerX, centerY, angle, pt2);
	RotatePoint(x2, y2, centerX, centerY, angle, pt3);
	RotatePoint(x2, y1, centerX, centerY, angle, pt4);
	pointList.push_back(pt1);
	pointList.push_back(pt2);
	pointList.push_back(pt3);
	pointList.push_back(pt4);
	DrawLine(img, pointList);
}
// 只旋转车牌区域，以车牌中心为旋转中心，坐标原点
int test32_1(int argc, char *argv[])
{
	


	
	string outputdraw = "I:/mtcnn-train/rotateResult/draw";
	mkdir(outputdraw.c_str());
	string inputtxt = "I:/mtcnn-train/rotateResult/0src/000_one.txt";
	
	vector<string> v_img_;
	readTxt((char*)inputtxt.c_str(), v_img_);
	srand((unsigned)time(NULL));
	for (int i = 0; i < v_img_.size(); i++)
	{
		string 	annotation = v_img_[i];
		cout << annotation << endl;
		int npos = annotation.find_last_of('/');
		int npos2 = annotation.find_last_of('.');
		string name1 = annotation.substr(npos + 1, npos2 - npos - 1);
		string im_path;
		istringstream is(annotation);
		int label; int x1, y1, x2, y2; int degree;

		is >> im_path>> label >> x1 >> y1 >> x2 >> y2 >> degree;


		Mat img = imread(im_path.c_str());
		if (img.data == NULL)
		{

			printf("图像为空!\n");
			cout << im_path.c_str() << endl;
			system("pause");
		}
		

		

		Mat dst = img.clone();
		Point pt1, pt2, pt3, pt4;
		pt1.x = x1; pt1.y = y1; pt2.x = x2; pt2.y = y2;
		int left = 0; int top = 0; int right = 0; int bottom = 0;

		Mat drawimg2 = dst.clone();
		
		DrawFace(drawimg2, pt1, pt2, degree);
		string str3 = outputdraw + "/" + name1 + "-b.jpg";
		imwrite(str3.c_str(), drawimg2);
		

		Mat drawimg3 = dst.clone();
		rectangle(drawimg3,pt1,pt2,Scalar(0,0,255));
		string str4 = outputdraw + "/" + name1 + "-a.jpg";
		imwrite(str4.c_str(), drawimg3);

		double radian = -M_PI*degree*1.0 / 180;
		J_Rotate_src(img, x1, y1, x2, y2, radian, dst, left, top, right, bottom);
		pt3.x = left; pt3.y = top; pt4.x = right; pt4.y = bottom;

		Mat drawimg = dst.clone();
		rectangle(drawimg, pt3, pt4, Scalar(0, 0, 255));

		string str2 = outputdraw + "/" + name1  + "-c.jpg";
		imwrite(str2.c_str(), drawimg);
		

		
		int jjjjjj = 90;

	}

	
	return 0;
}