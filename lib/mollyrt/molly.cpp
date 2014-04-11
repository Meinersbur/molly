/**
 * Molly Runtime Library
 *
 * Do not use __builtin_molly_* in here, they won't work. In fact, most builtins from the user application will be transformed to to calls to this runtime library
 */
#define __MOLLYRT
#include "molly.h"
 
#include <malloc.h>
#include <cstdio>
#include <mpi.h> 
#include <assert.h>
#include <vector>
 
#include <iostream>

#ifdef _WIN32
#include <process.h>
#include <windows.h>
#else
#include <unistd.h>
#endif /* _WIN32 */

using namespace molly;


#pragma region Older stuff
#define MOLLY_COMMUNICATOR_MPI

extern "C" void test_mollyrt() {}

 //class MPICommunicator;

namespace molly {
 
  class MemcpyCommunicator;



  class CommunicatorCommon {
  private:
    // Static classes, do not instantiate
      ~CommunicatorCommon() = delete;
      CommunicatorCommon() = delete;
  public:
  };

#ifdef MOLLY_COMMUNICATOR_MPI
 // typedef MPICommunicator TheCommunicator;
#endif

#ifdef MOLLY_COMMUNICATOR_SPI
#endif

#ifdef MOLLY_COMMUNICATOR_SINGLE
#endif

#ifdef MOLLY_COMMUNICATOR_MEMCPY
  typedef MemcpyCommunicator TheCommunicator;
#endif
  //}

#define ERROREXIT(...) \
  do { \
  fprintf(stderr, "\n" __VA_ARGS__); \
  abort(); \
  } while (0)



#define MPI_CHECK(CALL)                                         \
  do {                                                          \
    MOLLY_DEBUG(#CALL);                                         \
    auto retval = (CALL);                                       \
    if (retval!=MPI_SUCCESS) {                                  \
      ERROREXIT("MPI fail: %s\nReturned: %d\n", #CALL, retval); \
    }                                                           \
  } while (0)

#define RTASSERT(ASSUMPTION, ...) \
  do { \
    if (!(ASSUMPTION)) { \
      fprintf(stderr, "\nAssertion  fail: %s", #ASSUMPTION); \
      ERROREXIT(__VA_ARGS__); \
    } \
  } while (0)



  void foo() {
    ERROREXIT("X");
    RTASSERT(true, "Y");
  }


  void broadcast_send(const void *sendbuf, size_t size) { MOLLY_DEBUG_FUNCTION_SCOPE
   //TheCommunicator::broadcast_send(sendbuf, size);
  }

  void broadcast_recv(void *recvbuf, size_t size, rank_t sender) { MOLLY_DEBUG_FUNCTION_SCOPE
    //TheCommunicator::broadcast_recv(recvbuf, size, sender);
  }


} // namespace molly 


extern "C" void waitToAttach() {
#ifndef NDEBUG
  static int i = 0;
  if (i)
    return; // Already attached
  auto rank = __molly_cluster_mympirank();
  if (rank!=0)
    return;

  std::ostringstream os; // Buffer to avoid this is scattered in output between other rank output  
  os << '\n' << rank << ") PID " << getpid() << " is waiting until you change i to something nonzero\n\n";
  std::cerr << os.str() << std::flush;
  while (!i) {
#ifdef _WIN32
    Sleep(2000);
#else
    sleep(2);
#endif
    std::cerr << '.';
  }
#endif /* NDEBUG */
}






static class MollyInit {
public:
  MollyInit() {
    //communicator = new TheCommunicator();
    // This will be called multiple times
    //int x = _cart_lengths[0]; // To avoid that early optimizers throw it away 
    //int y = _cart_local_coord[0];
  }
  ~MollyInit() {
    //delete communicator;
  }
} molly_global;
#pragma endregion






#pragma region MPI Communicator
namespace {

  void DebugWait(int rank) {
//#ifndef NDEBUG
    char a;

    if(rank == 0) {
      printf("Rank %d is waiting for signal...\n", rank);
        //std::cout << "Rank " << rank << " is waiting for signal..." << std::endl;
      scanf("%c", &a);
      printf("%d: Starting now\n", rank);
    } 

    MPI_Bcast(&a, 1, MPI_BYTE, 0, MPI_COMM_WORLD);
    printf("%d: Starting now\n", rank);
//#endif
}


  
  class MPICommunicator {
    friend class MPISendCommunication;
    friend class MPISendCommunicationBuffer;
    friend class MPIRecvCommunication;
    friend class MPIRecvCommunicationBuffer;

    // From init
    bool initialized;

    int nClusterDims;
    uint64_t nRanks;
    int *shape;
    int *periods;

    /// From MPI_Init_thread
    int providedThreadLevel;
    
    /// From MPI_Comm_size
   int _world_ranks;

   /// From MPI_Comm_rank
   int _world_self;

   /// From MPI_Cart_create
   MPI_Comm _cart_comm;

   /// From MPI_Cart_coords
   int _cart_self;

   /// From MPI_Cart_coords
   int *_cart_self_coords;

  protected:
    void barrier() {
      MPI_CHECK(MPI_Barrier(MPI_COMM_WORLD));
    }

    int getMPICommRank(uint64_t nClusterDims, uint64_t *coords) { // MOLLY_DEBUG_METHOD_ARGS(nClusterDims, coords)
      assert(this->nClusterDims == nClusterDims);

      //TODO: This is a waste; avoid using alloca? Use int right away?
      std::vector<int> intCoords;
      intCoords.resize(nClusterDims);
      for (auto i=nClusterDims-nClusterDims;i<nClusterDims;i+=1) {
        intCoords[i] = coords[i];
      }

      int rank;
      auto coordsData = intCoords.data();
      PMPI_Cart_rank(_cart_comm, coordsData, &rank);
      return rank;
    }

  public:
    MPICommunicator() : initialized(false), _world_self(-1) { MOLLY_DEBUG_FUNCTION_SCOPE }
    ~MPICommunicator() { MOLLY_DEBUG_FUNCTION_SCOPE
      if (!initialized)
        return;

      MPI_CHECK(MPI_Finalize());
      free(_cart_self_coords);
      _cart_self_coords = NULL;
    }


    bool isInitialized() {
      return this && this->initialized;
    }


    void init(uint64_t nClusterDims, uint64_t *clusterShape, bool *clusterPeriodic, int &argc, char **(&argv)) {
      this->nClusterDims = nClusterDims;
      this->nRanks = 1;
      this->shape = new int[nClusterDims];
      this->periods = new int[nClusterDims];
      for (auto i = nClusterDims - nClusterDims; i < nClusterDims; i += 1) {
        this->nRanks *= clusterShape[i];
        this->shape[i] = clusterShape[i];
        this->periods[i] = clusterPeriodic[i];
      }

      // MPI_Init_thread may look for arguments intended to configure MPI, and then remove these args to avoid processing by the user program
      MPI_CHECK(MPI_Init_thread(&argc, &argv, MPI_THREAD_SINGLE/*TODO: Support OpenMP*/, &providedThreadLevel));
      MPI_CHECK(MPI_Comm_rank(MPI_COMM_WORLD, &_world_self));
      //DebugWait(_world_self);
      MPI_CHECK(MPI_Comm_size(MPI_COMM_WORLD, &_world_ranks));
      //std::cout << _world_ranks << std::endl;
      MOLLY_VAR(_world_ranks, nRanks);
      if (__molly_cluster_mympirank() == PRINTRANK) {
        std::cerr << "Expected ranks = " << nRanks << "\n";
        std::cerr << "MPI ranks      = " << _world_ranks << "\n";
      }

      RTASSERT(_world_ranks == nRanks, "Have to mpirun with exact shape that was used when compiling");

#ifndef NDEBUG
      barrier(); // Wait for all ranks to avoid mixing messages with non-master ranks
#endif
      if (__molly_cluster_mympirank() == PRINTRANK) {
        std::cerr << "Geometry = (";
        for (auto i = 0; i < nClusterDims; i += 1) {
          if (i != 0)
            std::cerr << ",";
          std::cerr << clusterShape[i];
        }
        std::cerr << ")\n";
        std::cerr << "###############################################################################\n";
      }


      MOLLY_DEBUG("__molly_cluster_dims=" << nClusterDims << " __molly_cluster_size=" << nRanks);
      for (auto d = nClusterDims - nClusterDims; d < nClusterDims; d += 1) {
        MOLLY_DEBUG("d=" << d << " __molly_cluster_lengths[d]=" << clusterShape[d] << " __molly_cluster_periodic[d]=" << clusterPeriodic[d]);
      }

      MPI_CHECK(MPI_Cart_create(MPI_COMM_WORLD, nClusterDims, shape, periods, true, &_cart_comm));
      MPI_CHECK(MPI_Comm_rank(_cart_comm, &_cart_self));
      MOLLY_DEBUG("_cart_self=" << _cart_self);

      _cart_self_coords = new int[nClusterDims];
      MPI_CHECK(MPI_Cart_coords(_cart_comm, _cart_self, nClusterDims, _cart_self_coords));

      this->initialized = true;

#ifndef NDEBUG
      std::cerr << std::flush; std::cout << std::flush;  barrier(); std::string cartcoord;
      std::stringstream os;
      for (auto d = nClusterDims - nClusterDims; d < nClusterDims; d += 1) {
        if (d != 0)
          os << ",";
        os << _cart_self_coords[d];
      }
      auto tmp = _world_self;
      //_world_self = 0;
      MOLLY_DEBUG("Here is rank " << _world_self << " at cart coord (" << os.rdbuf() << ") running on PID " << getpid());
      //_world_self = tmp;

      barrier();

      //waitToAttach();
#endif /* NDEBUG */
    }

    int getNumDimensions() {
      assert(initialized);
      return nClusterDims;
    }

    int getDimLength(int d) {
      assert(initialized);
      assert(shape);
      assert(0 <= d && d < nClusterDims);
      return shape[d];
    }

    int getSelfCoordinate(int d) {
      assert(initialized);
      assert(_cart_self_coords);
      assert(0 <= d && d < nClusterDims);
      return _cart_self_coords[d];
    }

    bool isMaster() {
      return _world_self==0;
    }


    int getMPIMyRank() {
      return _world_self;
    }

  }; // class MPICommunicator

} // namespace

MPICommunicator *communicator;

#pragma endregion


#pragma region SendCommunicationBuffer
namespace {
   int get_MPI_count(MPI_Status *status) {
  int count;
  MPI_CHECK(MPI_Get_count(status, MPI_BYTE, &count));
  return count;
   }
  
  
  class MPISendCommunication;

  class MPISendCommunicationBuffer {
  private:
    bool initialized;

    MPISendCommunication *parent;
    size_t elts;
    size_t eltSize;
    uint64_t dst;
    uint64_t tag;

    void *buf;
    bool sending;
    MPI_Request request;

#ifndef NDEBUG
    std::vector<int> dstCoords;
#endif

  protected:
    bool isInitialized() {
      return buf!=nullptr;
    }

    void dump() {
#ifndef NDEBUG
  std::ostringstream os;
  os << "(";
  for (auto i = 0; i < dstCoords.size(); i+=1) {
    if (i!=0)
      os << ", ";
    os << dstCoords[i];
  }
  os << ")";
  std::string dstCoordinates = os.str();
  MOLLY_VAR(tag, dst, dstCoordinates, elts, sending);
#endif
    }

  public:
    ~MPISendCommunicationBuffer() { MOLLY_DEBUG_FUNCTION_SCOPE
      if (buf) {
        MPI_CHECK(MPI_Request_free(&request));
        free(buf); 
        buf = nullptr;
      }
    }

    MPISendCommunicationBuffer() : initialized(false), parent(nullptr), buf(nullptr) { //MOLLY_DEBUG_FUNCTION_SCOPE
    }

    void init(MPISendCommunication *parent, size_t elts, size_t eltSize, uint64_t dst, uint64_t nClusterDims, uint64_t *dstCoords, uint64_t tag) { MOLLY_DEBUG_METHOD_ARGS(parent, elts, eltSize, dst, nClusterDims, dstCoords, tag)
      assert(!initialized && "No double-initialization");
      assert(parent);
      assert(elts>=1);
      assert(eltSize>=1);
      assert(dstCoords);

      this->parent = parent;
      this->elts = elts;
      this->eltSize = eltSize;
      this->dst = dst;
      this->tag = tag;
      this->buf = malloc(elts * eltSize);

#ifndef NDEBUG
      this->dstCoords.resize(nClusterDims);
      for (auto i = nClusterDims-nClusterDims;i<nClusterDims;i+=1) {
        this->dstCoords[i] = dstCoords[i];
      }
#endif

      this->sending = false;
      auto dstMpiRank = communicator->getMPICommRank(nClusterDims, dstCoords);
      MPI_CHECK(MPI_Send_init(buf, elts*eltSize, MPI_BYTE, dstMpiRank, tag, communicator->_cart_comm, &request));// MPI_Rsend_init ???

      this->initialized = true;
      dump();
    }

    void *getDataPtr() { // MOLLY_DEBUG_FUNCTION_SCOPE
      dump();
      assert(buf);
      return buf;
    }

    void send() { MOLLY_DEBUG_FUNCTION_SCOPE
      dump();

      assert(!sending);
      MPI_CHECK(MPI_Start(&request));
      this->sending = true;
    }

    void wait() { // MOLLY_DEBUG_FUNCTION_SCOPE
      dump();

      if (!sending)
  return; // May happen before the very first send; Molly always waits before sending the next chunk
      
      MPI_Status status;
      MPI_CHECK(MPI_Wait(&request, &status));
      this->sending = false;

#ifndef NDEBUG
      auto count = get_MPI_count(&status);
      //MOLLY_VAR(count,sending,status.count,status.cancelled,status.MPI_SOURCE,status.MPI_TAG,status.MPI_ERROR);
      MOLLY_VAR(count,sending);
      //assert(count > 0 && "must receive something");
#endif
    }
  }; // class MPISendCommunicationBuffer


class MPISendCommunication {
private:
  uint64_t dstCount;
  size_t eltSize;
 MPISendCommunicationBuffer *dstBufs;

protected:
  MPISendCommunicationBuffer *getBuffer(uint64_t dst) {
    assert(0 <= dst && dst < dstCount);
    assert(0 <= dst && dst < communicator->_world_ranks);
    assert(dstBufs);
    return &dstBufs[dst];
  }

public:
  ~MPISendCommunication() {
    delete[] dstBufs;
  }

  MPISendCommunication(uint64_t dstCount, uint64_t eltSize) : dstCount(dstCount), eltSize(eltSize) { //MOLLY_DEBUG_METHOD_ARGS(dstCount, eltSize)
    // FIXME: Currently nodes are indexed at a global scale, not in the range [0..dstCount)
    //dstCount = communicator->_world_ranks; 
    dstBufs = new MPISendCommunicationBuffer[dstCount];
  }

  void initDst(uint64_t dst, uint64_t nClusterDims, uint64_t *dstCoords, uint64_t count, uint64_t tag) { // MOLLY_DEBUG_METHOD_ARGS(dst, nClusterDims, dstCoords, count, tag)
    getBuffer(dst)->init(this, count, eltSize, dst, nClusterDims, dstCoords, tag);
  }

  void *getDataPtr(uint64_t dst) { // MOLLY_DEBUG_METHOD_ARGS(dst)
    return getBuffer(dst)->getDataPtr();
  }

  void send(uint64_t dst) { MOLLY_DEBUG_METHOD_ARGS(dst)
    return getBuffer(dst)->send();
  }

  void wait(uint64_t dst) { // MOLLY_DEBUG_METHOD_ARGS(dst)
    return getBuffer(dst)->wait();
  }

}; // class MPISendCommunication
} // namespace
#pragma enregion


#pragma region SendCommunicationBuffer
namespace {
  class MPIRecvCommunication;

  class MPIRecvCommunicationBuffer {
  private:
    MPIRecvCommunication *parent;
    size_t elts;
    size_t eltSize;
    uint64_t src;
    uint64_t tag;

    void *buf;
        MPI_Request request;

#ifndef NDEBUG
        std::vector<int> srcCoords;
#endif

  protected:
    void dump() {
#ifndef NDEBUG
  std::ostringstream os;
  os << "(";
  for (auto i = 0; i < srcCoords.size(); i+=1) {
    if (i!=0)
      os << ", ";
    os << srcCoords[i];
  }
  os << ")";
  std::string srcCoordinates = os.str();
  MOLLY_VAR(tag, src, srcCoordinates, elts);
#endif /* NDEBUG */
    }
  
  public:
    ~MPIRecvCommunicationBuffer() {
      if (buf) {
       MPI_CHECK( MPI_Request_free(&request));
      free(buf);
      buf = nullptr;
      }
    }

    MPIRecvCommunicationBuffer() : parent(nullptr), buf(nullptr) {
    }

    void init(MPIRecvCommunication *parent, size_t elts, size_t eltSize, uint64_t src, uint64_t nClusterDims, uint64_t *srcCoords, uint64_t tag) { MOLLY_DEBUG_METHOD_ARGS(parent, elts, eltSize, src, nClusterDims, srcCoords, tag)
      this->parent=parent;
      this->elts=elts;
      this->eltSize=eltSize;
      this->src = src;
      this->tag = tag;
      this->buf = malloc(elts * eltSize);

#ifndef NDEBUG
      this->srcCoords.resize(nClusterDims);
      for (auto i = nClusterDims-nClusterDims;i<nClusterDims;i+=1) {
        this->srcCoords[i] = srcCoords[i];
      }
#endif

      auto dstMpiRank = communicator->getMPICommRank(nClusterDims, srcCoords);
      MPI_CHECK(MPI_Recv_init(buf, elts*eltSize, MPI_BYTE, dstMpiRank, tag, communicator->_cart_comm, &request));// MPI_Rrecv_init ???
    
      // Get to ready state immediately
      MPI_CHECK(MPI_Start(&request));
      dump();
    }

    void *getDataPtr() { MOLLY_DEBUG_FUNCTION_SCOPE
      dump();
      
      assert(buf);
      return buf;
    }

    void recv() { MOLLY_DEBUG_FUNCTION_SCOPE
      dump();
      
      MPI_CHECK(MPI_Start(&request));
    }

    void wait() { MOLLY_DEBUG_FUNCTION_SCOPE
      dump();
      
      MPI_Status status;
      MPI_CHECK(MPI_Wait(&request, &status));

#ifndef NDEBUG
      int count = -1;
      MPI_Get_count(&status, MPI_BYTE, &count);
      assert(count > 0 && "Nothing received");
#endif
    }

  }; // class MPISendCommunicationBuffer

  class MPIRecvCommunication {
  private:
    uint64_t srcCount;
    size_t eltSize;
    MPIRecvCommunicationBuffer *srcBufs;

  protected:
    MPIRecvCommunicationBuffer *getBuffer(uint64_t src) { // MOLLY_DEBUG_METHOD_ARGS(src)
      assert(0 <= src && src < srcCount);
      assert(0 <= src && src < communicator->_world_ranks);
      assert(srcBufs);
      return &srcBufs[src];
    }

  public:
    ~MPIRecvCommunication() { MOLLY_DEBUG_FUNCTION_SCOPE
      delete[] srcBufs;
    }

    MPIRecvCommunication(uint64_t srcCount, uint64_t eltSize) : srcCount(srcCount), eltSize(eltSize) { // MOLLY_DEBUG_FUNCTION_SCOPE
      // FIXME: Currently nodes are indexed at a global scale, not in the range [0..srcCount)
      //srcCount = communicator->_world_ranks;
      srcBufs = new MPIRecvCommunicationBuffer[srcCount];
    }

    void initSrc(uint64_t src, uint64_t nClusterDims, uint64_t *srcCoords, uint64_t count, uint64_t tag) { // MOLLY_DEBUG_METHOD_ARGS(src, nClusterDims, srcCoords, count, tag)
      getBuffer(src)->init(this, count, eltSize, src, nClusterDims, srcCoords, tag);
    }

    void *getDataPtr(uint64_t dst) { MOLLY_DEBUG_FUNCTION_SCOPE
      return getBuffer(dst)->getDataPtr();
    }

    void recv(uint64_t src) { MOLLY_DEBUG_FUNCTION_SCOPE
      return getBuffer(src)->recv();
    }

    void wait(uint64_t dst) { MOLLY_DEBUG_FUNCTION_SCOPE
      return getBuffer(dst)->wait();
    }

  }; // class MPISendCommunication
} // namespace
#pragma endregion


// If running with molly optimization, the original main method will be renamed to this one
// This is a weak symbol because we want it compilable without mollycc, when main is not renamed to __molly_orig_main
//extern "C" int __molly_orig_main(int argc, char *argv[]) __attribute__((weak));

// ... and a new main generated, that just calls __molly_main
//extern "C" int main(int argc, char *argv[]);


extern "C" void __molly_generated_init();
extern "C" int __molly_orig_main(int argc, char *argv[], char *envp[]);
extern "C" void __molly_generated_release();



#pragma region Molly generates calls to these

/// Molly makes the runtime call this instead of the application's main function
extern "C" int __molly_main(int argc, char *argv[], char *envp[], uint64_t nClusterDims, uint64_t *clusterShape, bool *clusterPeriodic) {
  MOLLY_DEBUG_FUNCTION_ARGS(argc, argv, nClusterDims, clusterShape, clusterPeriodic)
    assert(&__molly_orig_main && "Must be compiled using mollycc");

#ifndef NDEBUG
  std::cerr << "## __molly_main #############################################################################\n";
#endif 

  //TODO: We could change the communicator dynamically using argc,argv or getenv()
  communicator = new MPICommunicator();
  communicator->init(nClusterDims, clusterShape, clusterPeriodic, argc, argv);
  //Communicator::init(argc, argv);
  //waitToAttach();
  __molly_generated_init();

  // Molly puts combuf and local storage initialization here
  //TODO: Molly could also put these into .ctor
  //__builtin_molly_global_init();

#ifndef NDEBUG
  if (PRINTRANK < 0 || __molly_cluster_mympirank()==PRINTRANK) {
    std::cerr << "###############################################################################\n";
  }
#endif

  //FIXME: Exception-handling, but currently we do not support exceptions
  auto retval = __molly_orig_main(argc, argv, envp);
  //auto retval = __builtin_molly_orig_main(argc, argv, envp);

  // Molly puts combuf and local storage release here
  //TODO: Molly could also put these into .dtor
  //__builtin_molly_global_free();
  __molly_generated_release();

  //Communicator::finalize();
  delete communicator;
  communicator = nullptr;

  return retval;
}


#pragma region Cluster

/// When molly requests what the coordinate of the node this executes is
/// Intrinsic: int_molly_cluster_current_coordinate (deprecated)
/// Intrinsic: int_molly_cluster_pos
extern "C" uint64_t __molly_cluster_current_coordinate(uint64_t d) { //MOLLY_DEBUG_FUNCTION_ARGS(d)
  if (!communicator)
    return 0; // Before initialization
  assert(communicator->isInitialized());

  auto result = communicator->getSelfCoordinate(d);
  MOLLY_DEBUG("CALL  __molly_cluster_current_coordinate(" << d << ") = " << result);
  return result;
}

#pragma endregion
 
  
#pragma region Local Storage

/// Initialize the local part of a field
/// The field's array object is used for this
extern "C" void __molly_local_init(void *localbuf, uint64_t count, void *rankFunc, void *eltFunc) { MOLLY_DEBUG_FUNCTION_ARGS(localbuf, count, rankFunc, eltFunc)
  assert(localbuf);
  assert(count > 0);
  auto ls = static_cast<LocalStore*>(localbuf); // i.e. LocalStore MUST be first base class
  ls->init(count);
}


extern "C" void __molly_local_free(void *localbuf) { MOLLY_DEBUG_FUNCTION_SCOPE
  // Resources freed in destructor
}


extern "C" void *__molly_local_ptr(void *localbuf) { MOLLY_DEBUG_FUNCTION_ARGS(localbuf)
   assert(localbuf);
   auto ls = static_cast<LocalStore*>(localbuf);
   return ls->getDataPtr();
}

#pragma endregion


#pragma region Communication buffer to send data

extern "C" void *__molly_combuf_send_alloc(uint64_t dstCount, uint64_t eltSize, uint64_t tag) { MOLLY_DEBUG_FUNCTION_ARGS(dstCount, eltSize, tag)
  return new MPISendCommunication(dstCount, eltSize);
}


extern "C" void __molly_combuf_send_dst_init(MPISendCommunication *sendbuf, uint64_t dst, uint64_t nClusterDims, uint64_t *dstCoords, uint64_t count, uint64_t tag) { MOLLY_DEBUG_FUNCTION_ARGS(sendbuf, dst, nClusterDims, dstCoords, count, tag)
#ifndef NDEBUG
  std::ostringstream os;
  for (auto i = 0; i < nClusterDims; i+=1) {
    if (i!=0)
      os << ", ";
    os << dstCoords[i];
  }
  MOLLY_DEBUG("Dst coord: (" << os.str() << ")");
#endif
  
  sendbuf->initDst(dst, nClusterDims, dstCoords, count, tag); 
}


extern "C" void __molly_combuf_send_free(MPISendCommunication *sendbuf) { MOLLY_DEBUG_FUNCTION_ARGS(sendbuf)
  delete sendbuf;
}


extern "C" void *__molly_combuf_send_ptr(MPISendCommunication *sendbuf, uint64_t dst) { MOLLY_DEBUG_FUNCTION_ARGS(sendbuf, dst)
  return sendbuf->getDataPtr(dst);
}


extern "C" void __molly_combuf_send(MPISendCommunication *sendbuf, uint64_t dst) { MOLLY_DEBUG_FUNCTION_ARGS(sendbuf, dst)
  sendbuf->send(dst);
}


extern "C" void *__molly_combuf_send_wait(MPISendCommunication *sendbuf, uint64_t dst) { MOLLY_DEBUG_FUNCTION_ARGS(sendbuf, dst)
  sendbuf->wait(dst);
  return sendbuf->getDataPtr(dst);
}

#pragma endregion


#pragma region Communication buffer to recv data

extern "C" void *__molly_combuf_recv_alloc(uint64_t srcCount, uint64_t eltSize, uint64_t tag) { MOLLY_DEBUG_FUNCTION_ARGS(srcCount, eltSize, tag)
  return new MPIRecvCommunication(srcCount, eltSize);
}


extern "C" void __molly_combuf_recv_src_init(MPIRecvCommunication *recvbuf, uint64_t src, uint64_t nClusterDims, uint64_t *srcCoords, uint64_t count, uint64_t tag) { MOLLY_DEBUG_FUNCTION_ARGS(recvbuf, src, nClusterDims, srcCoords, count, tag)
#ifndef NDEBUG
  std::ostringstream os;
  for (auto i = 0; i < nClusterDims; i+=1) {
    if (i!=0)
      os << ", ";
    os << srcCoords[i];
  }
  MOLLY_DEBUG("Src coord: (" << os.str() << ")");
#endif
  recvbuf->initSrc(src, nClusterDims, srcCoords, count, tag);
}


extern "C" void *__molly_combuf_recv_ptr(MPIRecvCommunication *recvbuf, uint64_t src) { MOLLY_DEBUG_FUNCTION_ARGS(recvbuf, src)
  return recvbuf->getDataPtr(src);
}


extern "C" void __molly_combuf_recv(MPIRecvCommunication *recvbuf, uint64_t src) { MOLLY_DEBUG_FUNCTION_ARGS(recvbuf, src)
  recvbuf->recv(src);
}


extern "C" void *__molly_combuf_recv_wait(MPIRecvCommunication *recvbuf, uint64_t src) { MOLLY_DEBUG_FUNCTION_ARGS(recvbuf, src)
  recvbuf->wait(src);
  return recvbuf->getDataPtr(src);
}

#pragma endregion


#pragma region Load and store of single values

/// Intrinsic: molly.value.load
extern "C" void __molly_value_load_disabled(LocalStore *buf, void *val, uint64_t rank, uint64_t idx) { MOLLY_DEBUG_FUNCTION_SCOPE
  assert(!"to implement");
}


/// Intrinsic: molly.value.store
extern "C" void __molly_value_store_disabled(LocalStore *buf, void *val, uint64_t rank, uint64_t idx) { MOLLY_DEBUG_FUNCTION_SCOPE
  assert(!"to implement");
}

#pragma endregion

#pragma endregion


#pragma region Required by molly.h
// FIXME: Do we need the extern "C"?

extern "C" int __molly_cluster_mympirank() {
  if (!communicator)
    return -2;

  return communicator->getMPIMyRank();
}


extern "C" int64_t __molly_cluster_myrank() {
  if (!communicator)
    return -1ll;
  if (!communicator->isInitialized())
    return -2ll;
  
  // This must correspond to the schema in molly::RectangularMapping::codegenIndex
 auto nDims = communicator->getNumDimensions();
 uint64_t result = communicator->getSelfCoordinate(0);
 for (auto d=nDims-nDims+1;d<nDims;d+=1) {
   result = result * communicator->getDimLength(d-1) + communicator->getDimLength(d);
 }
 return result;
}
 
 
extern "C" bool __molly_isMaster() {
  // Before real initialization, any rank must assume they are master otherwise we get no output at all
  if (!communicator)
    return true;

  return communicator->isMaster();
}


extern "C" int64_t __molly_mod(int64_t divident, int64_t divisor) {
  // divident == quotient*divisor + remainder
  auto remainder1 = (divident % divisor) < 0 ? (divident % divisor + divisor) : (divident % divisor);
  auto remainder2 = (divident + divident%divisor) % divisor;
  auto remainder3 = divident % divisor - ((divident % divisor) >> 63) * divisor;
  //auto remainder4 = divident - __molly_floord(divident, divisor) * divisor;

  assert(0 <= remainder1 && remainder1 < llabs(divisor));

  assert(remainder1 == remainder2);
  assert(remainder1 == remainder3);
  //assert(remainder1 == remainder4);

  return remainder2;
}


/// Round to -inf division
extern "C" int64_t __molly_floord(int64_t divident, int64_t divisor) {
  // divident == quotient*divisor + remainder
  auto quotient1 = ((divident < 0) ? (divident - divisor + 1) : divident) / divisor;
  auto quotient2 = (divident / divisor) + ((divident % divisor) >> 63);
  auto quotient3 = (divident - __molly_mod(divident,divisor)) / divisor;

  assert(quotient1 == quotient2);
  assert(quotient1 == quotient3);

  return quotient2;
}


extern "C" void __molly_begin_marker(const char *str) {
  MOLLY_DEBUG("BEGIN " << str);
}
extern "C" void __molly_begin_marker_coord(const char *str, int64_t count, int64_t *vals) {
#ifndef NDEBUG
  if (std::string(str) == std::string("flow local write 'body3_fstore0'source")) {
    assert(count >= 2);
    if (vals[0] == 2 && vals[1] == 0) {
      MOLLY_DEBUG("Got here!!!");
      int a = 0;
    }
  }

  std::ostringstream os;
  for (auto i = count - count; i < count; i += 1) {
    if (i>0)
      os << ",";
    os << vals[i];
  }

  MOLLY_DEBUG("BEGIN " << str << " (" << os.str() << ")");
#endif /* NDEBUG */
}

extern "C" void __molly_end_marker(const char *str) {
  MOLLY_DEBUG("END   " << str);
}
extern "C" void __molly_end_marker_coord(const char *str, int64_t count, int64_t *vals) {
#ifndef NDEBUG
  std::ostringstream os;
  for (auto i = count - count; i < count; i += 1) {
    if (i>0)
      os << ",";
    os << vals[i];
  }

  MOLLY_DEBUG("END   " << str << " (" << os.str() << ")");
#endif /* NDEBUG */
}
extern "C" void __molly_marker_int(const char *str, int64_t val) {
  MOLLY_DEBUG("MARKER " << str << " (" << val << ")");
 //if (__molly_cluster_mympirank() != 0)      
 //  return;                                         
 //std::cerr << __molly_cluster_mympirank() << ")";
 // std::cerr << "MARKER " << str << " (" << val << ")\n";
}

extern "C" void *__molly_combuf_local_alloc(int64_t count, int64_t eltSize) { MOLLY_DEBUG_FUNCTION_ARGS(count, eltSize)
  return malloc(count*eltSize);
}

extern "C" void __moll_combuf_local_free(void *combuf_local) { MOLLY_DEBUG_FUNCTION_ARGS(combuf_local)
  free(combuf_local);
}

extern "C" void *__molly_combuf_local_dataptr(void *combuf_local) { MOLLY_DEBUG_FUNCTION_ARGS(combuf_local)
  assert(combuf_local);
  return combuf_local;
}

#pragma endregion
