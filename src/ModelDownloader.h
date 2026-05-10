#pragma once
#include <QObject>
#include <QString>
#include <QStringList>
#include <QtQml/qqmlregistration.h>

class QNetworkAccessManager;
class QNetworkReply;
class QFile;

// Self-contained model downloader for sinophone-family apps.
// Downloads Qwen2.5 GGUF models from Hugging Face into a shared directory
// so that multiple sinophone apps (doramaplus, confucius, etc.) reuse the
// same files without duplicate downloads.
class ModelDownloader : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool    busy       READ busy       NOTIFY busyChanged)
    Q_PROPERTY(qreal   progress   READ progress   NOTIFY progressChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)

public:
    explicit ModelDownloader(QObject* parent = nullptr);
    ~ModelDownloader() override;

    bool    busy()       const { return m_busy; }
    qreal   progress()   const { return m_progress; }
    QString statusText() const { return m_statusText; }

    // Shared models dir: ~/.local/share/sinophone/models/ on Linux,
    // QStandardPaths::AppDataLocation/sinophone/models on Android.
    Q_INVOKABLE static QString sharedModelsDir();

    // Returns the full path if filename exists in sharedModelsDir(), else "".
    Q_INVOKABLE static QString findModel(const QString& filename);

    Q_INVOKABLE void download(const QString& modelId);
    Q_INVOKABLE void cancel();

signals:
    void busyChanged();
    void progressChanged();
    void statusTextChanged();
    // path is the first-shard path (llama.cpp auto-discovers remaining shards)
    void finished(bool success, const QString& path, const QString& error);

private:
    void downloadNextShard();
    void setBusy(bool v);
    void setProgress(qreal v);
    void setStatusText(const QString& t);

    QNetworkAccessManager* m_nam            = nullptr;
    QNetworkReply*         m_reply          = nullptr;
    QFile*                 m_file           = nullptr;
    QStringList            m_queue;
    QString                m_downloadDir;
    QString                m_firstShardPath;
    int                    m_shardIndex     = 0;
    int                    m_shardCount     = 0;
    bool                   m_busy           = false;
    qreal                  m_progress       = 0.0;
    QString                m_statusText;
};
