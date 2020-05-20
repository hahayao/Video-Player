#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <opencv2/imgproc/types_c.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/ml/ml.hpp>
#include <string.h>
#include <vector>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <sys/stat.h>
#include<opencv2/stitching.hpp>
#include <opencv2/videoio.hpp>


using namespace std;
using namespace cv;

//structure of the cluster
struct Cluster
{
    int frameNumInCluster = 0;
    vector<double> frameID;
    double CoreValue[16][8] = {0};
};
//structure of the frame information
struct FrameInfo
{
    int HSValue[16][8] = {0};
};

struct ImageFrame
{
    Mat f;
    string frameAbsolutePath;
};

struct Frame
{
    vector<ImageFrame> vf;
};

struct KeyFrame
{
    int videoNo;
    int clusterNo;
    int bestID;
    Mat f;
    string AP;
};

//set of clusters
vector<vector<Cluster>>clusterSet;
//set of all frames
//the last one would stores image
vector<vector<FrameInfo>> frameInfoSet;
//all rgb files in video folder in Mat type
vector<Frame> video;
//store rgb files in image folder in Mat type
vector<ImageFrame> image;
//store selected keyframe ID in an array
vector<KeyFrame> ShowKeyFrame;

//count the total num of cluster
//int clusterCount = 0;

int width = 352;
int height = 288;

//unsigned char checkBuffer[3 * 21824 * 288];

string path = "/Users/haha/Downloads/dataset";
//define the threshold of similarity
double thresholdSimilarity = 0.68;

void BGR2HSV(Mat temp, int videoNo);
void CreateNewCluster(int frameID, int videoNo);
void UpdateClusterCentroid(int frameID, int choosedCluster, int videoNo);
void CalSimilarity(int frameID);
void FindKeyFrameInCluster();

//transfer BGR to HSV, and quantization the histogram
//H: 0 - 15 (interval = 180 / 16)
//S: 0 - 7 (interval = 256 / 8)
void BGR2HSV(Mat temp, int videoNo)
{
    Mat hsv;
    FrameInfo tempFrameInfo;
    
    cvtColor(temp, hsv, COLOR_BGR2HSV);
    for(int i = 0; i < temp.rows; i++)
    {
        for(int j = 0; j < temp.cols; j++)
        {
            double hValue = hsv.at<Vec3b>(i, j)[0];
            double sValue = hsv.at<Vec3b>(i, j)[1];
            
            int hLevel = (int) (hValue / (180.0 / 16));
            if(hLevel == 16)
            {
                hLevel --;
            }
            int sLevel = sValue / (256 / 8);
            if(sLevel == 8)
            {
                sLevel --;
            }
            tempFrameInfo.HSValue[hLevel][sLevel] ++;
        }
    }
    frameInfoSet[videoNo].push_back(tempFrameInfo);
}

//calculate the similarity between currentframe and each centroid of the clusters
void CalSimilarity(int frameID, int videoNo)
{
    double sameValue = 0;
    int choosedCluster = -1;
    if(videoNo < video.size())
    {
        for(int i = clusterSet[videoNo].size() - 1; i < clusterSet[videoNo].size(); i++)
        {
            double currentSameValue = 0;
            for(int j = 0; j < 16; j++)
            {
                for(int k = 0; k < 8; k++)
                {
                    int clusterValue = clusterSet[videoNo][i].CoreValue[j][k];
                    int frameValue = frameInfoSet[videoNo][frameID].HSValue[j][k];
                    currentSameValue += min(clusterValue, frameValue);
                }
            }
            currentSameValue = currentSameValue * 1.0 / (width * height);
            //cout<<"currentSame"<<currentSameValue<<endl;
            if(currentSameValue >= thresholdSimilarity && currentSameValue > sameValue)
            {
                choosedCluster = i;
                sameValue = currentSameValue;
            }
        }
    }
    else
    {
        for(int i = 0; i < clusterSet[videoNo].size(); i++)
        {
            double currentSameValue = 0;
            for(int j = 0; j < 16; j++)
            {
                for(int k = 0; k < 8; k++)
                {
                    int clusterValue = clusterSet[videoNo][i].CoreValue[j][k];
                    int frameValue = frameInfoSet[videoNo][frameID].HSValue[j][k];
                    currentSameValue += min(clusterValue, frameValue);
                }
            }
            currentSameValue = currentSameValue * 1.0 / (width * height);
            //cout<<"currentSame"<<currentSameValue<<endl;
            if(currentSameValue >= 0.6 && currentSameValue > sameValue)
            {
                choosedCluster = i;
                sameValue = currentSameValue;
            }
        }
    }
    
    
    if(choosedCluster != -1)
    {
        UpdateClusterCentroid(frameID, choosedCluster, videoNo);
    }
    else
    {
        CreateNewCluster(frameID, videoNo);
    }
}

//update the centroid of the cluster
void UpdateClusterCentroid(int frameID, int choosedCluster, int videoNo)
{
    clusterSet[videoNo][choosedCluster].frameNumInCluster++;
    clusterSet[videoNo][choosedCluster].frameID.push_back(frameID);
    for(int i = 0; i < 16; i++)
    {
        for(int j = 0; j < 8; j++)
        {
            double previous = clusterSet[videoNo][choosedCluster].CoreValue[i][j];
            double ratio = (clusterSet[videoNo][choosedCluster].frameNumInCluster - 1) * 1.0 / clusterSet[videoNo][choosedCluster].frameNumInCluster;
            clusterSet[videoNo][choosedCluster].CoreValue[i][j] = ratio * previous + (1 - ratio) * frameInfoSet[videoNo][frameID].HSValue[i][j];
        }
    }
    
}

//if the frame is not similar to any centroid (similarity < threshold), then create new cluster
void CreateNewCluster(int frameID, int videoNo)
{
    Cluster tmpCluster;
    tmpCluster.frameNumInCluster ++;
    tmpCluster.frameID.push_back(frameID);
    for(int i = 0; i < 16; i++)
    {
        for(int j = 0; j < 8; j++)
        {
            tmpCluster.CoreValue[i][j] = frameInfoSet[videoNo][frameID].HSValue[i][j];
        }
    }
    
    clusterSet[videoNo].push_back(tmpCluster);
    //cout<<frameID<<", "<<clusterCount<<endl;
}

//calculate best representative frame in each cluster
void FindKeyFrameInCluster()
{
    for(int i = 0; i < clusterSet.size(); i++)
    {
        //cout<<clusterSet[i].size()<<endl;
        for(int j = 0; j < clusterSet[i].size(); j++)
        {
            KeyFrame temp;
            double best = pow(352 * 288, 2);
            int bestID = 0;
            for(int k = 0; k < clusterSet[i][j].frameNumInCluster; k++)
            {
                double difference = 0;
                int currentID = clusterSet[i][j].frameID[k];
                for(int m = 0; m < 16; m++)
                {
                    for(int n = 0; n < 8; n++)
                    {
                        difference += pow(frameInfoSet[i][currentID].HSValue[m][n] - clusterSet[i][j].CoreValue[m][n], 2);
                    }
                }
                if(difference < best)
                {
                    best = difference;
                    bestID = currentID;
                }
            }
            //cout<<bestID<<endl;
            //cout<<clusterSet[i][j].frameID[0]<<","<<clusterSet[i][j].frameID[1]<<endl;
            if(i == (clusterSet.size() - 1))
            {
                temp.videoNo = -1;
                temp.bestID = bestID;
                temp.AP = image[bestID].frameAbsolutePath;
                temp.f = image[bestID].f;
            }
            else
            {
                temp.videoNo = i;
                temp.bestID = bestID;
                temp.AP = video[i].vf[bestID].frameAbsolutePath;
                temp.f = video[i].vf[bestID].f;
            }
            temp.clusterNo = clusterSet[i][j].frameNumInCluster;
            
            
            ShowKeyFrame.push_back(temp);
        }
    }
}


int main()
{
    video.resize(0);
    clusterSet.resize(0);
    int videoNumber = 0;
    
    struct stat s;
    lstat(path.c_str(), &s);
    if( ! S_ISDIR( s.st_mode ) )
    {
        cout<<"dir_name is not a valid directory !"<<endl;
        return 0;
    }
    
    struct dirent ** fileName;
    int filenameNum = scandir(path.c_str(), &fileName, NULL, alphasort);
    for(int filenameCount = 0; filenameCount < filenameNum; filenameCount++)
    {
        string currentfilName(fileName[filenameCount]->d_name);
        if( strcmp(currentfilName.c_str(), ".") == 0 || strcmp(currentfilName.c_str(), "..") == 0)
            continue;
        //convert to lower case
        string pathToLowerCase(currentfilName);
        for_each(pathToLowerCase.begin(), pathToLowerCase.end(), [](char & c){
            c = tolower(c);
        });
        //check if it is rgbvideo
        size_t foundVideo = pathToLowerCase.find("video");
        //check if it is image
        size_t foundImage = pathToLowerCase.find("image");
        //check if it is .wav
        size_t foundwav = pathToLowerCase.find(".wav");
        
        if(foundVideo != std::string::npos)
        {
            string videoFolderPath = path + "/" + currentfilName;
            if(opendir(videoFolderPath.c_str()) == NULL)
            {
                cout<<"Cannot open this folder: "<<videoFolderPath<<endl;
                continue;
            }
            cout<<"open "<<videoFolderPath<<" folder"<<endl;
            videoNumber++;
            struct dirent **frameList;
            int frameNum = scandir(videoFolderPath.c_str(), &frameList, NULL, alphasort);
            Frame rgbFolder;
            ImageFrame rgbFrame;
            
            for(int frameCount = 0; frameCount < frameNum; frameCount++)
            {
                if( strcmp(frameList[frameCount]->d_name, ".") == 0 || strcmp(frameList[frameCount]->d_name, "..") == 0)
                continue;
                string framePath = videoFolderPath + "/" + frameList[frameCount]->d_name;
                
                rgbFrame.frameAbsolutePath = framePath;
                unsigned char buffer[3 * width * height];
                FILE * fp = fopen(framePath.c_str(), "rb");
                if (fp == NULL)
                    cout<<"Cannot open this rgb file"<<framePath<<endl;
                else
                {
                    Mat tempFrame(height, width, CV_8UC3);

                    fread(buffer, sizeof(unsigned char), 3 * width*height, fp);
                                
                    int rgb = 0;
                    for(int h = 0; h < height; h++)
                    {
                        for(int w = 0; w < width; w++)
                        {
                            //b
                            tempFrame.at<Vec3b>(h, w)[0] = buffer[rgb + 2 * height * width];
                            //g
                            tempFrame.at<Vec3b>(h, w)[1] = buffer[rgb + height * width];
                            //r
                            tempFrame.at<Vec3b>(h, w)[2] = buffer[rgb];
                            rgb++;
                        }
                    }
                    rgbFrame.f = tempFrame;
                    rgbFolder.vf.push_back(rgbFrame);
                }
                fclose(fp);
                //free(buffer);
            }
            video.push_back(rgbFolder);
        }
        else if(foundImage != std::string::npos)
        {
            string imageFolderPath = path + "/" + currentfilName;
            if(opendir(imageFolderPath.c_str()) == NULL)
            {
                cout<<"Cannot open this folder: "<<imageFolderPath<<endl;
                continue;
            }
            cout<<"open "<<imageFolderPath<<" folder"<<endl;
            struct dirent **frameList;
            int frameNum = scandir(imageFolderPath.c_str(), &frameList, NULL, alphasort);
            
            for(int frameCount = 0; frameCount < frameNum; frameCount++)
            {
                ImageFrame rgbFolder;
                if( strcmp(frameList[frameCount]->d_name, ".") == 0 || strcmp(frameList[frameCount]->d_name, "..") == 0)
                continue;
                string framePath = imageFolderPath + "/" + frameList[frameCount]->d_name;
                //cout<<framePath<<endl;
                
                rgbFolder.frameAbsolutePath = framePath;
                unsigned char buffer[3 * width * height];
                FILE * fp = fopen(framePath.c_str(), "rb");
                if (fp == NULL)
                    cout<<"Cannot open this rgb file"<<framePath<<endl;
                else
                {
                    Mat tempFrame(height, width, CV_8UC3);

                    fread(buffer, sizeof(unsigned char), 3 * width * height, fp);
                                
                    int rgb = 0;
                    for(int h = 0; h < height; h++)
                    {
                        for(int w = 0; w < width; w++)
                        {
                            //b
                            tempFrame.at<Vec3b>(h, w)[0] = buffer[rgb + 2 * height * width];
                            //g
                            tempFrame.at<Vec3b>(h, w)[1] = buffer[rgb + height * width];
                            //r
                            tempFrame.at<Vec3b>(h, w)[2] = buffer[rgb];
                            rgb++;
                        }
                    }
                    rgbFolder.f = tempFrame;
                    image.push_back(rgbFolder);
                }
                fclose(fp);
                //free(buffer);
            }
            
        }
        else
        {
            
        }
    }
    
    cout<<"video NO."<<video.size()<<endl;
    
    clusterSet.resize(videoNumber + 1);
    frameInfoSet.resize(videoNumber + 1);
    for(int i = 0; i < video.size(); i++)
    {
        clusterSet[i].resize(0);
        frameInfoSet[i].resize(0);
        for(int j = 0; j < video[i].vf.size(); j++)
        {
            //calculate HSV
            BGR2HSV(video[i].vf[j].f, i);
            //create cluster
            if(j == 0)
            {
                CreateNewCluster(0, i);
            }
            else
            {
                //calculate the difference among existing clusters' core value
                CalSimilarity(j, i);
            }
        }
    }
    //cout<<image.size()<<endl;
    clusterSet[videoNumber].resize(0);
    frameInfoSet[videoNumber].resize(0);
    for(int i = 0; i < image.size(); i++)
    {
        
        BGR2HSV(image[i].f, videoNumber);
        if(i == 0)
        {
            CreateNewCluster(0, videoNumber);
        }
        else
        {
            //calculate the difference among existing clusters' core value
            CalSimilarity(i, videoNumber);
                            
        }
    }
    
    
    //find key frame in each cluster
    FindKeyFrameInCluster();
    
    int count = 0;
    vector <KeyFrame>::iterator Iter;
    //only output the cluster which contains enough frames
    //double validInterval = video.size() * 1.0 / clusterSet.size();
    for(Iter = ShowKeyFrame.begin(); Iter < ShowKeyFrame.end(); Iter++)
    {
        stringstream str;
        str << path <<"/KeyFrame/"<< count<<".jpg";
        //cout << str.str() << endl;
        
        if(Iter->videoNo != -1)
        {
            if(Iter->clusterNo < 10)
            {
                vector <KeyFrame>::iterator IterTemp = Iter - 1;
                ShowKeyFrame.erase(Iter);
                Iter = IterTemp;
                continue;
                
            }
            imwrite(str.str(), video[Iter->videoNo].vf[Iter->bestID].f);
            //cout<<ShowKeyFrame[i].videoNo<<endl;
        }
        else
        {
            imwrite(str.str(), image[Iter->bestID].f);
        }
            count ++;
        
    }
    
    
    
    //int IterInt = 0;
    //check if they can stich
    for(Iter = ShowKeyFrame.begin(); Iter < ShowKeyFrame.end() - 1; Iter++)
    {
        cout<<"Stiching"<<endl;
        vector<Mat> imgs;
        
        //imgs.push_back(ShowKeyFrame[IterInt].f);
        imgs.push_back(Iter->f);
        //imgs.push_back(ShowKeyFrame[IterInt + 1].f);
        imgs.push_back((Iter + 1)->f);
        Mat pano;
        Stitcher::Mode mode = Stitcher::PANORAMA;// panorama 表示camera的全景模式
        /*Stitcher stitcher = Stitcher::createDefault(false);*/
        Ptr<Stitcher> stitcher = Stitcher::create(mode);
        Stitcher::Status status = stitcher->stitch(imgs, pano);
        if (status == Stitcher::OK)
        {
            cout<<"Find"<<Iter->AP<<endl;
            //check the image and video seperately
            if(Iter->videoNo == videoNumber && (Iter + 1)->videoNo == -1)
            {
                continue;
            }
            
            //vector <KeyFrame>::iterator IterTemp = ShowKeyFrame.begin();
            ShowKeyFrame.erase(Iter);
            Iter = ShowKeyFrame.begin();
            //ShowKeyFrame.erase(Iter);
            //Iter = ShowKeyFrame.begin();
        }
    }

    
    //
    vector<Mat>output;

    int cols = width * ShowKeyFrame.size();
    int rows = height;
    
    vector <KeyFrame>::iterator IterOutput;
    for(IterOutput = ShowKeyFrame.begin(); IterOutput < ShowKeyFrame.end(); IterOutput++)
    {
        output.push_back(IterOutput->f);
    }
    
    //Mat res(rows,cols-256, CV_8UC3, Scalar(0, 0, 0));
    Mat synopsis(rows,cols, CV_8UC3, Scalar(0, 0, 0));
    
    for(int i = 0; i < output.size(); i++)
    {
        output[i].copyTo(synopsis(Rect(width * i, 0, width, height)));
    }
    
    //cout<<synopsis.cols<<endl;

    //namedWindow("Video", WINDOW_NORMAL);
    //imshow("Video", synopsis);
    //waitKey(0);
    stringstream synopsisPath;
    synopsisPath << path<<"/synopsis.jpg";
    imwrite(synopsisPath.str(), synopsis);
    
    //cout<<3 * synopsis.cols * synopsis.rows<<endl;
    unsigned char outputBuffer[synopsis.cols * synopsis.rows];
    
    //cout<<"test1 "<<endl;
    string pathSynopsis = path + "/" + "Synopsis.rgb";
    FILE * fpSynopsis = fopen(pathSynopsis.c_str(), "wb");
    
    if (fpSynopsis == NULL)
        cout<<"Cannot open this rgb file"<<pathSynopsis<<endl;
    else
    {
        for(int rgb = 0; rgb < 3; rgb++)
        {
            int iter = 0;
            for(int h = 0; h < synopsis.rows; h++)
            {
                for(int w = 0; w < synopsis.cols; w++)
                {
                    outputBuffer[iter] = synopsis.at<Vec3b>(h, w)[2 - rgb];

                    iter++;
                }
            }
            fwrite(outputBuffer, sizeof(unsigned char), sizeof(outputBuffer), fpSynopsis);
        }

    }
    fclose(fpSynopsis);

    //avi   ·
    int aviCount = 0;
    for(int i = 0; i < video.size(); i++)
    {
        stringstream videoAviPath;
        videoAviPath << path <<"/avi/"<< "0" << i<<".avi";
        
        double fps = 30.0;                          // framerate of the created video stream
        //writer.open(videoAviPath.str(), codec, fps, video.size(), CV_8UC3);
        VideoWriter writer(videoAviPath.str(), VideoWriter::fourcc('M', 'J', 'P', 'G'), fps, Size(width, height), CV_8UC3);
        for(int j = 0; j < video[i].vf.size(); j++)
        {
            writer<<video[i].vf[j].f;
        }
        writer.release();
        aviCount++;
    }
    
    for(int i = 0; i < ShowKeyFrame.size(); i++)
    {
        
        if(ShowKeyFrame[i].videoNo != -1)
        {
            stringstream videoAviPath;
            videoAviPath << path << "/avi/" << ShowKeyFrame[i].videoNo <<".avi";
            //videoAviPath << ShowKeyFrame[i].videoNo;
            ShowKeyFrame[i].AP = to_string(ShowKeyFrame[i].videoNo);
            continue;
        }
        stringstream videoAviPath;
        if(aviCount < 10)
        {
            videoAviPath << path <<"/avi/"<< "0" << aviCount<<".avi";
        }
        else
        {
            videoAviPath << path <<"/avi/"<< aviCount<<".avi";
        }
        
        //videoAviPath <<aviCount;
        ShowKeyFrame[i].AP = to_string(aviCount);

        ShowKeyFrame[i].AP = ShowKeyFrame[i].AP;
        
        double fps = 30.0;                          // framerate of the created video stream
        //writer.open(videoAviPath.str(), codec, fps, video.size(), CV_8UC3);
        VideoWriter writer(videoAviPath.str(), VideoWriter::fourcc('M', 'J', 'P', 'G'), fps, Size(width, height), CV_8UC3);
        
        writer<<ShowKeyFrame[i].f;

        writer.release();
        aviCount++;
    }
    
    //output info: synopsis width, height, timer
    for(int i = 0; i < ShowKeyFrame.size(); i++)
    {
        
    }
    
    //check rgb
    /*Mat check(288, 21824, CV_8UC3);
    string videoFolderPath = "/Users/haha/Downloads/CSCI576ProjectMediaCopy/Synopsis.rgb";

       FILE * fp = fopen(videoFolderPath.c_str(), "rb");
       if (fp == NULL)
           cout<<"Cannot open this rgb file"<<videoFolderPath<<endl;
       else
       {
           fread(checkBuffer, sizeof(unsigned char), 3 * 21824*288, fp);
                       
           int rgb = 0;
           for(int h = 0; h < 288; h++)
           {
               for(int w = 0; w < 21824; w++)
               {
                   //b
                   check.at<Vec3b>(h, w)[0] = checkBuffer[rgb + 2 * 21824*288];
                   //g
                   check.at<Vec3b>(h, w)[1] = checkBuffer[rgb + 21824*288];
                   //r
                   check.at<Vec3b>(h, w)[2] = checkBuffer[rgb];
                   rgb++;
               }
           }

       fclose(fp);
       //free(buffer);
    }
    imshow("check", check);
    waitKey(0);*/
    
    //write "info.txt",
    string infoPath = path + "/path.txt";

    ofstream out(infoPath);
    if (out.is_open())
   {
       out<<synopsis.cols<<" ";
       out<<synopsis.rows<<endl;;
       //out<<path + "/avi"<<" ";;
       //out<<path<<endl;;
       
       for(int i = 0; i < ShowKeyFrame.size(); i++)
       {
           double timeStamp = 0;
           
           if(ShowKeyFrame[i].videoNo != -1)
           {
               //cout<<ShowKeyFrame[i].bestID<<endl;;
               timeStamp = ShowKeyFrame[i].bestID / 30.0;
           }
           //out<<ShowKeyFrame[i].AP[ShowKeyFrame[i].AP.length() - 5]<<endl;
           out<<ShowKeyFrame[i].AP<<" ";
           out<<timeStamp<<endl;
           
           
           
       }
        out.close();
    }

    
    return 0;
    
}
//ffmpeg -i /Users/haha/Downloads/CSCI576ProjectMediaCopy/video_1.wav   -i /Users/haha/Downloads/CSCI576ProjectMediaCopy/avi/0.avi  /Users/haha/Downloads/CSCI576ProjectMediaCopy/avi/outpu0.avi
