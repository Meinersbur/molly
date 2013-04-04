
#include <cstdint>
#include <cstddef>
#include "mollyrt/molly.h"

#include <mpi.h>

int _cart_lengths[1];
int _cart_local_coord[1];
int _rank_local;


namespace molly {
  class Communicator {
  public:
    virtual ~Communicator()  {}


#pragma region Distributed memory setup
    //TODO: Make own class?
    virtual void startDistributed() { }
    virtual void stopDistributed() { }
    //TODO: rank type configurable
    virtual uint32_t getMyRank() {}
    virtual bool isMaster() { return false; }
#pragma endregion


#pragma region Communication buffers
    virtual void *createSendBuffer(size_t length, uint32_t destRank) { }
    virtual void freeSendBuffer(void  *buf) {}
    virtual void sendAll() {}
    virtual void waitSend() {}

    virtual void *createRecvBuffer(size_t length, uint32_t senderRank) {}
    virtual void freeRecvBuffer(void  *buf) {}
    virtual void waitRecv() {}
//#pragma endregion
  }; // class Communicator


  /// Communicator woth only one Node
  class SingleCommunicator : public Communicator {
  public:
    virtual ~SingleCommunicator() {}
  }; // class SingleCommunicator


  /// To simmulate distributed communication with memcpy
 class MemcpyCommunicator {
      public:
    virtual ~MemcpyCommunicator() {}

  }; // class MemcpyCommunicator


  class MPICommunicator {
   public:
    virtual ~MPICommunicator() {}


  }; // class MPICommunicator


  class SPICommunicator {
    public:
    virtual ~SPICommunicator() {}
  }; // class SPICommunicator



} // namespace molly


extern "C" {
}
