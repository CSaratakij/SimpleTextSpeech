#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtTextToSpeech>
#include <QMessageBox>
#include <QFileDialog>
#include <QTextBlock>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QString engine = QTextToSpeech::availableEngines()[0];
    speech = new QTextToSpeech(engine, this);

    QVoice voice(speech->availableVoices()[0]);
    QLocale locale(speech->availableLocales()[0]);

    speech->setPitch(3 / 10.0);
    speech->setRate(1 / 10.0);
    speech->setVolume(ui->volumeSlider->value() / 100.0);
    speech->setVoice(voice);
    speech->setLocale(locale);

    isSpeaking = false;
    isAllowScroll = true;
    isForceStop = false;

    currentLine = 0;
    allTextList.append("");

    cursor = ui->txtEditor->textCursor();
    cursor.movePosition(QTextCursor::Start);

    ui->txtEditor->setTextCursor(cursor);
    ui->txtEditor->setFocus(Qt::FocusReason::MouseFocusReason);

    ui->chkAllowScroll->setChecked(isAllowScroll);
    ui->boxVolume->setValue(ui->volumeSlider->value());

    connect(speech, &QTextToSpeech::stateChanged, this, &MainWindow::on_speech_stateChanged);
}

MainWindow::~MainWindow()
{
    delete ui;
    delete speech;
}

void MainWindow::on_btnTalk_clicked()
{
    QString text = ui->txtEditor->toPlainText();
    text = text.trimmed();

    if (text.isEmpty() || text.isNull())
        return;

    if (speech->state() == QTextToSpeech::BackendError) {
        ui->statusBar->showMessage("Can't initialize TextToSpeechEngine");
        return;
    }

    if (speech->state() != QTextToSpeech::Ready) {
        int result = QMessageBox::warning(this, "Warning", "Are you sure to start over?", QMessageBox::Yes, QMessageBox::No);
        if (QMessageBox::Yes == QMessageBox::StandardButton(result)) {
            speech->stop();
            ui->btnPause->setText("Pause");
        }
        else {
            return;
        }
    }

    ui->txtEditor->clear();
    ui->txtEditor->setText(text);

    allTextList.clear();
    allTextList = text.split("\n");

    currentLine = 0;
    bool isValid = IsCanFindNextValidLine();

    if (!isValid)
        return;

    MoveToLine(currentLine);

    isForceStop = false;
    isSpeaking = true;

    speech->say(allTextList[currentLine].trimmed());

    ui->txtEditor->setReadOnly(true);
    ui->statusBar->showMessage("Speaking...");
}

void MainWindow::on_speech_stateChanged(QTextToSpeech::State state)
{
    if (isForceStop) {
        speech->stop();
        isSpeaking = false;
        ui->btnPause->setText("Pause");
        ui->statusBar->showMessage("Stopped...");
        ui->txtEditor->setReadOnly(false);
        return;
    }

    if (QTextToSpeech::Ready == state) {
        if (isSpeaking) {
            isSpeaking = false;
            currentLine += 1;

            if (currentLine < allTextList.length()) {
                bool isValid = IsCanFindNextValidLine();
                if (isValid) {
                    speech->say(allTextList[currentLine].trimmed());
                    MoveToLastSpeechLine();
                }
            }
        }
    }
    else if (QTextToSpeech::Speaking == state) {
        isSpeaking = true;
        speech->setVolume(ui->volumeSlider->value() / 100.0);
        speech->setRate(ui->boxSpeechSpeed->value() / 10.0);
    }
}

void MainWindow::on_btnPause_clicked()
{
    if (speech->state() == QTextToSpeech::Speaking) {
        speech->pause();
        ui->btnPause->setText("Resume");
        ui->statusBar->showMessage("Pause...");
    }
    else if (speech->state() == QTextToSpeech::Paused) {
        speech->resume();
        ui->btnPause->setText("Pause");
        ui->statusBar->showMessage("Resume...");
    }
    else {
        return;
    }
}

void MainWindow::on_action_Quit_triggered()
{
    int result = QMessageBox::warning(this, "Warning", "Are you sure to quit?", QMessageBox::Yes, QMessageBox::No);
    if (QMessageBox::Yes == QMessageBox::StandardButton(result))
        QApplication::quit();
}

void MainWindow::on_actionOpen_triggered()
{
    QString path = QFileDialog::getOpenFileName(this, "Open file", "", "Text (*.txt);;All File (*.*)");

    if (path.isEmpty())
        return;

    if (!QFile::exists((path)))
            return;

    QFile file(path);
    bool isCanOpen = file.open(QIODevice::ReadOnly);

    if (!isCanOpen)
        return;

    QTextStream stream(&file);
    stream.setAutoDetectUnicode(true);

    QString allText = stream.readAll();
    allText = allText.trimmed();

    ui->txtEditor->clear();
    ui->txtEditor->insertPlainText(allText);

    file.close();
    isForceStop = true;
}

void MainWindow::on_volumeSlider_valueChanged(int value)
{
   speech->setVolume(value / 100.0);
   ui->boxVolume->setValue(value);
}

void MainWindow::on_actionTalk_triggered()
{
    emit this->on_btnTalk_clicked();
}

void MainWindow::on_actionPause_triggered()
{
    emit this->on_btnPause_clicked();
}

void MainWindow::on_actionMove_to_speech_line_triggered()
{
    MoveToLastSpeechLine();
}

void MainWindow::on_actionSkip_to_10_line_triggered()
{
    if (speech->state() != QTextToSpeech::Ready)
        speech->stop();

    MoveToLine(currentLine + 10);
    ui->btnPause->setText("Pause");
    speech->say(allTextList[currentLine].trimmed());
}

void MainWindow::on_btnSearchText_clicked()
{
    QString text = ui->txtSearchText->text();

    if (text.isEmpty() | text.isNull())
        return;

    int index = SearchText(text, 0);

    if (index == -1)
        return;

    if (speech->state() != QTextToSpeech::Ready)
        speech->stop();

    ui->btnPause->setText("Pause");
    ui->txtSearchText->setText("");

    MoveToLine(index);
    speech->say(allTextList[index].trimmed());
}

void MainWindow::on_txtSearchText_returnPressed()
{
    emit this->on_btnSearchText_clicked();
}

void MainWindow::on_chkAllowScroll_stateChanged(int state)
{
    isAllowScroll = (Qt::CheckState::Checked == state) ? true : false;
    if (!isAllowScroll)
        ui->txtEditor->textCursor().clearSelection();
}

void MainWindow::on_btnJumpToLine_clicked()
{
    int index = ui->boxJumpLine->value();
    if (index >= allTextList.length()) {
        index = currentLine;
        ui->boxJumpLine->setValue(index);
        return;
    }
    JumpToCertainLine(index);
}

void MainWindow::on_boxJumpLine_valueChanged(int value)
{
    if (value >= allTextList.length()) {
        value = currentLine;
        ui->boxJumpLine->setValue(value);
        return;
    }
    JumpToCertainLine(value);
}

void MainWindow::on_btnStop_clicked()
{
    StopSpeech();
}

void MainWindow::on_boxVolume_valueChanged(int value)
{
    int volumeSliderValue = ui->volumeSlider->value();
    if (value != volumeSliderValue)
        ui->volumeSlider->setValue(value);
}

void MainWindow::on_boxVolume_editingFinished()
{
    ui->boxVolume->clearFocus();
}

void MainWindow::on_boxJumpLine_editingFinished()
{
    ui->boxJumpLine->clearFocus();
}

void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::information(this, "About", "Author : Chatchai Saratakij", QMessageBox::Ok);
}

void MainWindow::on_actionVolume_Up_triggered()
{
    int maxVolume = ui->boxVolume->maximum();
    int currentVolume = ui->boxVolume->value();

    currentVolume = (currentVolume + 10) > maxVolume ? maxVolume : (currentVolume + 10);
    ui->boxVolume->setValue(currentVolume);

    emit this->on_boxVolume_valueChanged(currentVolume);
}

void MainWindow::on_actionVolume_Down_triggered()
{
    int minimumVolume = ui->boxVolume->minimum();
    int currentVolume = ui->boxVolume->value();

    currentVolume = (currentVolume - 10) < minimumVolume ? minimumVolume : (currentVolume - 10);
    ui->boxVolume->setValue(currentVolume);

    emit this->on_boxVolume_valueChanged(currentVolume);
}

void MainWindow::on_boxSpeechSpeed_valueChanged(int value)
{
    double rate = (value / 10.0);
    if (speech->rate() > rate || speech->rate() < rate) {
        speech->setRate(rate);
    }
}

void MainWindow::on_boxSpeechSpeed_editingFinished()
{
    ui->boxSpeechSpeed->clearFocus();
}

bool MainWindow::IsCanFindNextValidLine()
{
    bool isValid = false;
    QString line;

    do {
        if (currentLine >= allTextList.length()) {
            currentLine = 0;
            isValid = false;
            StopSpeech();
            break;
        }

        line = allTextList[currentLine].trimmed();
        isValid = !(line.isNull() || line.isEmpty() || line == "\n");

        if (!isValid)
            currentLine += 1;
    }
    while (!isValid);
    return isValid;
}

void MainWindow::MoveToLine(int lineIndex)
{
    if (lineIndex >= allTextList.length())
        return;

    if (!isAllowScroll)
        return;

    currentLine = lineIndex;

    QTextDocument *doc = ui->txtEditor->document();
    QTextCursor cursor(doc->findBlockByLineNumber(currentLine));

    cursor.select(QTextCursor::BlockUnderCursor);

    ui->txtEditor->moveCursor(QTextCursor::End);
    ui->txtEditor->setTextCursor(cursor);

    UpdateLineLabel(currentLine);
}

void MainWindow::MoveToLastSpeechLine()
{
    MoveToLine(currentLine);
}

int MainWindow::SearchText(QString text, int from)
{
    int index = allTextList.indexOf(text, from);
    return index;
}

void MainWindow::StopSpeech()
{
    isForceStop = true;
    speech->stop();
    ui->btnPause->setText("Pause");
    ui->statusBar->showMessage("Stopped...");
    ui->txtEditor->setReadOnly(false);
}

void MainWindow::JumpToCertainLine(int index)
{
    if (speech->state() != QTextToSpeech::Ready)
        speech->stop();

    MoveToLine(index);

    ui->btnPause->setText("Pause");
    speech->say(allTextList[currentLine].trimmed());
}

void MainWindow::UpdateLineLabel(int index)
{
    QString text = QString("Line : %1").arg(index);
    ui->lblLine->setText(text);
}
