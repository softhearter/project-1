#include<iostream>
#include <math.h>
#include<opencv2/opencv.hpp>
#include<vector>

using namespace cv;
using namespace std;

///////////////////////////////////////
//计算列上的像素值和
int getColSum(Mat &src,int cols)
{
    int result = 0;
    for (int i = 0;i<src.rows;i++)
	{
        result += src.at<uchar>(i,cols);
    }
    return result;
}
//计行上的像素值和
int getRowSum(Mat &src,int rows)
{
    int result = 0;
    for (int i = 0;i<src.cols;i++)
	{
        result += src.at<uchar>(rows,i);
    }
    return result;
}

//上下剪切图像
int cutTop(Mat &dst,Mat &result)
{
    int up = 0,down;
    for (int i = 0;i<dst.rows;i++)
	{
        int rows_value = getRowSum(dst,i);
        if (rows_value > 0)
		{
            up = i;
            break;
        }
    }
    if (up == 0)
	{
        return 1;
    }
    for (int i = up;i<dst.rows;i++)
	{
        int rows_value = getRowSum(dst,i);
        if (rows_value == 0)
		{
            down = i;
            break;
        }
    }
    int height = down - up;
    Rect roi(0,up,dst.cols,height);
    result = dst(roi).clone();
    return 0;
}

//左右剪切图像
int cutLeft(Mat &src,Mat &result,Mat &rImg)
{
    int left = 0,right;
    int cols_value;

    //单个数字的左界限
    for (int i = 0;i<src.cols;i++)
	{
        cols_value = getColSum(src,i);
        if (cols_value > 0)
		{
            left = i;
            break;
        }
    }
    if (left == 0)
	{
        return 1;
    }
    //右界限
    for (int i = left;i<src.cols;i++)
	{
        cols_value = getColSum(src,i);
        if (cols_value == 0)
		{
            right = i;
            break;
        }
    }
    int w = right-left;
    Mat dst;
    Rect roi(left,0,w,src.rows);
    dst = src(roi).clone();

    Rect rest(right,0,src.cols-right,src.rows);
    rImg = src(rest).clone();

    cutTop(dst,result);
    return 0;
}

//两个模板：识别模板和数字模板
bool Cutnum_Save_num(Mat img,vector<Mat>&Num_Temp)
{
	Mat result,rImg,gray,thresh;
	cvtColor(img,gray,COLOR_BGR2GRAY);
    threshold(gray,thresh,150,255,THRESH_BINARY);
    int flag = cutLeft(thresh,result,rImg);
	int i = 0;
	while(flag == 0)
	{
        cutLeft(thresh,result,rImg);
        Num_Temp.push_back(result.clone());
        i++;
        thresh = rImg.clone();
        flag = cutLeft(thresh,result,rImg);
    }
	if(Num_Temp.empty()||Num_Temp.size()!=16)
	{
		return false;
	}
	return true;
}

bool Cutmod_Save_num(Mat img_mod,vector<Mat>&Card_Temp)
{
	Mat result,rImg,gray,thresh;
	cvtColor(img_mod,gray,COLOR_BGR2GRAY);
    threshold(gray,thresh,150,255,THRESH_BINARY);
    int flag = cutLeft(thresh,result,rImg);
	int i = 0;
	while(flag == 0)
	{
        cutLeft(thresh,result,rImg);
        Card_Temp.push_back(result.clone());
        i++;
        thresh = rImg.clone();
        flag = cutLeft(thresh,result,rImg);
    }
	if(Card_Temp.empty()||Card_Temp.size()!=10)
	{
		return false;
	}
	return true;
}

//对比两张图片的像素重合多少，通过like返回到调用者
int my_compare(Mat a,Mat b)
{
    int same = 0;
    for (int i = 0;i<a.rows;i++)
	{
        for(int j = 0;j<a.cols;j++)
		{
            if (a.at<uchar>(i,j) == b.at<uchar>(i,j))
			{
				same++;
			}
        }
    }
    return same;
}


void check(vector<Mat>Card_Temp,vector<Mat>Num_Temp,vector<int>&num)
{
    int maxidx,max = 0;
    for (int i = 0;i<Num_Temp.size();i++)
	{
		resize(Num_Temp[i],Num_Temp[i],Size(8,8),0,0);
	}
    for (int i = 0;i<Card_Temp.size();i++)
	{
		resize(Card_Temp[i],Card_Temp[i],Size(8,8),0,0);
	}
    for (int i = 0;i<Num_Temp.size();i++)
	{
        for(int j = 0;j<Card_Temp.size();j++)
		{
            if (my_compare(Num_Temp[i],Card_Temp[j])>max)
			{
                max = my_compare(Num_Temp[i],Card_Temp[j]);
                maxidx = j;
            }
        }
        num.push_back(maxidx);
        max = 0;
        maxidx = 0;
    }
}

/////////////////////////////////////////////////////////

bool Cut_Block(Mat src, vector<Rect>&Block_ROI)
{
	Rect m_select = Rect(0,src.rows*0.5,src.cols,src.rows*0.175);//裁出大概位置
	Mat ROI = src(m_select);
	//imshow("剪裁图", ROI);
	//waitKey(0);
	resize(ROI, src,Size(),2,2,INTER_LINEAR);
	//形态学操作、以便找到银行卡号区域轮廓
    Mat gray;
	cvtColor(ROI, gray, COLOR_BGR2GRAY);

	Mat gaussian;
	GaussianBlur(gray, gaussian, Size(3, 3), 0);
	//imshow("gaussian",gaussian);
	//waitKey(0);

	Mat thresh;
	threshold(gaussian, thresh, 0, 255, THRESH_BINARY | THRESH_OTSU);
	//imshow("thresh",thresh);
	//waitKey(0);

	Mat imgCanny;
	Canny(thresh, imgCanny, 150, 200);
	//imshow("imgCanny",imgCanny);
	//waitKey(0);

	Mat close;
	Mat kernel2 = getStructuringElement(MORPH_RECT, Size(13, 10));
	Mat kernel1 = getStructuringElement(MORPH_RECT, Size(6, 5));
	Mat imgDia;
	dilate(imgCanny, imgDia, kernel2);
	//imshow("image Dilation", imgDia);

	morphologyEx(imgDia, close, MORPH_CLOSE, kernel1);
	//imshow("close",close);
	//waitKey(0);
	////////////////////

	vector<vector<Point>>contours;
	findContours(close, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
	for (int i = 0; i < contours.size(); i++)
	{
		//通过面积、长宽比筛选出银行卡号区域
		double area = contourArea(contours[i]);

		if (area > 800 && area < 7000)
		{
			Rect rect = boundingRect(contours[i]);
			float ratio = double(rect.width) / double(rect.height);

			if (ratio > 2.4 && ratio < 3.1)
			{
				rect.y = rect.y+src.rows*1.46;
				Block_ROI.push_back(rect);
			}
		}
	}
	
	if (Block_ROI.size()!=4)
	{
		cout<<Block_ROI.size();
		return false;
	}
	
	for (int i = 0; i < Block_ROI.size()-1; i++)
	{
		for (int j = 0; j < Block_ROI.size() - 1 - i; j++)
		{
			if (Block_ROI[j].x > Block_ROI[j + 1].x)
			{
				Rect temp = Block_ROI[j];
				Block_ROI[j] = Block_ROI[j + 1];
				Block_ROI[j + 1] = temp;
			}
		}
	}

	return true;
}


int main ()
{
    Mat img = imread("Model/ocr_a_reference.png");//数字模板
	Mat src = imread("Card/credit_card_02.png");//目标模板

    vector<Rect> Block_ROI;//区块模板
	vector<Mat> Card_Temp;
	vector<Mat> Num_Temp;
    vector<int> Num;
    vector<string> number;
    
    bitwise_not(img,img);
	if(!Cutmod_Save_num(img,Card_Temp))
	{
		cout<<"cut mod faild"<<endl;
		system("pause");
		return -1;
	}
	else
	{
        Rect rect;
		if(Cut_Block(src, Block_ROI))
		{
			rect.x=Block_ROI[0].x;
			rect.y=Block_ROI[0].y;
			rect.width = Block_ROI[3].x-Block_ROI[0].x+Block_ROI[3].width;
			rect.height =Block_ROI[0].height;
            rectangle(src, rect, Scalar(0, 255, 0), 1);
            //imshow("src",src);
            //waitKey(0);
			if(!Cutnum_Save_num(src(rect),Num_Temp))
			{
				cout<<"cut num faild"<<endl;
			    system("pause");
			    return -1;
			}
			else
			{
				check(Card_Temp,Num_Temp,Num);
				string text1,text2,text3,text4;
                for(int i=0;i<Num.size();i++)
				{
					if(i>=0&&i<=3)
					{
						char temp;
						temp = Num[i]+'0';
						text1.push_back(temp);
					}
					else if(i>=4&&i<=7)
					{
						char temp;
						temp = Num[i]+'0';
						text2.push_back(temp);
					}
					else if(i>=8&&i<=11)
					{
						char temp;
						temp = Num[i]+'0';
						text3.push_back(temp);
					}
					else
					{
						char temp;
						temp = Num[i]+'0';
						text4.push_back(temp);
					}
				}
				//cout<<text1<<" "<<text2<<" "<<text3<<" "<<text4<<endl;
				for(int i=0;i<Block_ROI.size();i++)
				{
					rectangle(src, Block_ROI[i], Scalar(0, 255, 0), 2);
                }
				putText(src,text1, Block_ROI[0].tl (),FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 0, 255), 2);
				putText(src,text2, Block_ROI[1].tl (),FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 0, 255), 2);
				putText(src,text3, Block_ROI[2].tl (),FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 0, 255), 2);
				putText(src,text4, Block_ROI[3].tl (),FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 0, 255), 2);
				imshow("src",src);
				waitKey(0);
				system("pause");
				return -1;
			}
		}
        else
		{
			cout<<"cut block faild"<<endl;
			system("pause");
			return -1;
		} 
		
	}
}
