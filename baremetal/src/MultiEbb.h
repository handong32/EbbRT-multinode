#ifndef MULTI_H_
#define MULTI_H_

#include <ebbrt/LocalIdMap.h>
#include <ebbrt/GlobalIdMap.h>
#include <ebbrt/EbbRef.h>
#include <ebbrt/SharedEbb.h>
#include <ebbrt/Message.h>
#include <ebbrt/UniqueIOBuf.h>
#include <ebbrt/EbbRef.h>
#include <ebbrt/IOBuf.h>
#include <ebbrt/Messenger.h>
#include <ebbrt/SpinBarrier.h>

using namespace ebbrt;

class MultiEbb : public ebbrt::Messagable<MultiEbb> {

  EbbId ebbid;

  // this is used to save and load context
  ebbrt::EventManager::EventContext *emec{ nullptr };

  // a void promise where we want to start some computation after some other
  // function has finished
  ebbrt::Promise<void> mypromise;

public:
  explicit MultiEbb(ebbrt::Messenger::NetworkId nid, EbbId _ebbid)
      : Messagable<MultiEbb>(_ebbid), remote_nid_(std::move(nid)) {
    ebbid = _ebbid;
  }

  static MultiEbb &HandleFault(ebbrt::EbbId id);

  // this uses the void promise to return a future that will
  // get invoked when mypromose gets set with some value
  ebbrt::Future<void> waitReceive() { return std::move(mypromise.GetFuture()); }

  void Send(const char *str);

  void ReceiveMessage(ebbrt::Messenger::NetworkId nid,
                      std::unique_ptr<ebbrt::IOBuf> &&buffer);

  EbbId getEbbId() { return ebbid; }

  // void runJob(ebbrt::Messenger::NetworkId);

  void destroy() { delete this; }

private:
  ebbrt::Messenger::NetworkId remote_nid_;
};

#endif
