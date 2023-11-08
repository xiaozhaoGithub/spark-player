#include "config.h"

Config::Config(QObject* parent)
    : QObject(parent)
{
    char config_path[256] = {0};
    GetConfigFileDir(config_path, 256);
    app_settings_ = new QSettings(QString(config_path) + "/config.ini", QSettings::IniFormat);
}

Config::~Config()
{
    if (app_settings_) {
        // m_appSettings->sync();
        delete app_settings_;
        app_settings_ = nullptr;
    }
}

void Config::SetAppConfigData(const QString& group, const QString& key, const QVariant& value)
{
    SetConfigData(app_settings_, group, key, value);
}

QVariant Config::AppConfigData(const QString& group, const QString& key, const QVariant& default)
{
    return AppConfigData(app_settings_, group, key, default);
}

QVariant Config::AppConfigData(QSettings* settings, const QString& group, const QString& key,
                               const QVariant& default)
{
    if (group.isEmpty() || key.isEmpty() || !settings)
        return default;

    return settings->value(QString("%1/%2").arg(group).arg(key), default);
}

void Config::SetConfigData(QSettings* settings, const QString& group, const QString& key,
                           const QVariant& value)
{
    if (group.isEmpty() || key.isEmpty() || !settings)
        return;

    app_settings_->setValue(QString("%1/%2").arg(group, key), value);
    app_settings_->sync();
}

quint32 Config::GetConfigFileDir(char* filename, quint32 size)
{
    char system_path[128] = {0};
    GetSystemDirectoryA(system_path, 128);

    char* pos = strchr(system_path, '\\');
    system_path[pos - system_path] = '\0';
    strcat(system_path, "\\ProgramData\\ffmpeg-player\\");

    memset(filename, 0, size);
    memcpy(filename, system_path, (std::min<size_t>)(strlen(system_path), size));

    size = (quint32)strlen(filename);
    return size;
}
