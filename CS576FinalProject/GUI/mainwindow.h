#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include<QMediaPlaylist>
#include<QMediaPlayer>
#include<QVideoWidget>
#include<QVBoxLayout>
#include<QMouseEvent>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
protected:
    void mousePressEvent(QMouseEvent *e);
private slots:
    void on_btn_start_clicked();

    void on_btn_pause_clicked();

    void on_btn_stop_clicked();

private:
    Ui::MainWindow *ui;
    QVBoxLayout* layout_video;//
    QMediaPlayer* videoPlayer,*soundPlayer;   //
    QVideoWidget* videoWidget;
    QMediaPlaylist *videoList,*soundList;
    //bool play_state;
    bool if_reload=false;
    int index;
private:
    void initData();
    void loadData(QString);//
    void loadPicture(QString picture_path);//
    void loadVideoFile(QString);//
    void loadSoundFile(QString);//
};
#endif // MAINWINDOW_H
