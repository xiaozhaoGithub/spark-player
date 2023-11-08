#ifndef CONFIG_H_
#define CONFIG_H_

#include <QObject>
#include <QSettings>
#include <Windows.h>

class Config : public QObject
{
    Q_OBJECT
public:
    explicit Config(QObject* parent = nullptr);
    ~Config();

    void SetAppConfigData(const QString& group, const QString& key, const QVariant& value);
    QVariant AppConfigData(const QString& group, const QString& key, const QVariant& default);

    quint32 GetConfigFileDir(char* filename, quint32 size);

private:
    void SetConfigData(QSettings* settings, const QString& group, const QString& key,
                       const QVariant& value);
    QVariant AppConfigData(QSettings* settings, const QString& group, const QString& key,
                           const QVariant& default);

private:
    QSettings* app_settings_;
};

#endif
