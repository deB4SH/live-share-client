#include <QtGlobal> // qWarning
#include <QSettings>
#include <QHotkey>
#include <QFile>
#include "RecordingManager.h"


static QKeySequence DefaultImageShortcut = QKeySequence();
static QKeySequence DefaultVideoShortcut = QKeySequence();

RecordingManager::RecordingManager(QObject *parent) :
    QObject(parent),
    imageHotkey(new QHotkey(this)),
    videoHotkey(new QHotkey(this)),
    _recorder(new Recorder(this))
{
    // Hotkey --{activated}-> RecordingManager
    connect(imageHotkey, &QHotkey::activated,
            this, &RecordingManager::imageHotkeyActivated);
    connect(videoHotkey, &QHotkey::activated,
            this, &RecordingManager::videoHotkeyActivated);

    // Hotkey --{registered}-> RecordingManager
    connect(imageHotkey, &QHotkey::registeredChanged,
            this, &RecordingManager::imageHotkeyRegisteredChanged);
    connect(videoHotkey, &QHotkey::registeredChanged,
            this, &RecordingManager::videoHotkeyRegisteredChanged);

    // Recorder --{finished}-> RecordingManager
    connect(_recorder, &Recorder::finished,
            this, &RecordingManager::recorderFinished);
}

RecordingManager::~RecordingManager()
{
}

void RecordingManager::readSettings(QSettings &settings)
{
    setImageShortcut(settings.value("imageShortcut", DefaultImageShortcut).value<QKeySequence>());
    setVideoShortcut(settings.value("videoShortcut", DefaultVideoShortcut).value<QKeySequence>());

    emit imageShortcutChanged(imageShortcut());
    emit videoShortcutChanged(videoShortcut());

    _recorder->readSettings(settings);
}

void RecordingManager::writeSettings(QSettings &settings)
{
    settings.setValue("imageShortcut", imageShortcut());
    settings.setValue("videoShortcut", videoShortcut());

    _recorder->writeSettings(settings);
}


// ---- Getters ----

QKeySequence RecordingManager::imageShortcut() const
{
    return imageHotkey->shortcut();
}

QKeySequence RecordingManager::videoShortcut() const
{
    return videoHotkey->shortcut();
}

Recorder *RecordingManager::recorder() const
{
    return _recorder;
}


// ---- Setters ----

void RecordingManager::setImageShortcut(QKeySequence shortcut)
{
    const QKeySequence oldShortcut = imageShortcut();
    if(!imageHotkey->setShortcut(shortcut, true)) // set and register
    {
        qWarning("can't set image hotkey");
        emit imageShortcutChanged(oldShortcut);
    }
}

void RecordingManager::setVideoShortcut(QKeySequence shortcut)
{
    const QKeySequence oldShortcut = videoShortcut();
    if(!videoHotkey->setShortcut(shortcut, true)) // set and register
    {
        qWarning("can't set video hotkey");
        emit videoShortcutChanged(oldShortcut);
    }
}


// ---- Others Slots ----

void RecordingManager::imageHotkeyActivated()
{
    qWarning("image hotkey activated");
    if(!_recorder->isRecording())
        startRecording(Recorder::Image);
}

void RecordingManager::videoHotkeyActivated()
{
    qWarning("video hotkey activated");
    if(_recorder->isRecording())
    {
        if(_recorder->recordType() == Recorder::Video)
            _recorder->stop();
    }
    else
        startRecording(Recorder::Video);
}

void RecordingManager::imageHotkeyRegisteredChanged(bool registered)
{
    qWarning("image hotkey registered: %d", (int)registered);
}

void RecordingManager::videoHotkeyRegisteredChanged(bool registered)
{
    qWarning("video hotkey registered: %d", (int)registered);
}

void RecordingManager::recorderFinished(QFile *outputFile)
{
    if(outputFile)
    {
        qWarning("recorderFinished %s (%s)",
            qUtf8Printable(outputFile->fileName()),
            qUtf8Printable(_recorder->mimeType()));
        emit recordingFinished(outputFile);
    }
    else
        qWarning("on_recorder_finished null");
}

void RecordingManager::startRecording(Recorder::RecordType recordType)
{
    Q_ASSERT(!_recorder->isRecording());

    WindowInfo windowInfo;
    if(!GetInfoOfActiveWindow(&windowInfo))
    {
        qWarning("GetInfoOfActiveWindow failed");
        return;
    }

    qWarning("XXX DPI: %d", windowInfo.dpi);
    qWarning("XXX Fullscreen: %d", (int)windowInfo.isFullscreen);

    _recorder->setWindowInfo(windowInfo);
    _recorder->setRecordType(recordType);
    _recorder->start();
}
