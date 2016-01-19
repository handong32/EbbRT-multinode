#include "MultiEbb.h"

EBBRT_PUBLISH_TYPE(, MultiEbb);

void MultiEbb::Send(ebbrt::Messenger::NetworkId nid, const char *str) {
  auto len = strlen(str) + 1;
  auto buf = ebbrt::MakeUniqueIOBuf(len);
  snprintf(reinterpret_cast<char *>(buf->MutData()), len, "%s", str);

  std::cout << "Send(): length of sent iobuf: " << buf->ComputeChainDataLength()
            << " bytes" << std::endl;

  SendMessage(nid, std::move(buf));
}

void MultiEbb::ReceiveMessage(ebbrt::Messenger::NetworkId nid,
                              std::unique_ptr<ebbrt::IOBuf> &&buffer) {
  /***********************************
   * std::string(reinterpret_cast<const char*> buffer->Data(),
   *buffer->Length())
   *
   * Using the above code, I had to first do strcmp(output.c_str(), ...) to
   *ensure
   * it matched the input string.
   * Direct comparison using "==" seems to be working when I don't pass in the
   *Length() as second arg
   **********************************/
  auto output = std::string(reinterpret_cast<const char *>(buffer->Data()));
  std::cout << "Received msg: " << nid.ToString() << ": " << output << "\n";
  counter++;

  if (counter == numNodes) {
    mypromise.SetValue();
  }
}

void MultiEbb::runJob() {
  std::cout << "runJob() " << std::endl;
  std::string ts = "ping";

  for (int i = 0; i < (int)nids.size(); i++) {
    std::cout << "Sending to .. " << nids[i].ToString() << std::endl;
    Send(nids[i], ts.c_str());
  }
}
