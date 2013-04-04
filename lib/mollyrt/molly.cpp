#include <mollyrt/molly.h>

#include <malloc.h>
#include <cstdio>
#include <mpi.h> 

using namespace molly;

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


  int providedThreadLevel;
  int world_ranks;
  rank_t world_self;
  
  int cart_dims;
  MPI_Comm cart_self;
  MPI_Comm cart_comm;
  int *cart_lengths;
  rank_t *cart_self_coords;


  void init(int &argc, char **&argv, int clusterDims, int *dimLengths, bool *dimPeriodical) {
    MPI_CHECK(MPI_Init_thread(&argc, &argv, MPI_THREAD_SINGLE, &providedThreadLevel)); //TODO: Molly needs to insert an initialization into main that calls this
    MPI_CHECK(MPI_Comm_rank(MPI_COMM_WORLD, &world_self));

    int expectedRanks = 1; 
#ifdef _MSC_VER
    int *periods = (int*)_malloca(sizeof(periods[0]) * clusterDims);
#else
    int periods[clusterDims];
    cart_lengths = new int[clusterDims];
    cart_self_coords = new rank_t[clusterDims];
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
    MPI_CHECK(MPI_Cart_create(MPI_COMM_WORLD, clusterDims, dimLengths, periods, true, &cart_comm));
    cart_dims = clusterDims;
    MPI_CHECK(MPI_Comm_rank(cart_comm, &cart_self));
    MPI_CHECK(MPI_Cart_coords(cart_comm, cart_self, clusterDims, cart_self_coords));

  #ifdef _MSC_VER
  _freea(periods);
#endif
  }



  void finalize() {
    delete[] cart_lengths;
    cart_lengths = NULL;
    delete[] cart_self_coords;
    cart_self_coords = NULL;
    MPI_CHECK(MPI_Finalize());
  }


  void broadcast_send(const void *sendbuf, size_t size) {
    MPI_CHECK(MPI_Bcast(const_cast<void*> (sendbuf), size, MPI_BYTE, cart_self, cart_comm));
  }

   void broadcast_recv(void *recvbuf, size_t size, rank_t sender) {
     assert(sender != cart_self);
     MPI_CHECK(MPI_Bcast(recvbuf, size, MPI_BYTE, sender, cart_comm));
   }


class MemcpyCommunicator : public CommunicatorCommon {
public:
   ~MemcpyCommunicator() {}
  MemcpyCommunicator() {}
}; // class MemcpyCommunicator



class Communicator : public TheCommunicator {};
}

rank_t molly::getMyRank() {
}


bool molly::isMaster() {
}


int molly::getClusterDims() {
}


int molly::getClusterLength(int d) {
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
