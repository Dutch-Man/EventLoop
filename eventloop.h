#ifndef EVENT_LOOP_H_
#define EVENT_LOOP_H_

#include <stdint.h>
#include <time.h>
#include <signal.h>
#include <sys/epoll.h>

namespace eventloop {

class EventLoop;
class SignalManager;

class BaseEvent {
  friend class EventLoop;
  friend class SignalManager;
 public:
  static const uint32_t  NONE = 0;
  static const uint32_t  ONESHOT = 1 << 30;
  static const uint32_t  TIMEOUT = 1 << 31;

 public:
  BaseEvent(uint32_t events = 0) { events_ = events; }

  virtual ~BaseEvent() {};

  virtual void OnEvents(uint32_t events) = 0;

 public:
  virtual void SetEvents(uint32_t events) { events_ = events; }
  virtual uint32_t Events() const { return events_; }

 protected:
  uint32_t events_;
};

class BaseFileEvent : public BaseEvent {
  friend class EventLoop;
 public:
  static const uint32_t  READ = 1 << 0;
  static const uint32_t  WRITE = 1 << 1;
  static const uint32_t  ERROR = 1 << 2;

 public:
  explicit BaseFileEvent(uint32_t events = BaseEvent::NONE) : BaseEvent(events) {}
  virtual ~BaseFileEvent() {};

 public:
  void SetFile(int fd) { file = fd; }
  int File() const { return file; }

 public:
  virtual void OnEvents(uint32_t events) = 0;

 protected:
  int file;
};

class BufferFileEvent : public BaseFileEvent {
  friend class EventLoop;
 public:
  explicit BufferFileEvent()
    :BaseFileEvent(BaseFileEvent::READ | BaseFileEvent::ERROR) {
  }
  virtual ~BufferFileEvent() {};

 public:
  void Recive(char *buffer, uint32_t len);
  void Send(char *buffer, uint32_t len);

  virtual void OnRecived(char *buffer, uint32_t len) {};
  virtual void OnSent(char *buffer, uint32_t len) {};
  virtual void OnError() {};

 private:
  void OnEvents(uint32_t events);

 private:
  char *recvbuf_;
  uint32_t torecv_;
  uint32_t recvd_;

  char *sendbuf_;
  uint32_t tosend_;
  uint32_t sent_;

  EventLoop *el_;
};

class BaseSignalEvent : public BaseEvent {
  friend class EventLoop;
 public:
  enum SIGNO {
      UNDEFINED = 0,
      HUP       = SIGHUP,
      INT       = SIGINT,
      QUIT      = SIGQUIT,
      ILL       = SIGILL,
      TRAP      = SIGTRAP,
      ABRT      = SIGABRT,
      BUS       = SIGBUS,
      FPE       = SIGFPE,
      KILL      = SIGKILL,
      USR1      = SIGUSR1,
      SEGV      = SIGSEGV,
      USR2      = SIGUSR2,
      PIPE      = SIGPIPE,
      ALRM      = SIGALRM,
      TERM      = SIGTERM,
      STKFLT    = SIGSTKFLT,
      CHLD      = SIGCHLD,
      CONT      = SIGCONT,
      STOP      = SIGSTOP,
      TSTP      = SIGTSTP,
      TTIN      = SIGTTIN,
      TTOU      = SIGTTOU,
      URG       = SIGURG,
      XCPU      = SIGXCPU,
      XFSZ      = SIGXFSZ,
      VTALRM    = SIGVTALRM,
      PROF      = SIGPROF,
      WINCH     = SIGWINCH,
      IO        = SIGIO,
      PWR       = SIGPWR,
      SYS       = SIGSYS,
  };

 public:
  explicit BaseSignalEvent(uint32_t events = BaseEvent::NONE) : BaseEvent(events) {}
  virtual ~BaseSignalEvent() {};

 public:
  void SetSignal(SIGNO sig_no) { sig_no_ = sig_no; }
  SIGNO Signal() const { return sig_no_; }

 protected:
  SIGNO sig_no_;
};

class BaseTimerEvent : public BaseEvent {
  friend class EventLoop;
 public:
  static const uint32_t TIMER = 1 << 0;

 public:
  explicit BaseTimerEvent(uint32_t events = BaseEvent::NONE) : BaseEvent(events) {}
  virtual ~BaseTimerEvent() {};

 public:
  void SetTime(timeval tv) { time_ = tv; }
  timeval Time() const { return time_; }

 protected:
  timeval time_;
};

class PeriodicTimerEvent : public BaseTimerEvent {
  friend class EventLoop;
 public:
  explicit PeriodicTimerEvent() :BaseTimerEvent(BaseEvent::NONE), el_(NULL) {};
  explicit PeriodicTimerEvent(timeval inter) :BaseTimerEvent(BaseEvent::NONE), interval_(inter), el_(NULL) {};
  virtual ~PeriodicTimerEvent() {};

  void SetInterval(timeval inter) { interval_ = inter; }

  void Start();
  void Stop();

  bool IsRunning() { return running_; }

  virtual void OnTimer() = 0;

 private:
  void OnEvents(uint32_t events);

 private:
  timeval interval_;
  bool running_;

  EventLoop *el_;
};

class EventLoop {
 public:
  EventLoop();
  ~EventLoop();

 public:
  // add delete & update event objects
  int AddEvent(BaseFileEvent *e);
  int DeleteEvent(BaseFileEvent *e);
  int UpdateEvent(BaseFileEvent *e);

  int AddEvent(BaseTimerEvent *e);
  int DeleteEvent(BaseTimerEvent *e);
  int UpdateEvent(BaseTimerEvent *e);

  int AddEvent(BaseSignalEvent *e);
  int DeleteEvent(BaseSignalEvent *e);
  int UpdateEvent(BaseSignalEvent *e);

  int AddEvent(BufferFileEvent *e);
  int AddEvent(PeriodicTimerEvent *e);

  // do epoll_waite and collect events
  int ProcessEvents(int timeout);

  // event loop control
  void StartLoop();
  void StopLoop();

  timeval Now() const { return now_; }

 private:
  int CollectFileEvents(int timeout);
  int DoTimeout();

 private:
  int epfd_;
  epoll_event evs_[256];

  timeval now_;
  bool stop_;

  void *timermanager_;
};

int SetNonblocking(int fd);

int BindTo(const char *host, short port);

int ConnectTo(const char *host, short port, bool async);

}

#endif // EVENT_LOOP_H_
