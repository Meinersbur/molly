/**
 * Molly Runtime Library
 *
 * Do not use __builtin_molly_* in here, they won't work. In fact, most builtins from the user application will be transformed to to calls to this runtime library
 */
#include "molly.h"

#include <malloc.h>
#include <cstdio>
#include <mpi.h> 
#include <assert.h>
 
using namespace molly;

extern "C" void test() {
}

namespace molly {
  class MPICommunicator;
  class MemcpyCommunicator;



  class CommunicatorCommon {
  public:
  };

#ifdef MOLLY_COMMUNICATOR_MPI
  typedef MPICommunicator TheCommunicator;
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


#define MPI_CHECK(CALL) \
  do { \
  auto retval = (CALL); \
  if (retval!=MPI_SUCCESS) { \
  ERROREXIT("MPI fail: %s\nReturned: %d\n", #CALL, retval); \
  } \
  } while (0)

#define RTASSERT(ASSUMPTION, ...) \
  do { \
  if (!ASSUMPTION) { \
  fprintf(stderr, "\nAssertion  fail: %s", #ASSUMPTION); \
  ERROREXIT(__VA_ARGS__); \
  } \
  } while (0)



  void foo() {
    ERROREXIT("X");
    RTASSERT(true, "Y");
  }

  class MPICommunicator : public CommunicatorCommon {
  private:
    int providedThreadLevel;
    int ranks;
    rank_t self;

  public:
    ~MPICommunicator() {
      MPI_CHECK(MPI_Finalize());
    }
    MPICommunicator() {
#if 0
      char *argv_data[] = {"molly", NULL};
      char **argv = argv_data;
      int argc = sizeof(argv)/sizeof(argv[0]) - 1;
      MPI_CHECK(MPI_Init_thread(&argc, &argv, MPI_THREAD_SINGLE, &providedThreadLevel)); //TODO: Molly needs to insert an initialization into main that calls this
#endif

      MPI_CHECK(MPI_Comm_size(MPI_COMM_WORLD, &ranks));
      MPI_CHECK(MPI_Comm_rank(MPI_COMM_WORLD, &self));
    }




    void scatter(void *sendbuf, size_t size, void *recvbuf) {

    }
  }; // class MPICommunicator


  int _providedThreadLevel;
  int _world_ranks;
  rank_t _world_self;

  int _cart_dims = 0;
  rank_t _cart_self;
  MPI_Comm _cart_comm;
  int *_cart_lengths;
  rank_t *_cart_self_coords;


  void init(int &argc, char **&argv, int clusterDims, int *dimLengths, bool *dimPeriodical) {
    MPI_CHECK(MPI_Init_thread(&argc, &argv, MPI_THREAD_SINGLE, &_providedThreadLevel)); //TODO: Molly needs to insert an initialization into main that calls this
    MPI_CHECK(MPI_Comm_rank(MPI_COMM_WORLD, &_world_self));

    int expectedRanks = 1; 
#ifdef _MSC_VER
    int *periods = (int*)_malloca(sizeof(periods[0]) * clusterDims);
#else
    int periods[clusterDims];
    _cart_lengths = new int[clusterDims];
    _cart_self_coords = new rank_t[clusterDims];
#endif
    for (auto d = 0; d < clusterDims; d+=1) {
      expectedRanks *= dimLengths[d];
      periods[d] = dimPeriodical[d];
    }
    assert(expectedRanks >= 1);


    int realRanks;
    MPI_CHECK(MPI_Comm_size(MPI_COMM_WORLD, &realRanks));
    RTASSERT(realRanks == expectedRanks, "Not running on shape that this executable has been compiled for");

    //MPI_Get_processor_name(processor_name, &namelen);

    //MPI_CHECK(MPI_Dims_create(world_ranks, clusterDims, dimLengths)); 
    MPI_CHECK(MPI_Cart_create(MPI_COMM_WORLD, clusterDims, dimLengths, periods, true, &_cart_comm));
    _cart_dims = clusterDims;
    MPI_CHECK(MPI_Comm_rank(_cart_comm, &_cart_self));
    MPI_CHECK(MPI_Cart_coords(_cart_comm, _cart_self, clusterDims, _cart_self_coords));

#ifdef _MSC_VER
    _freea(periods);
#endif
  }



  void finalize() {
    delete[] _cart_lengths;
    _cart_lengths = NULL;
    delete[] _cart_self_coords;
    _cart_self_coords = NULL;
    MPI_CHECK(MPI_Finalize());
  }


  void broadcast_send(const void *sendbuf, size_t size) {
    MPI_CHECK(MPI_Bcast(const_cast<void*> (sendbuf), size, MPI_BYTE, _cart_self, _cart_comm));
  }

  void broadcast_recv(void *recvbuf, size_t size, rank_t sender) {
    assert(sender != _cart_self);
    MPI_CHECK(MPI_Bcast(recvbuf, size, MPI_BYTE, sender, _cart_comm));
  }

  int cart_dims() {
    return _cart_dims;
  }

  int cart_self_coord(int d) {
    assert(d >= 0);
    if (d >= cart_dims())
      return 0;
    return _cart_self_coords[d];
  }

  rank_t world_self() {
	  return _world_self;
  }

  class MemcpyCommunicator : public CommunicatorCommon {
  public:
    ~MemcpyCommunicator() {}
    MemcpyCommunicator() {}
  }; // class MemcpyCommunicator



  class Communicator : public TheCommunicator {};
}

rank_t molly::getMyRank() {
  return _cart_self;
}


bool molly::isMaster() {
  return _world_self==0;
}


int molly::getClusterDims() {
  return _cart_dims;
}


int molly::getClusterLength(int d) {
  assert(0 <= d /*&& d < _cart_dims*/);
  if (d > _cart_dims)
    return 1;
  return _cart_lengths[d];
}



static Communicator communicator; 


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


// If running with molly optimization, the original main method will be renamed to this one
extern "C" int __molly_orig_main(int argc, char *argv[]) __attribute__((weak));
// ... and a new main generated, that just calls __molly_main
extern "C" int main(int argc, char *argv[]);


extern "C" int __molly_main(int argc, char *argv[]) {
  MOLLY_DEBUG_FUNCTION_SCOPE
  assert(&__molly_orig_main && "Must be compiled using mollycc");
  //TODO: MPI_Init
  return __molly_orig_main(argc, argv);//TODO: MPI may modify arguments
}
