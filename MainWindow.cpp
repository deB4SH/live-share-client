#include <QtGlobal> // qWarning
#include <QSettings>
#include <QCloseEvent>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QFileDialog>
#include <QNetworkReply>
#include <QMimeDatabase>
#include <QSignalBlocker>
#include <QHotkey>
#include "ExecutableValidator.h"
#include "WindowInfo.h"
#include "RecordingManager.h"
#include "Recorder.h"
#include "MainWindow.h"
#include "ui_MainWindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    recordingManager(new RecordingManager(this))
    //networkAccessManager(new QNetworkAccessManager(this)),
    //uploadedFile(nullptr),
{
    ui->setupUi(this);

    // TODO:
    // ui->categoryLineEdit->setValidator(); - no / at start or end; only URL safe characters
    {
        // Protocol must be http or https
        // Must not end with a slash
        const QRegularExpression regExp("https?:\\/\\/[^\\s]*[^\\/]");
        QValidator *validator = new QRegularExpressionValidator(regExp, this);
        ui->urlLineEdit->setValidator(validator);
    }

    {
        ExecutableValidator *validator = new ExecutableValidator(this);
        ui->ffmpegLineEdit->setValidator(validator);
    }

    Recorder *recorder = recordingManager->recorder();

    // Widget --{changed}-> RecordingManager
    connect(ui->imageKeySequenceEdit, &KeySequence_Widget::keySequenceChanged,
            recordingManager, &RecordingManager::setImageShortcut);
    connect(ui->videoKeySequenceEdit, &KeySequence_Widget::keySequenceChanged,
            recordingManager, &RecordingManager::setVideoShortcut);

    // Widget --{changed}-> Recorder
    connect(ui->maxImageEdgeLengthSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            recorder, &Recorder::setMaxImageEdgeLength);
    connect(ui->maxVideoEdgeLengthSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            recorder, &Recorder::setMaxVideoEdgeLength);
    connect(ui->frameRateDoubleSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            recorder, &Recorder::setVideoFrameRate);
    connect(ui->maxVideoLengthSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            recorder, &Recorder::setMaxVideoLength);

    // HotkeyResetButton --{clear}-> KeySequenceEdit
    connect(ui->imageHotkeyResetButton, &QToolButton::clicked,
            ui->imageKeySequenceEdit, &KeySequence_Widget::clear);
    connect(ui->videoHotkeyResetButton, &QToolButton::clicked,
            ui->videoKeySequenceEdit, &KeySequence_Widget::clear);

    // RecordingManager --{changed}-> Widget
    connect(recordingManager, &RecordingManager::imageShortcutChanged,
            ui->imageKeySequenceEdit, &KeySequence_Widget::setKeySequence);
    connect(recordingManager, &RecordingManager::videoShortcutChanged,
            ui->videoKeySequenceEdit, &KeySequence_Widget::setKeySequence);

    // Recorder --{changed}-> Widget
    connect(recorder, &Recorder::ffmpegExecutableChanged,
            ui->ffmpegLineEdit, &QLineEdit::setText);
    connect(recorder, &Recorder::maxImageEdgeLengthChanged,
            ui->maxImageEdgeLengthSpinBox, &QSpinBox::setValue);
    connect(recorder, &Recorder::maxVideoEdgeLengthChanged,
            ui->maxVideoEdgeLengthSpinBox, &QSpinBox::setValue);
    connect(recorder, &Recorder::videoFrameRateChanged,
            ui->frameRateDoubleSpinBox, &QDoubleSpinBox::setValue);
    connect(recorder, &Recorder::maxVideoLengthChanged,
            ui->maxVideoLengthSpinBox, &QSpinBox::setValue);

    const QSignalBlocker imageKeySequenceBlocker(ui->imageKeySequenceEdit);
    const QSignalBlocker videoKeySequenceBlocker(ui->videoKeySequenceEdit);
    const QSignalBlocker ffmpegExecutableBlocker(ui->ffmpegLineEdit);
    const QSignalBlocker maxImageEdgeLengthBlocker(ui->maxImageEdgeLengthSpinBox);
    const QSignalBlocker maxVideoEdgeLengthBlocker(ui->maxVideoEdgeLengthSpinBox);
    const QSignalBlocker videoFrameRateBlocker(ui->frameRateDoubleSpinBox);
    const QSignalBlocker maxVideoLengthBlocker(ui->maxVideoLengthSpinBox);
    readSettings();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::readSettings()
{
    QSettings settings;
    //qWarning("read settings %s", qUtf8Printable(settings.fileName()));

    settings.beginGroup("recording");
    recordingManager->readSettings(settings);
    settings.endGroup();
}

void MainWindow::writeSettings()
{
    QSettings settings;
    //qWarning("write settings %s", qUtf8Printable(settings.fileName()));

    settings.beginGroup("recording");
    recordingManager->writeSettings(settings);
    settings.endGroup();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    writeSettings();
    event->accept();
}

/*
void MainWindow::uploadFile(const QString& fileName)
{
    Q_ASSERT(uploadedFile == nullptr);
    uploadedFile = new QFile(fileName);
    uploadedFile->open(QIODevice::ReadOnly);

    const QString url = serviceUrl + QString("/user/") + uploadedFile->fileName();

    QMimeDatabase mimeDb;
    const QMimeType mimeType = mimeDb.mimeTypeForFile(fileName);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, mimeType.name());

    QNetworkReply* reply = networkAccessManager->post(request, uploadedFile);
    reply->deleteLater();

    connect(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error),
        [=](QNetworkReply::NetworkError code){});

    connect(reply, &QNetworkReply::uploadProgress,
            this, [this](qint64 sent, qint64 total){ printf("upload %lld/%lld\n", sent, total); });
    connect(reply, &QNetworkReply::finished,
            this, [this](){ printf("upload complete\n"); });
}
*/

void MainWindow::on_urlLineEdit_editingFinished()
{
    //serviceUrl = ui->urlLineEdit->text();
}

void MainWindow::on_ffmpegLineEdit_editingFinished()
{
    recordingManager->recorder()->setFfmpegExecutable(ui->ffmpegLineEdit->text());
}

void MainWindow::on_ffmpegFileDialogButton_clicked()
{
#if defined(_WIN32)
    const char *filter = "*.exe";
#else
    const char *filter = "";
#endif

    const QString selection =
        QFileDialog::getOpenFileName(this,
                                     tr("Select FFmpeg executable"), // caption
                                     recordingManager->recorder()->ffmpegExecutable(), // start dir/file
                                     filter);
    if(!selection.isNull())
    {
        ui->ffmpegLineEdit->setText(selection);
        recordingManager->recorder()->setFfmpegExecutable(selection);
    }

    /*
    QFileDialog dialog(this);
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    //dialog.setFileMode(QFileDialog::ExistingFile);
#if defined(_WIN32)
    dialog.setNameFilter("*.exe");
#endif
    if(dialog.exec() == QDialog::Accepted)
    {
        const QString ffmpegLocation = dialog.selectedFiles().first();
        ui->ffmpegLineEdit->setText(ffmpegLocation);
        recordingManager->setFfmpegExecutable(ffmpegLocation);
    }
    */
}