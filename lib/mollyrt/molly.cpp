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

#define MOLLY_COMMUNICATOR_MPI

extern "C" void test() {
}

namespace molly {
  class MPICommunicator;
  class MemcpyCommunicator;



  class CommunicatorCommon {
  private:
	  // Static classes, do not instantiate
	    ~CommunicatorCommon() = delete;
	    CommunicatorCommon() = delete;
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
	  MOLLY_DEBUG(#CALL); \
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

  // Will be generated by mollycc
  extern "C" const int __molly_cluster_dims;
  extern "C" const int __molly_cluster_size;
  extern "C" const int *__molly_cluster_lengths;
  extern "C" const int *__molly_cluster_periodic;

  // MPI_COMM_WORLD communicator or equivalent
  int _world_ranks;
  rank_t _world_self = 0/*This means all ranks will print debug info until init() called*/;

  // Cartesian grid communicator
  rank_t _cart_self;
  int _cart_dims = 0;
  int *_cart_lengths;
  rank_t *_cart_self_coords;


  class MPICommunicator : public CommunicatorCommon {
    static int providedThreadLevel;
    static MPI_Comm _cart_comm;

  public:
    static void init(int &argc, char **(&argv)) {
    	// MPI_Init_thread may look for arguments intended to configure MPI, and then remove these args to avoid processing by the user program
    	MPI_CHECK(MPI_Init_thread(&argc, &argv, MPI_THREAD_SINGLE/*TODO: Support OpenMP*/, &providedThreadLevel));
    	MPI_CHECK(MPI_Comm_size(MPI_COMM_WORLD, &_world_ranks));
    	MPI_CHECK(MPI_Comm_rank(MPI_COMM_WORLD, &_world_self));
    	RTASSERT(_world_ranks == __molly_cluster_size, "Have to mpirun with exact shape that was used when compiling");

#ifndef NDEBUG
    	MPI_CHECK(MPI_Barrier(MPI_COMM_WORLD)); // Wait for all ranks to avoid mixing messages with non-master ranks
    	if (__molly_isMaster()) {
    		std::cerr << "###############################################################################\n";
    	}
#endif

    	MOLLY_DEBUG("__molly_cluster_dims="<<__molly_cluster_dims << " __molly_cluster_size="<<__molly_cluster_size);
    	for (auto d = __molly_cluster_dims-__molly_cluster_dims; d < __molly_cluster_dims; d+=1) {
    		MOLLY_DEBUG("d="<<d << " __molly_cluster_lengths[d]="<<__molly_cluster_lengths[d] << " __molly_cluster_periodic[d]="<<__molly_cluster_periodic[d]);
    	}

    	_cart_dims = __molly_cluster_dims;
	   MPI_CHECK(MPI_Cart_create(MPI_COMM_WORLD, _cart_dims, const_cast<int*>(__molly_cluster_lengths), const_cast<int*>(__molly_cluster_periodic), true, &_cart_comm));
	   MPI_CHECK(MPI_Comm_rank(_cart_comm, &_cart_self));
	   MOLLY_DEBUG("_cart_self="<<_cart_self);

	   _cart_self_coords = static_cast<int*>(malloc(sizeof(*_cart_self_coords) * _cart_dims)); // TODO: mollycc could preallocate some space
	   MPI_CHECK(MPI_Cart_coords(_cart_comm, _cart_self, __molly_cluster_dims, _cart_self_coords));

#ifndef NDEBUG
	   std::string cartcoord;
	   std::stringstream os;
	   for (auto d=__molly_cluster_dims-__molly_cluster_dims; d<__molly_cluster_dims;d+=1) {
		   if (d!=0)
			   os<<",";
		   os<<_cart_self_coords[d];
	   }
	   auto tmp = _world_self;
	   _world_self = 0;
	   MOLLY_DEBUG("Here is rank " << tmp << " at cart coord ("<<os.rdbuf()<<")");
	   _world_self = tmp;
	   MPI_CHECK(MPI_Barrier(MPI_COMM_WORLD));
#endif
    }

    static void finalize() {
    	 MPI_CHECK(MPI_Finalize());
    	 free(_cart_self_coords);
    	 _cart_self_coords = NULL;
    }


    static  void scatter(void *sendbuf, size_t size, void *recvbuf) {

    }


    static  void broadcast_send(const void *sendbuf, size_t size) { MOLLY_DEBUG_FUNCTION_SCOPE
      MPI_CHECK(MPI_Bcast(const_cast<void*> (sendbuf), size, MPI_BYTE, _cart_self, _cart_comm));
    }


    static void broadcast_recv(void *recvbuf, size_t size, rank_t sender) { MOLLY_DEBUG_FUNCTION_SCOPE
      assert(sender != _cart_self);
      MPI_CHECK(MPI_Bcast(recvbuf, size, MPI_BYTE, sender, _cart_comm));
    }

  }; // class MPICommunicator

   int MPICommunicator::providedThreadLevel;
   MPI_Comm MPICommunicator::_cart_comm;

  //static int _providedThreadLevel;


  void broadcast_send(const void *sendbuf, size_t size) { MOLLY_DEBUG_FUNCTION_SCOPE
   TheCommunicator::broadcast_send(sendbuf, size);
  }

  void broadcast_recv(void *recvbuf, size_t size, rank_t sender) { MOLLY_DEBUG_FUNCTION_SCOPE
	  TheCommunicator::broadcast_recv(recvbuf, size, sender);
  }

  int cart_dims() {
    return _cart_dims;
  }

  int cart_self_coord(int d) { MOLLY_DEBUG_FUNCTION_SCOPE
    assert(d >= 0);
    if (d >= cart_dims())
      return 0;
    MOLLY_VAR(d, _cart_self_coords[d]);
    return _cart_self_coords[d];
  }

  rank_t world_self() {
	  return _world_self;
  }

  class MemcpyCommunicator : public CommunicatorCommon {
  public:
  }; // class MemcpyCommunicator



  class Communicator : public TheCommunicator {};
}

rank_t molly::getMyRank() { MOLLY_DEBUG_FUNCTION_SCOPE
  return _cart_self;
}


bool molly::isMaster() { MOLLY_DEBUG_FUNCTION_SCOPE
  return _world_self==0;
}


extern "C" bool __molly_isMaster() {
	return _world_self==0;
}


int molly::getClusterDims() { MOLLY_DEBUG_FUNCTION_SCOPE
  return _cart_dims;
}


int molly::getClusterLength(int d) {
  assert(0 <= d /*&& d < _cart_dims*/);
  if (d > _cart_dims)
    return 1;
  return _cart_lengths[d];
}



//static Communicator communicator;


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
// This is a weak symbol because we want it compilable without mollycc, when main is not renamed to __molly_orig_main
extern "C" int __molly_orig_main(int argc, char *argv[]) __attribute__((weak));
// ... and a new main generated, that just calls __molly_main
extern "C" int main(int argc, char *argv[]);


extern "C" int __molly_main(int argc, char *argv[]) {
  MOLLY_DEBUG_FUNCTION_SCOPE
  assert(&__molly_orig_main && "Must be compiled using mollycc");

  //TODO: We could change the communicator dynamically using argc,argv or getenv()
  Communicator::init(argc, argv);
  //FIXME: Exception-handling, but currently we do not support exceptions
  auto retval = __molly_orig_main(argc, argv);
  Communicator::finalize();
  return retval;
}
