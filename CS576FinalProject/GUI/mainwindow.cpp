#include "mainwindow.h"
#include "ui_mainwindow.h"
#include<QFileDialog>
#include<QMessageBox>


using namespace  std;
 typedef struct mfiledata{
    int picture_index;
    double time;
}mfile;
// mfiledata temp_file;
//vector<mfiledata>file_data;
mfiledata *file_data=new mfiledata[59];
int flag=0;
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    loadData("/Users/haha/Downloads/untitled/data.txt");
    loadPicture("/Users/haha/Downloads/untitled/synopsis.jpg");
    loadVideoFile("/Users/haha/Downloads/untitled/video/avi");
    loadSoundFile("/Users/haha/Downloads/untitled/sound");
    initData();
}

MainWindow::~MainWindow()
{
    delete layout_video;
    delete videoPlayer ;
    delete videoWidget ;
    delete ui;
}

void MainWindow::mousePressEvent(QMouseEvent *e)
{

    if((e->button()==Qt::LeftButton)&&e->y()>350)
    {
        int x=-(ui->scrollAreaWidgetContents->x())+e->x()-20;//
        qDebug()<<x;
        index=x/352;
//        ui->label_num->setText(QString::number(index+1));

        qDebug()<<file_data[index].picture_index<<file_data[index].time;
        videoList->setCurrentIndex(file_data[index].picture_index);
        soundList->setCurrentIndex(file_data[index].picture_index);

//        qDebug()<<videoPlayer->currentMedia().canonicalResource().url().toString();
//        QFileInfo f;
//        f.setFile(videoPlayer->currentMedia().canonicalResource().url().toString());
//        qDebug()<<f.size();
//        if(file_data[index].time>(59))
//        {
//            videoPlayer->setPosition(file_data[index].time*1000);
//            videoPlayer->pause();
//            soundPlayer->pause();
//        }
//        else
        if(file_data[index].time==0)
        {
            videoPlayer->pause();
            soundPlayer->pause();


        }
        else
        {
            soundPlayer->setPosition(file_data[index].time*1000);
           soundPlayer->play();
            videoPlayer->setPosition(file_data[index].time*1000);
            videoPlayer->play();

        }

    }
}


void MainWindow::loadPicture(QString picture_path)
{

    QPixmap* pixmap = new QPixmap(picture_path);
    pixmap->scaled(ui->label->size(),Qt::KeepAspectRatio);
    ui->label->setScaledContents(true);
    ui->label->setPixmap(*pixmap);

}

void MainWindow::loadVideoFile(QString str)
{

    videoList=new QMediaPlaylist;
    videoList->setPlaybackMode(QMediaPlaylist::CurrentItemOnce);
    QDir dir(str);
    QStringList nameFilters;
    nameFilters << "*.avi";//
    QList<QFileInfo> *fileInfo=new QList<QFileInfo>(dir.entryInfoList(nameFilters));
    for(int i = 0;i<fileInfo->count(); i++)
    {
       qDebug()<<fileInfo->at(i).filePath();
       videoList->addMedia(QUrl::fromLocalFile(fileInfo->at(i).filePath()));
    }

}

void MainWindow::loadSoundFile(QString str)
{

    soundList=new QMediaPlaylist;
    soundList->setPlaybackMode(QMediaPlaylist::CurrentItemOnce);
    QDir dir(str);
    QStringList nameFilters;
    nameFilters << "*.wav";//
    QList<QFileInfo> *fileInfo=new QList<QFileInfo>(dir.entryInfoList(nameFilters));
    for(int i = 0;i<fileInfo->count(); i++)
    {
       qDebug()<<fileInfo->at(i).filePath();
       soundList->addMedia(QUrl::fromLocalFile(fileInfo->at(i).filePath()));
    }

}


void MainWindow::loadData(QString str)
{
   int i=0;
    qDebug()<<"***********read data*********************";
    QFile f;
    f.setFileName(str);
    f.open(QIODevice::ReadOnly);
//    if(!f.open(QIODevice::ReadOnly))
//    {
//       // QMessageBox::warning(this,"warning",f.errorString());
//        return ;
//    }

    while(!f.atEnd())
    {
        QString str=f.readLine();
         int per_index=(str.mid(0,str.indexOf(" "))).toInt();
        double per_position=(str.mid(str.indexOf(" ")+1,str.length()-str.indexOf(" ")-1)).toDouble();
        qDebug()<<str<<per_position<<per_index;
       /* temp_file.time=per_position;
        temp_file.picture_index=per_index;*/
//        file_data.push_back(temp_file);
        file_data[i].time=per_position;
        file_data[i].picture_index=per_index;
        i++;
    }
    f.close();
    qDebug()<<QString("***********read%1行*********************").arg(i);
//    for(int j=0;j<i;j++)
//    {
//        qDebug()<<file_data[j].time<<file_data[j].picture_index;
//    }
}


void MainWindow::on_btn_start_clicked()
{
    if(file_data[index].time==0)        return;
    if(flag==1)
    {
            soundPlayer->setPosition(0);
            videoPlayer->setPosition(0);
            flag=0;
    }
    soundPlayer->play();
    videoPlayer->play();
}

void MainWindow::initData()
{
    layout_video = new QVBoxLayout;
    videoPlayer = new QMediaPlayer;
    soundPlayer = new QMediaPlayer;
    videoWidget = new QVideoWidget;
    layout_video->setMargin(1);
    ui->label_mp4->setMargin(1);
    videoWidget->resize(ui->label_mp4->size());
    layout_video->addWidget(videoWidget);
    layout()->addWidget(videoWidget);
    ui->label_mp4->setLayout(layout_video);
    videoPlayer->setVideoOutput(videoWidget);
    videoPlayer->setPlaylist(videoList);
    soundPlayer->setPlaylist(soundList);
//    videoPlayer->play();
//    soundPlayer->play();
    videoPlayer->setVolume(0);
  //  soundPlayer->setVolume(0);
}

void MainWindow::on_btn_pause_clicked()
{
    soundPlayer->pause();
    videoPlayer->pause();
}

void MainWindow::on_btn_stop_clicked()
{


    videoPlayer->pause();
    soundPlayer->pause();
//    soundPlayer->setPosition(0);
//    videoPlayer->setPosition(0);
    flag=1;

}


