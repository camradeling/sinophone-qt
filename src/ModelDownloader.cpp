#include "ModelDownloader.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QFile>
#include <QDir>
#include <QStandardPaths>

static const QMap<QString, QStringList> kModelUrls = {
    {"Qwen2.5-1.5B-Q4", {
        "https://huggingface.co/Qwen/Qwen2.5-1.5B-Instruct-GGUF/resolve/main/qwen2.5-1.5b-instruct-q4_k_m.gguf",
    }},
    {"Qwen2.5-3B-Q4", {
        "https://huggingface.co/Qwen/Qwen2.5-3B-Instruct-GGUF/resolve/main/qwen2.5-3b-instruct-q4_k_m.gguf",
    }},
    {"Qwen2.5-7B-Q4", {
        "https://huggingface.co/Qwen/Qwen2.5-7B-Instruct-GGUF/resolve/main/qwen2.5-7b-instruct-q4_k_m-00001-of-00002.gguf",
        "https://huggingface.co/Qwen/Qwen2.5-7B-Instruct-GGUF/resolve/main/qwen2.5-7b-instruct-q4_k_m-00002-of-00002.gguf",
    }},
    {"Qwen2.5-14B-Q4", {
        "https://huggingface.co/Qwen/Qwen2.5-14B-Instruct-GGUF/resolve/main/qwen2.5-14b-instruct-q4_k_m-00001-of-00003.gguf",
        "https://huggingface.co/Qwen/Qwen2.5-14B-Instruct-GGUF/resolve/main/qwen2.5-14b-instruct-q4_k_m-00002-of-00003.gguf",
        "https://huggingface.co/Qwen/Qwen2.5-14B-Instruct-GGUF/resolve/main/qwen2.5-14b-instruct-q4_k_m-00003-of-00003.gguf",
    }},
};

ModelDownloader::ModelDownloader(QObject* parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{}

ModelDownloader::~ModelDownloader()
{
    cancel();
}

QString ModelDownloader::sharedModelsDir()
{
#ifdef Q_OS_ANDROID
    // Android is sandboxed — use app-local storage under a sinophone/ subfolder
    // so the path is at least consistent across sinophone apps on the same device
    // if they ever gain cross-app storage permissions.
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
           + "/sinophone/models";
#else
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
           + "/sinophone/models";
#endif
}

QString ModelDownloader::findModel(const QString& filename)
{
    QString path = sharedModelsDir() + "/" + filename;
    return QFile::exists(path) ? path : QString{};
}

void ModelDownloader::download(const QString& modelId)
{
    if (m_busy) return;

    QStringList urls = kModelUrls.value(modelId);
    if (urls.isEmpty()) {
        emit finished(false, {}, "Unknown model: " + modelId);
        return;
    }

    m_queue          = urls;
    m_downloadDir    = sharedModelsDir();
    m_shardIndex     = 0;
    m_shardCount     = urls.size();
    m_firstShardPath.clear();
    QDir().mkpath(m_downloadDir);

    setBusy(true);
    setProgress(0.0);
    downloadNextShard();
}

void ModelDownloader::downloadNextShard()
{
    if (m_queue.isEmpty()) {
        setBusy(false);
        setStatusText({});
        emit finished(true, m_firstShardPath, {});
        return;
    }

    QString url      = m_queue.takeFirst();
    QString filename = url.section('/', -1);
    QString dest     = m_downloadDir + "/" + filename;

    ++m_shardIndex;
    if (m_shardIndex == 1) m_firstShardPath = dest;

    if (m_shardCount > 1)
        setStatusText(QString("Downloading shard %1 of %2…").arg(m_shardIndex).arg(m_shardCount));

    m_file = new QFile(dest, this);
    if (!m_file->open(QIODevice::WriteOnly)) {
        m_queue.clear();
        setBusy(false);
        emit finished(false, {}, "Cannot write: " + dest);
        delete m_file; m_file = nullptr;
        return;
    }

    QNetworkRequest req{QUrl(url)};
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);
    m_reply = m_nam->get(req);

    connect(m_reply, &QNetworkReply::readyRead, this, [this]() {
        if (m_file) m_file->write(m_reply->readAll());
    });

    connect(m_reply, &QNetworkReply::downloadProgress, this,
            [this](qint64 recv, qint64 total) {
        if (total > 0) setProgress(qreal(recv) / qreal(total));
    });

    connect(m_reply, &QNetworkReply::finished, this, [this, dest]() {
        m_file->close();
        bool ok  = (m_reply->error() == QNetworkReply::NoError);
        QString err = ok ? QString{} : m_reply->errorString();

        m_reply->deleteLater(); m_reply = nullptr;
        delete m_file;          m_file  = nullptr;

        if (!ok) {
            QFile::remove(dest);
            m_queue.clear();
            setBusy(false);
            emit finished(false, {}, err);
            return;
        }

        setProgress(0.0);
        downloadNextShard();
    });
}

void ModelDownloader::cancel()
{
    m_queue.clear();
    if (m_reply) m_reply->abort();
}

void ModelDownloader::setBusy(bool v)
{
    if (m_busy == v) return;
    m_busy = v;
    emit busyChanged();
}

void ModelDownloader::setProgress(qreal v)
{
    if (qFuzzyCompare(m_progress, v)) return;
    m_progress = v;
    emit progressChanged();
}

void ModelDownloader::setStatusText(const QString& t)
{
    if (m_statusText == t) return;
    m_statusText = t;
    emit statusTextChanged();
}
