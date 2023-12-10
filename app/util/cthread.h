#ifndef CTHREAD_H_
#define CTHREAD_H_

#include <QThread>

class CThread : public QThread
{
    Q_OBJECT
public:
    enum State
    {
        kRunning,
        kPause,
        kStop
    };

    enum SleepPolicy
    {
        kYield,
        kWait,
        kUntil
    };

    CThread(QObject* parent)
        : QThread(parent)
        , state_(kStop)
        , sleep_policy_(kWait)
        , sleep_ms_(50)
        , state_changed_(false)
    {
        setTerminationEnabled(false);
    }

    State state() { return state_; }
    void set_state(State state)
    {
        state_ = state;
        state_changed_ = true; // Must be reset base time point, otherwise dropped frame.
    }

    void set_sleep_policy(SleepPolicy policy, uint64_t ms)
    {
        sleep_policy_ = policy;
        sleep_ms_ = ms;
        state_changed_ = true;
    }

    void Sleep()
    {
        switch (sleep_policy_) {
        case kYield:
            std::this_thread::yield();
            break;
        case kWait:
            std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms_));
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

protected:
    virtual bool DoPrepare() = 0;
    virtual void DoTask() = 0;
    virtual void DoFinish() = 0;

private:
    void run() override
    {
        if (!DoPrepare())
            return;

        DoTask();

        DoFinish();
    }

private:
    std::atomic<State> state_;
    SleepPolicy sleep_policy_;
    uint64_t sleep_ms_;
    std::atomic<bool> state_changed_;

    std::chrono::system_clock::time_point base_tp_;
};

#endif
