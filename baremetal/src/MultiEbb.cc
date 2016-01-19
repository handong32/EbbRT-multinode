#include "MultiEbb.h"

EBBRT_PUBLISH_TYPE(, MultiEbb);

using namespace ebbrt;

void AppMain() {}

void MultiEbb::Send(const char *str) {
  auto len = strlen(str) + 1;
  auto buf = ebbrt::MakeUniqueIOBuf(len);
  snprintf(reinterpret_cast<char *>(buf->MutData()), len, "%s", str);

  SendMessage(remote_nid_, std::move(buf));
}

static size_t indexToCPU(size_t i) { return i; }

void MultiEbb::ReceiveMessage(ebbrt::Messenger::NetworkId nid,
                              std::unique_ptr<ebbrt::IOBuf> &&buffer) {
  auto output = std::string(reinterpret_cast<const char *>(buffer->Data()));
  ebbrt::kprintf("Received msg: %s\n", output.c_str());

  // get number of cores/cpus on backend
  size_t ncpus = ebbrt::Cpu::Count();
  // get my cpu id
  size_t theCpu = ebbrt::Cpu::GetMine();

  // create a spin barrier on all cpus
  static ebbrt::SpinBarrier bar(ncpus);

  // gets current context
  ebbrt::EventManager::EventContext context;

  // atomic type with value 0
  std::atomic<size_t> count(0);

  for (size_t i = 0; i < ncpus; i++) {
    // spawn jobs on each core using SpawnRemote
    ebbrt::event_manager->SpawnRemote(
        [theCpu, ncpus, &count, &context]() {
          // get my cpu id
          size_t mycpu = ebbrt::Cpu::GetMine();
          ebbrt::kprintf("theCpu: %d, mycpu: %d\n", theCpu, mycpu);

          // atomically increment count
          count++;

          // barrier here ensures all cores run until this point
          bar.Wait();

          // basically wait until all cores reach this point
          while (count < (size_t)ncpus);

          // the cpu that initiated the SpawnRemote has the SaveContext
          if (ebbrt::Cpu::GetMine() == theCpu) {
            // activate context will return computation to instruction
            // after SaveContext below
            ebbrt::event_manager->ActivateContext(std::move(context));
          }
        },
        indexToCPU(
            i)); // if i don't add indexToCPU, one of the cores never run??
  }

  ebbrt::event_manager->SaveContext(context);

  ebbrt::kprintf("Context restored...\n");

  std::string t = "abc";
  Send(t.c_str());
}

MultiEbb &MultiEbb::HandleFault(ebbrt::EbbId id) {
  {
    ebbrt::LocalIdMap::ConstAccessor accessor;
    auto found = ebbrt::local_id_map->Find(accessor, id);
    if (found) {
      auto &pr = *boost::any_cast<MultiEbb *>(accessor->second);
      ebbrt::EbbRef<MultiEbb>::CacheRef(id, pr);
      return pr;
    }
  }

  ebbrt::EventManager::EventContext context;
  auto f = ebbrt::global_id_map->Get(id);
  MultiEbb *p;
  f.Then([&f, &context, &p, id](ebbrt::Future<std::string> inner) {
    p = new MultiEbb(ebbrt::Messenger::NetworkId(inner.Get()), id);
    ebbrt::event_manager->ActivateContext(std::move(context));
  });
  ebbrt::event_manager->SaveContext(context);
  auto inserted = ebbrt::local_id_map->Insert(std::make_pair(id, p));
  if (inserted) {
    ebbrt::EbbRef<MultiEbb>::CacheRef(id, *p);
    return *p;
  }

  delete p;
  // retry reading
  ebbrt::LocalIdMap::ConstAccessor accessor;
  ebbrt::local_id_map->Find(accessor, id);
  auto &pr = *boost::any_cast<MultiEbb *>(accessor->second);
  ebbrt::EbbRef<MultiEbb>::CacheRef(id, pr);
  return pr;
}
