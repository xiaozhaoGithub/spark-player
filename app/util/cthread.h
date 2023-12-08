#ifndef CTHREAD_H_
#define CTHREAD_H_

#include <QThread>

class CThread : public QThread
{
    Q_OBJECT
public:
    enum SleepPolicy
    {
        kWait,
        kUntil
    };

    CThread(QObject* parent)
        : QThread(parent)
        , sleep_policy_(kWait)
        , sleep_ms_(50)
        , state_changed_(false)
    {}

    void set_sleep_policy(SleepPolicy policy, uint64_t ms)
    {
        sleep_policy_ = policy;
        sleep_ms_ = ms;
        state_changed_ = true;
    }

    void Sleep()
    {
        switch (sleep_policy_) {
        case kWait:
            break;
        case kUntil: {
            if (state_changed_) {
                base_tp_ = std::chrono::system_clock::now();
                state_changed_ = false;
            }

            base_tp_ += std::chrono::milliseconds(sleep_ms_);
            std::this_thread::sleep_until(base_tp_);
        } break;
        default:
            break;
        }
    }

private:
    SleepPolicy sleep_policy_;
    uint64_t sleep_ms_;
    std::atomic<bool> state_changed_;

    std::chrono::system_clock::time_point base_tp_;
};

#endif
