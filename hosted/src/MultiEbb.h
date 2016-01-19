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

class MultiEbb;
typedef EbbRef<MultiEbb> MultiEbbRef;

class MultiEbb : public ebbrt::SharedEbb<MultiEbb>,
                 public ebbrt::Messagable<MultiEbb> {

  EbbId ebbid;

  int counter;
  int numNodes;
  std::vector<ebbrt::Messenger::NetworkId> nids;

  // this is used to save and load context
  ebbrt::EventManager::EventContext *emec{ nullptr };

  // a void promise where we want to start some computation after some other
  // function has finished
  ebbrt::Promise<void> mypromise;
  ebbrt::Promise<void> nodesinit;

public:
  MultiEbb(EbbId _ebbid, int _numNodes) : Messagable<MultiEbb>(_ebbid) {
    ebbid = _ebbid;
    numNodes = _numNodes;
    counter = 0;
    nids.clear();
  }

  static ebbrt::Future<MultiEbbRef> Create(int numNodes) {
    auto id = ebbrt::ebb_allocator->Allocate();
    auto ebbref = SharedEbb<MultiEbb>::Create(new MultiEbb(id, numNodes), id);

    // returns a future for the EbbRef
    return ebbrt::global_id_map
        ->Set(id, ebbrt::messenger->LocalNetworkId().ToBytes())
        // the Future<void> f is returned by the ebbrt::global_id_map
        .Then([ebbref](ebbrt::Future<void> f) {
          // f.Get() ensures the gobal_id_map has completed
          f.Get();
          return ebbref;
        });
  }

  // this uses the void promise to return a future that will
  // get invoked when mypromose gets set with some value
  ebbrt::Future<void> waitReceive() { return std::move(mypromise.GetFuture()); }

  // FIXME
  ebbrt::Future<void> waitNodes() { return std::move(nodesinit.GetFuture()); }

  void addNid(ebbrt::Messenger::NetworkId nid) {
    nids.push_back(nid);
    if ((int)nids.size() == numNodes) {
      nodesinit.SetValue();
    }
  }

  void Send(ebbrt::Messenger::NetworkId nid, const char *str);

  void ReceiveMessage(ebbrt::Messenger::NetworkId nid,
                      std::unique_ptr<ebbrt::IOBuf> &&buffer);

  EbbId getEbbId() { return ebbid; }

  void runJob();

  void destroy() { delete this; }
};

#endif
