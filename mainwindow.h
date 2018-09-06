#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextToSpeech>
#include <QTextCursor>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_btnTalk_clicked();
    void on_btnPause_clicked();
    void on_action_Quit_triggered();
    void on_actionOpen_triggered();
    void on_volumeSlider_valueChanged(int value);
    void on_actionTalk_triggered();
    void on_actionPause_triggered();
    void on_speech_stateChanged(QTextToSpeech::State state);
    void on_actionMove_to_speech_line_triggered();
    void on_actionSkip_to_10_line_triggered();
    void on_btnSearchText_clicked();
    void on_txtSearchText_returnPressed();
    void on_chkAllowScroll_stateChanged(int value);
    void on_btnJumpToLine_clicked();
    void on_boxJumpLine_valueChanged(int value);
    void on_btnStop_clicked();
    void on_boxVolume_valueChanged(int value);
    void on_boxVolume_editingFinished();
    void on_boxJumpLine_editingFinished();
    void on_actionAbout_triggered();
    void on_actionVolume_Up_triggered();
    void on_actionVolume_Down_triggered();

private:
    int currentLine;
    bool isAllowScroll;
    bool isSpeaking;
    bool isForceStop;
    Ui::MainWindow *ui;
    QTextToSpeech *speech;
    QTextCursor cursor;
    QStringList allTextList;
    void MoveToLine(int lineIndex);
    void MoveToLastSpeechLine();
    void StopSpeech();
    void JumpToCertainLine(int index);
    void UpdateLineLabel(int index);
    int SearchText(QString text, int from);
    bool IsCanFindNextValidLine();
};

#endif // MAINWINDOW_H
