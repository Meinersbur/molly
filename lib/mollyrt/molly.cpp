/**
 * Molly Runtime Library
 *
 * Do not use __builtin_molly_* in here, they won't work. In fact, most builtins from the user application will be transformed to to calls to this runtime library
 */
#define __MOLLYRT
#include "molly.h"

#include "mypapi.h"
 
#include <malloc.h>
#include <cstdio>
#include <mpi.h> 
#include <assert.h>
#include <vector>
#include <cinttypes>
 
#include <iostream>

#ifdef _WIN32
#include <process.h>
#include <windows.h>
#else
#include <unistd.h>
#endif /* _WIN32 */

#define BGQ_SPI
#ifdef BGQ_SPI
#include <cmath>
#include <hwi/include/bqc/A2_inlines.h> // GetTimeBase()
#include <l1p/pprefetch.h>
#include <l1p/sprefetch.h>
#endif

#ifndef NDEBUG
#include <sstream>
#endif

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
    
    uint64_t getRankCount() {
      return nRanks;      
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

#ifndef NTRACE
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

#ifndef NTRACE
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
#ifndef NTRACE
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
#ifndef NTRACE
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
  //auto remainder1 = (divident % divisor) < 0 ? (divident % divisor + divisor) : (divident % divisor);
  auto remainder1 = (divident<0) ? (divident % divisor + divisor) : (divident % divisor);
  auto remainder2 = (divisor + divident%divisor) % divisor;
  auto remainder3 = divident % divisor - ((divident % divisor) >> 63) * divisor;
  //auto remainder4 = divident - __molly_floord(divident, divisor) * divisor;

  assert(0 <= remainder1 && remainder1 < llabs(divisor));

  //printf("\nMOD divident=%" PRIi64 " divisor=%" PRIi64 " remainder1=%" PRIi64 " remainder2=%" PRIi64 " remainder3=%" PRIi64 "\n", divident, divisor, remainder1, remainder2, remainder3);
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
#ifndef NTRACE
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
#endif /* NTRACE */
}

extern "C" void __molly_end_marker(const char *str) {
  MOLLY_DEBUG("END   " << str);
}
extern "C" void __molly_end_marker_coord(const char *str, int64_t count, int64_t *vals) {
#ifndef NTRACE
  std::ostringstream os;
  for (auto i = count - count; i < count; i += 1) {
    if (i>0)
      os << ",";
    os << vals[i];
  }

  MOLLY_DEBUG("END   " << str << " (" << os.str() << ")");
#endif /* NTRACE */
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


#define BGQ_SPI 1



static void print_repeat(const char * const str, const int count) {
  auto g_proc_id = __molly_cluster_myrank();
  if (g_proc_id == 0) {
    for (int i = 0; i < count; i += 1) {
      printf("%s", str);
    }
  }
}

static double sqr(double val) {
  return val*val;  
}

#ifndef STRINGIFY
#define STRINGIFY(V) #V
#endif
#ifndef TOSTRING
#define TOSTRING(V) STRINGIFY(V)
#endif

#define lengthof(X) (sizeof(X)/sizeof((X)[0]))
#define CELLWIDTH 15
#define SCELLWIDTH TOSTRING(CELLWIDTH)

#define L1P_CHECK(RTNCODE)                                                                 \
  do {                                                                                   \
      int mpi_rtncode = (RTNCODE);                                                       \
      if (mpi_rtncode != MPI_SUCCESS) {                                                  \
      fprintf(stderr, "L1P call %s at %s:%d failed: errorcode %d\n", #RTNCODE, __FILE__, __LINE__, mpi_rtncode);  \
      assert(!"L1P call " #RTNCODE " failed");                                       \
      abort();                                                                       \
    }                                                                                  \
  } while (0)

static int omp_threads[] = { 1 };
static char *omp_threads_desc[] = { "1" };




#define DEFOPTS (0)
static const int flags[] = {
        DEFOPTS,
  DEFOPTS | hm_noprefetchstream | hm_prefetchimplicitdisable,
  DEFOPTS                       | hm_prefetchimplicitconfirmed,
  DEFOPTS | hm_noprefetchstream | hm_prefetchimplicitconfirmed,
  DEFOPTS | hm_noprefetchstream | hm_prefetchimplicitoptimistic
};
static const char *flags_desc[] = {
    "pf default",
    "pf disable",
    "pf stream",
    "pf confirmed",
    "pf optimistic"
  };

typedef struct {
  int j_max;
  const benchfunc_t *benchfunc;
  bgq_hmflags opts;
  benchstat result;
} master_args;

typedef struct {
  int set;
  mypapi_counters result;
} mypapi_work_t;




static void benchmark_setup_worker(void *argptr, size_t tid, size_t threads) {
  mypapi_init();

#ifndef NDEBUG
  //feenableexcept(FE_DIVBYZERO|FE_INVALID|FE_OVERFLOW|FE_UNDERFLOW);
#endif

#ifdef BGQ_SPI
  const master_args *args = (const master_args *)argptr;
  const int j_max = args->j_max;
  const benchfunc_t &benchfunc = *args->benchfunc;
  const bgq_hmflags opts = args->opts;
  const bool nocom = opts & hm_nocom;
  const bool nooverlap = opts & hm_nooverlap;
  const bool nokamul = opts & hm_nokamul;
  const bool prefetchlist = opts & hm_prefetchlist;
  const bool noprefetchstream = opts & hm_noprefetchstream;
  const bool noprefetchexplicit = opts & hm_noprefetchexplicit;
  const bool noweylsend = opts & hm_noweylsend;
  const bool nobody = opts & hm_nobody;
  const bool nosurface = opts & hm_nosurface;
  const bool experimental = opts & hm_experimental;
  const bgq_hmflags implicitprefetch = (bgq_hmflags)(opts & (hm_prefetchimplicitdisable | hm_prefetchimplicitoptimistic | hm_prefetchimplicitconfirmed));

  L1P_StreamPolicy_t pol;
  switch (implicitprefetch) {
  case hm_prefetchimplicitdisable:
    pol = noprefetchstream ? L1P_stream_disable : L1P_confirmed_or_dcbt/*No option to selectively disable implicit stream*/;
    break;
  case hm_prefetchimplicitoptimistic:
    pol = L1P_stream_optimistic;
    break;
  case hm_prefetchimplicitconfirmed:
    pol = noprefetchstream ? L1P_stream_confirmed : L1P_confirmed_or_dcbt;
    break;
  default:
    // Default setting
    pol = L1P_confirmed_or_dcbt;
    break;
  }

  L1P_CHECK(L1P_SetStreamPolicy(pol));

  //if (g_proc_id==0) {
  //	L1P_StreamPolicy_t poli;
  //	L1P_CHECK(L1P_GetStreamPolicy(&poli));
  //	printf("Prefetch policy: %d\n",poli);
  //}

  // Test whether it persists between parallel sections
  //L1P_StreamPolicy_t getpol = 0;
  //L1P_CHECK(L1P_GetStreamPolicy(&getpol));
  //if (getpol != pol)
  //	fprintf(stderr, "MK StreamPolicy not accepted\n");

  //L1P_CHECK(L1P_SetStreamDepth());
  //L1P_CHECK(L1P_SetStreamTotalDepth());

  //L1P_GetStreamDepth
  //L1P_GetStreamTotalDepth

  // Peter Boyle's setting
  // Note: L1P_CFG_PF_USR_pf_stream_establish_enable is never set in L1P_SetStreamPolicy
  //uint64_t *addr = ((uint64_t*)(Kernel_L1pBaseAddress() + L1P_CFG_PF_USR_ADJUST));
  //*addr |=  L1P_CFG_PF_USR_pf_stream_est_on_dcbt | L1P_CFG_PF_USR_pf_stream_optimistic | L1P_CFG_PF_USR_pf_stream_prefetch_enable | L1P_CFG_PF_USR_pf_stream_establish_enable; // Enable everything???

  if (prefetchlist) {
    L1P_PatternConfigure(1024*1024);
  }
#endif
}


static uint64_t bgq_wcycles() {
  return GetTimeBase();
}

static void donothing(void *arg, size_t tid, size_t threads) {
#ifdef BGQ_SPI
  DelayTimeBase(1600*100);
#endif
}


static void L1Pstart(void *arg_untyped, size_t tid, size_t threads) {
#ifdef BGQ_SPI
  int *record = (int*)arg_untyped;
  L1P_PatternStart(*record);
  L1P_PatternPause();
#endif
}


static void L1Pstop(void *arg_untyped, size_t tid, size_t threads) {
#ifdef BGQ_SPI
  L1P_PatternStop();
#endif
}


static void mypapi_start_worker(void *arg_untyped, size_t tid, size_t threads) {
  mypapi_work_t *arg = (mypapi_work_t *)arg_untyped;
  mypapi_start(arg->set);
}

static void mypapi_stop_worker(void *arg_untyped, size_t tid, size_t threads) {
  mypapi_work_t *arg = (mypapi_work_t *)arg_untyped;
  arg->result = mypapi_stop();
}

static void benchmark_free_worker(void *argptr, size_t tid, size_t threads) {
  master_args *args = (master_args*)argptr;
  bgq_hmflags opts = args->opts;
  bool prefetchlist = opts | hm_prefetchlist;

  mypapi_free();
#ifdef BGQ_SPI
  if (prefetchlist) {
    L1P_PatternUnconfigure();
  }
#endif
}



static uint64_t flopaccumulator = 0;
void __molly_add_flops(uint64_t flops) {
  flopaccumulator += flops;
}

typedef void (*bgq_worker_func)(void *arg, size_t tid, size_t threads);
typedef int (*bgq_master_func)(void *arg);


static int bgq_parallel(bgq_master_func master_func, void *master_arg) {
  assert(master_func);
  return (*master_func)(master_arg);
}


static void bgq_master_call(bgq_worker_func worker_func, void *arg) {
  assert(worker_func);
  (*worker_func)(arg, 0, 1);
}


static void bgq_master_sync() {
}


static int runcheck(bgq_hmflags hmflags) {
  return 0;
}

typedef  int64_t scoord;

static bool g_bgq_dispatch_l1p = false;
static int64_t g_nproc = -1;


static int benchmark_master(void *argptr) {
  master_args * const args = (master_args *) argptr;
  const int j_max = args->j_max;
  const benchfunc_t &benchfunc = *args->benchfunc;
  const bgq_hmflags opts = args->opts;
  const bool nocom = opts & hm_nocom;
  //const bool nooverlap = opts & hm_nooverlap;
  //const bool nokamul = opts & hm_nokamul;
  const bool prefetchlist = opts & hm_prefetchlist;
  //const bool noprefetchstream = opts & hm_noprefetchstream;
  //const bool noprefetchexplicit = opts & hm_noprefetchexplicit;
  //const bool noweylsend = opts & hm_noweylsend;
  //const bool nobody = opts & hm_nobody;
  //const bool nosurface = opts & hm_nosurface;
  //const bool experimental = opts & hm_experimental;
  bool floatprecision = opts & hm_floatprecision;
  //const bgq_hmflags implicitprefetch = opts & (hm_prefetchimplicitdisable | hm_prefetchimplicitoptimistic | hm_prefetchimplicitconfirmed);
  bool withcheck = opts & hm_withcheck;

  // Setup thread options (prefetch setting, performance counters, etc.)
  bgq_master_call(&benchmark_setup_worker, argptr);


  double err = 0;
  if (withcheck) {
    err = runcheck(opts);
  }


  // Give us a fresh environment
  if (nocom) {
    //for (ucoord d = 0; d < PHYSICAL_LD; d+=1) {
      //memset(g_bgq_sec_recv_double[d], 0, bgq_section_size(bgq_direction2section(d, false)));
    //}
  }
  //for (ucoord k = 0; k < k_max; k+=1) {
    //random_spinor_field(g_spinor_field[k], VOLUME/2, 0);
  //}
  const int warmups = 2;

  uint64_t sumotime = 0;
  for (int i = 0; i < 20; i += 1) {
    uint64_t start_time = bgq_wcycles();
    donothing(NULL, 0, 0);
    uint64_t mid_time = bgq_wcycles();
    bgq_master_call(&donothing, NULL);
    uint64_t stop_time = bgq_wcycles();

    uint64_t time = (stop_time - mid_time) - (mid_time- start_time);
    sumotime += time;
  }
  double avgovhtime = (double)sumotime / 20.0;

  static mypapi_work_t mypapi_arg;
  double localsumtime = 0;
  double localsumsqtime = 0;
  uint64_t localsumcycles=0;
  uint64_t localsumflop = 0;
  mypapi_counters counters;
  counters.init = false;
  int iterations = warmups; // Warmup phase
  iterations += j_max;
  if (iterations < warmups + MYPAPI_SETS)
    iterations = warmups + MYPAPI_SETS;

  for (int i = 0; i < iterations; i += 1) {
    //master_print("Starting iteration %d of %d\n", j+1, iterations);
    bool isWarmup = (i < warmups);
    int j = i - warmups;
    bool isPapi = !isWarmup && (i >= iterations - MYPAPI_SETS);
    int papiSet = i - (iterations - MYPAPI_SETS);
    bool isJMax = (0 <= j) && (j < j_max);

    double start_time;
    uint64_t start_cycles;
    uint64_t start_flop;
    if (isJMax) {
      start_flop = flopaccumulator;
    }
    if (isPapi) {
      bgq_master_sync();
      mypapi_arg.set = papiSet;
      bgq_master_call(mypapi_start_worker, &mypapi_arg);
    }
    if (isJMax) {
      start_time = MPI_Wtime();
      start_cycles = bgq_wcycles();
    }

    if (prefetchlist) {
      int record = isWarmup;
      // TODO: Might be done in bgq_dispatch, such no additional call is required
      bgq_master_call(L1Pstart, &record);
      bgq_master_sync();
      // TODO: If L1Pstart and L1Pstop are special-cased, this could be done just around the loop
      g_bgq_dispatch_l1p = true;
    }

    {
      // The main benchmark
      //for (int k = 0; k < k_max; k += 1) {
        // Note that flops computation assumes that readWeyllayout is used
        benchfunc(j, opts);
      //}

      bgq_master_sync(); // Wait for all threads to finish, to get worst thread timing
    }

    if (prefetchlist) {
      bgq_master_sync();
      g_bgq_dispatch_l1p = false;
      bgq_master_call(L1Pstop, NULL);
    }

    double end_time;
    uint64_t end_cycles;
    if (isJMax) {
      end_cycles = bgq_wcycles();
      end_time = MPI_Wtime();
    }
    if (isPapi) {
      bgq_master_call(mypapi_stop_worker, &mypapi_arg);
      counters = mypapi_merge_counters(&counters, &mypapi_arg.result);
    }

    if (isJMax) {
      double duration = end_time - start_time;
      localsumtime += duration;
      localsumsqtime += sqr(duration);
      localsumcycles += (end_cycles - start_cycles);
      localsumflop += (flopaccumulator - start_flop);
    }
  }

  bgq_master_call(&benchmark_free_worker, argptr);

  ucoord its = j_max;
  double localavgtime = localsumtime / its;
  double localavgsqtime = sqr(localavgtime);
  double localrmstime = sqrt((localsumsqtime / its) - localavgsqtime);
  double localcycles = (double)localsumcycles / (double)its;
  double localavgflop = (double)localsumflop / (double)its;

  double localtime[] = { localavgtime, localavgsqtime, localrmstime };
  double sumreduce[3] = { -1, -1, -1 };
  MPI_Allreduce(&localtime, &sumreduce, 3, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  double sumtime = sumreduce[0];
  double sumsqtime = sumreduce[1];
  double sumrmstime = sumreduce[2];

  double avgtime = sumtime / g_nproc;
  double avglocalrms = sumrmstime / g_nproc;
  double rmstime = sqrt((sumsqtime / g_nproc) - sqr(avgtime));

  // Assume k_max lattice site updates, even+odd sites
  //ucoord sites = LOCAL_LT * LOCAL_LX * LOCAL_LY * LOCAL_LZ;
  //ucoord sites_body = PHYSICAL_BODY * PHYSICAL_LK * PHYSICAL_LP;
  //ucoord sites_surface = PHYSICAL_SURFACE * PHYSICAL_LK * PHYSICAL_LP;
  //assert(sites == sites_body+sites_surface);
  //assert(sites == VOLUME);

  //ucoord lup_body = k_max * sites_body;
  //ucoord lup_surface = k_max * sites_surface;
  //ucoord lup = lup_body + lup_surface;
  //assert(lup == k_max * sites);

  benchstat *result = &args->result;
  result->avgtime = avgtime;
  result->localrmstime = avglocalrms;
  result->globalrmstime = rmstime;
  result->totcycles = localcycles;
  result->localavgflop = localavgflop;

  //result->sites_surface = sites_surface;
  //result->sites_body = sites_body;
  //result->sites = sites;
  //result->lup_surface = lup_surface;
  //result->lup_body = lup_body;
  //result->lup = lup;
  //result->flops = flops / avgtime;
  result->error = err;
  result->counters = counters;
  result->opts = opts;
  result->avgovhtime = avgovhtime;
  return EXIT_SUCCESS;
}


static benchstat runbench(const benchfunc_t &benchfunc, bgq_hmflags opts, int j_max, int ompthreads) {
#ifdef OMP
  omp_set_num_threads(ompthreads);
#endif
  master_args args = {
          .j_max = j_max,
          .benchfunc = &benchfunc,
          .opts = opts
  };
  int retcode = bgq_parallel(&benchmark_master, &args);
  assert(retcode == EXIT_SUCCESS);
  if (retcode != EXIT_SUCCESS) {
    exit(retcode);
  }

  return args.result;
}


static void print_stats(benchstat *stats) {
#if PAPI
  int threads = omp_get_num_threads();

  for (mypapi_interpretations j = 0; j < __pi_COUNT; j+=1) {
    printf("%10s|", "");
    char *desc = NULL;

    for (int i3 = 0; i3 < lengthof(flags); i3 += 1) {
      char str[80];
      str[0] = '\0';
      benchstat *stat = &stats[i3];
      bgq_hmflags opts = stat->opts;

      double avgtime = stat->avgtime;
      uint64_t lup = stat->lup;
      uint64_t flop = compute_flop(opts, stat->lup_body, stat->lup_surface);
      double flops = (double)flop/stat->avgtime;
      double localrms = stat->localrmstime / stat->avgtime;
      double globalrms = stat->globalrmstime / stat->avgtime;
      ucoord sites = stat->sites;

      double nCycles = stats[i3].counters.native[PEVT_CYCLES];
      double nCoreCycles = stats[i3].counters.corecycles;
      double nNodeCycles = stats[i3].counters.nodecycles;
      double nInstructions = stats[i3].counters.native[PEVT_INST_ALL];
      double nStores = stats[i3].counters.native[PEVT_LSU_COMMIT_STS];
      double nL1IStalls = stats[i3].counters.native[PEVT_LSU_COMMIT_STS];
      double nL1IBuffEmpty = stats[i3].counters.native[PEVT_IU_IBUFF_EMPTY_CYC];

      double nIS1Stalls = stats[i3].counters.native[PEVT_IU_IS1_STALL_CYC];
      double nIS2Stalls = stats[i3].counters.native[PEVT_IU_IS2_STALL_CYC];

      double nCachableLoads = stats[i3].counters.native[PEVT_LSU_COMMIT_CACHEABLE_LDS];
      double nL1Misses = stats[i3].counters.native[PEVT_LSU_COMMIT_LD_MISSES];
      double nL1Hits = nCachableLoads - nL1Misses;

      double nL1PMisses = stats[i3].counters.native[PEVT_L1P_BAS_MISS];
      double nL1PHits = stats[i3].counters.native[PEVT_L1P_BAS_HIT];
      double nL1PAccesses = nL1PHits + nL1PMisses;

      double nL2Misses = stats[i3].counters.native[PEVT_L2_MISSES];
      double nL2Hits = stats[i3].counters.native[PEVT_L2_HITS];
      double nL2Accesses = nL2Misses + nL2Hits;

      double nDcbtHits = stats[i3].counters.native[PEVT_LSU_COMMIT_DCBT_HITS];
      double nDcbtMisses = stats[i3].counters.native[PEVT_LSU_COMMIT_DCBT_MISSES];
      double nDcbtAccesses = nDcbtHits + nDcbtMisses;

      double nXUInstr = stats[i3].counters.native[PEVT_INST_XU_ALL];
      double nAXUInstr = stats[i3].counters.native[PEVT_INST_QFPU_ALL];
      double nXUAXUInstr = nXUInstr + nAXUInstr;

#if 0
      double nNecessaryInstr = 0;
      if (!(opts & hm_nobody))
      nNecessaryInstr += bodySites * (240/*QFMA*/+ 180/*QMUL+QADD*/+ 180/*LD+ST*/)/2;
      if (!(opts & hm_noweylsend))
      nNecessaryInstr += haloSites * (2*3*2/*QMUL+QADD*/+ 4*3/*LD*/+ 2*3/*ST*/)/2;
      if (!(opts & hm_nosurface))
      nNecessaryInstr += surfaceSites * (240/*QFMA*/+ 180/*QMUL+QADD*/+ 180/*LD+ST*/- 2*3*2/*QMUL+QADD*/- 4*3/*LD*/+ 2*3/*LD*/)/2;
      if (!(opts & hm_nokamul))
      nNecessaryInstr += sites * (8*2*3*1/*QFMA*/+ 8*2*3*1/*QMUL*/)/2;
#endif

      uint64_t nL1PListStarted = stats[i3].counters.native[PEVT_L1P_LIST_STARTED];
      uint64_t nL1PListAbandoned= stats[i3].counters.native[PEVT_L1P_LIST_ABANDON];
      uint64_t nL1PListMismatch= stats[i3].counters.native[PEVT_L1P_LIST_MISMATCH];
      uint64_t nL1PListSkips = stats[i3].counters.native[PEVT_L1P_LIST_SKIP];
      uint64_t nL1PListOverruns = stats[i3].counters.native[PEVT_L1P_LIST_CMP_OVRUN_PREFCH];

      double nL1PLatePrefetchStalls = stats[i3].counters.native[PEVT_L1P_BAS_LU_STALL_LIST_RD_CYC];

      uint64_t nStreamDetectedStreams = stats[i3].counters.native[PEVT_L1P_STRM_STRM_ESTB];
      double nL1PSteamUnusedLines = stats[i3].counters.native[PEVT_L1P_STRM_EVICT_UNUSED];
      double nL1PStreamPartiallyUsedLines = stats[i3].counters.native[PEVT_L1P_STRM_EVICT_PART_USED];
      double nL1PStreamLines = stats[i3].counters.native[PEVT_L1P_STRM_LINE_ESTB];
      double nL1PStreamHits = stats[i3].counters.native[PEVT_L1P_STRM_HIT_LIST];

      double nDdrFetchLine = stats[i3].counters.native[PEVT_L2_FETCH_LINE];
      double nDdrStoreLine = stats[i3].counters.native[PEVT_L2_STORE_LINE];
      double nDdrPrefetch = stats[i3].counters.native[PEVT_L2_PREFETCH];
      double nDdrStorePartial = stats[i3].counters.native[PEVT_L2_STORE_PARTIAL_LINE];

      switch (j) {
      case pi_correct:
        desc = "Max error to reference";
        if (opts & hm_withcheck) {
          snprintf(str, sizeof(str), "%g", stat->error);
        }
        break;
      case pi_ramfetchrate:
        desc = "DDR read";
        snprintf(str, sizeof(str), "%.2f GB/s", 128 * nDdrFetchLine / (avgtime * GIBI));
        break;
      case pi_ramstorerate:
        desc = "DDR write";
        snprintf(str, sizeof(str), "%.2f GB/s", 128 * nDdrStoreLine / (avgtime * GIBI));
        break;
      case pi_ramstorepartial:
        desc = "DDR partial writes";
        snprintf(str, sizeof(str), "%.2f %%",  100 * nDdrStorePartial / (nDdrStorePartial+nDdrStoreLine));
        break;
      case pi_l2prefetch:
        desc = "L2 prefetches";
        snprintf(str, sizeof(str), "%.2f %%",  100 * nDdrPrefetch / nDdrFetchLine);
        break;
      case pi_msecs:
        desc = "Iteration time";
        snprintf(str, sizeof(str), "%.3f mSecs",stat->avgtime/MILLI);
        break;
      case pi_cycpersite:
        desc = "per site update";
        snprintf(str, sizeof(str), "%.1f cyc", stat->totcycles / lup);
        break;
      case pi_instrpersite:
        desc = "instr per update";
        snprintf(str, sizeof(str), "%.1f", nInstructions / lup);
        break;
      case pi_fxupersite:
        desc = "FU instr per update";
        snprintf(str, sizeof(str), "%.1f", nAXUInstr / lup);
        break;
      case pi_flops:
        desc = "MFlop/s";
        snprintf(str, sizeof(str), "%.0f MFlop/s", flops/MEGA);
        break;
      case pi_flopsref:
        desc = "Speed";
        snprintf(str, sizeof(str), "%.0f MFlop/s", stat->localavgflop / (avgtime * MEGA));
        break;
      case pi_floppersite:
        desc = "Flop per site";
        snprintf(str, sizeof(str), "%.1f Flop", stat->localavgflop / sites);
        break;
      case pi_localrms:
        desc = "Thread RMS";
        snprintf(str, sizeof(str), "%.1f %%", 100.0*localrms);
        break;
      case pi_globalrms:
        desc = "Node RMS";
        snprintf(str, sizeof(str), "%.1f %%", 100.0*globalrms);
        break;
      case pi_avgovhtime:
        desc = "Threading overhead";
        snprintf(str, sizeof(str), "%.1f cyc", stat->avgovhtime);
        break;
      case pi_detstreams:
        desc = "Detected streams";
        snprintf(str, sizeof(str), "%llu", nStreamDetectedStreams);
        break;
      case pi_l1pstreamunusedlines:
        desc = "Unused (partially) lines";
        snprintf(str, sizeof(str), "%.2f%% (%.2f%%)", 100.0 * nL1PSteamUnusedLines / nL1PStreamLines, 100.0 * nL1PStreamPartiallyUsedLines / nL1PStreamLines);
        break;
      case pi_l1pstreamhitinl1p:
        desc = "Loads that hit in L1P stream";
        snprintf(str, sizeof(str), "%.2f %%", 100.0 * nL1PStreamHits / nCachableLoads);
        break;
      case pi_cpi:
        desc = "Cycles per instruction (Thread)";
        snprintf(str, sizeof(str), "%.3f cpi", nCycles / nInstructions);
        break;
      case pi_corecpi:
        desc = "Cycles per instruction (Core)";
        snprintf(str, sizeof(str), "%.3f cpi", nCoreCycles / nInstructions);
        break;
      case pi_l1istalls:
        desc = "Empty instr buffer";
        snprintf(str, sizeof(str), "%.2f %%", nL1IBuffEmpty / nCycles);
        break;
      case pi_is1stalls:
        desc = "IS1 Stalls (dependency)";
        snprintf(str, sizeof(str), "%.2f %%", 100 * nIS1Stalls / nCycles);
        break;
      case pi_is2stalls:
        desc = "IS2 Stalls (func unit)";
        snprintf(str, sizeof(str), "%.2f %%", 100 * nIS2Stalls / nCycles);
        break;
      case pi_hitinl1:
        desc = "Loads that hit in L1";
        snprintf(str, sizeof(str), "%.2f %%", 100 * nL1Hits / nCachableLoads);
        break;
      case pi_l1phitrate:
        desc = "L1P hit rate";
        snprintf(str, sizeof(str), "%.2f %%", 100 * nL1PHits / nL1PAccesses);
        break;
        //case pi_overhead:
        //desc = "Instr overhead";
        //snprintf(str, sizeof(str), "%.2f %%", 100 * (nInstructions - nNecessaryInstr) / nInstructions);
        //break;
      case pi_hitinl1p:
        desc = "Loads that hit in L1P";
        snprintf(str, sizeof(str), "%f %%" ,  100 * nL1PHits / nCachableLoads);
        break;
      case pi_l2hitrate:
        desc = "L2 hit rate";
        snprintf(str, sizeof(str), "%.2f %%", 100 * nL2Hits / nL2Accesses);
        break;
      case pi_dcbthitrate:
        desc = "dcbt hit rate";
        snprintf(str, sizeof(str), "%.2f %%", 100 * nDcbtHits / nDcbtAccesses);
        break;
      case pi_axufraction:
        desc = "FXU instrs";
        snprintf(str, sizeof(str), "%.2f %%", 100 * nAXUInstr / nXUAXUInstr);
        break;
      case pi_l1pliststarted:
        desc = "List prefetch started";
        snprintf(str, sizeof(str), "%llu", nL1PListStarted);
        break;
      case pi_l1plistabandoned:
        desc = "List prefetch abandoned";
        snprintf(str, sizeof(str), "%llu", nL1PListAbandoned);
        break;
      case pi_l1plistmismatch:
        desc = "List prefetch mismatch";
        snprintf(str, sizeof(str), "%llu", nL1PListMismatch);
        break;
      case pi_l1plistskips:
        desc = "List prefetch skip";
        snprintf(str, sizeof(str), "%llu", nL1PListSkips);
        break;
      case pi_l1plistoverruns:
        desc = "List prefetch overrun";
        snprintf(str, sizeof(str), "%llu", nL1PListOverruns);
        break;
      case pi_l1plistlatestalls:
        desc = "Stalls list prefetch behind";
        snprintf(str, sizeof(str), "%.2f", nL1PLatePrefetchStalls / nCoreCycles);
        break;
      default:
        continue;
      }

      printf("%"SCELLWIDTH"s|", str);
    }

    printf(" %s\n", desc);
  }
#endif
}


static void exec_table(const benchfunc_t &benchmark, bgq_hmflags additional_opts, bgq_hmflags kill_opts, int j_max) {
//static void exec_table(bool sloppiness, hm_func_double hm_double, hm_func_float hm_float, bgq_hmflags additional_opts) {
  assert(communicator);
  g_nproc = communicator->getRankCount();
  auto g_proc_id = communicator->getMPIMyRank();
  
  benchstat excerpt;

  if (g_proc_id == 0)
    printf("%10s|", "");
  for (int i3 = 0; i3 < lengthof(flags); i3 += 1) {
    if (g_proc_id == 0) {
      auto flagdesc = flags_desc[i3];
      printf("%-" SCELLWIDTH "s|", flagdesc);
    }
  }
  if (g_proc_id == 0)
    printf("\n");
  print_repeat("-", 10 + 1 + (CELLWIDTH + 1) * lengthof(flags));
  if (g_proc_id == 0)
    printf("\n");
  for (int i2 = 0; i2 < lengthof(omp_threads); i2 += 1) {
    int threads = omp_threads[i2];

    if (g_proc_id == 0)
      printf("%-10s|", omp_threads_desc[i2]);

    benchstat stats[lengthof(flags)];
    for (int i3 = 0; i3 < lengthof(flags); i3 += 1) {
      bgq_hmflags hmflags = (bgq_hmflags)flags[i3];
      hmflags =  (bgq_hmflags)((hmflags | additional_opts) & ~kill_opts);
 
      benchstat result = runbench(benchmark, hmflags, j_max, threads);
      stats[i3] = result;

      if (threads == 64 && i3 == 3) {
        excerpt = result;
      }
 
      char str[80] = { 0 };
      if (result.avgtime == 0)
        snprintf(str, sizeof(str), "~ %s", (result.error > 0.001) ? "X" : "");
      else
        snprintf(str, sizeof(str), "%.2f mlup/s%s", (double) result.lup / (result.avgtime * MEGA), (result.error > 0.001) ? "X" : "");
      if (g_proc_id == 0)
        printf("%" SCELLWIDTH "s|", str);
      if (g_proc_id == 0)
        fflush(stdout);
    }
    if (g_proc_id == 0)
      printf("\n");

    if (g_proc_id == 0) {
      print_stats(stats);
    }

    print_repeat("-", 10 + 1 + (CELLWIDTH + 1) * lengthof(flags));
    if (g_proc_id == 0)
      printf("\n");
  }

  if (g_proc_id == 0) {
    printf("Hardware counter excerpt (64 threads, nocom):\n");
    mypapi_print_counters(&excerpt.counters);
  }
  if (g_proc_id == 0)
    printf("\n");
}


void molly::exec_bench(const benchfunc_t &func, int nTests) {
  exec_table(func, (bgq_hmflags)0, (bgq_hmflags)0, nTests);
}

#pragma endregion
