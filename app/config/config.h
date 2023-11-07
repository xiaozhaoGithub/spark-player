#ifndef CONFIG_H_
#define CONFIG_H_

#include <QObject>

class Config : public QObject
{
    Q_OBJECT
public:
    explicit Config(QObject* parent = nullptr);
    ~Config();

private:
};


#endif
