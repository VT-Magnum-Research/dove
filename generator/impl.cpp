#include <boost/mpi.hpp>
#include <boost/mpi/collectives.hpp>
#include <boost/mpi/environment.hpp>
#include <boost/mpi/communicator.hpp>
namespace mpi = boost::mpi;

int main(int argc, char* argv[]) {
  mpi::environment env(argc, argv);
  mpi::communicator world;
  switch (world.rank()) {
    case 0: {
      std::cout << "0: Awake" << std::endl;
      time_t start;
      start = time(NULL);
      std::cout << "0: Started compute" << std::endl;
      while ((time(NULL)-start) < 0);
      std::cout << "0: Finished compute in " << (time(NULL)-start) << std::endl;
      mpi::request sreq[1];
      sreq[0] = world.isend(1, 0);
      mpi::wait_all(sreq, sreq + 1);
      std::cout << "0: Sent notice to succ 1" << std::endl;
      break; }
    case 1: {
      std::cout << "1: Awake" << std::endl;
      mpi::request req[1];
      req[0] = world.irecv(0, 0);
      mpi::wait_all(req, req + 1);
      std::cout << "1: Recv notice from pred 0" << std::endl;
      time_t start;
      start = time(NULL);
      std::cout << "1: Started compute" << std::endl;
      while ((time(NULL)-start) < 9);
      std::cout << "1: Finished compute in " << (time(NULL)-start) << std::endl;
      mpi::request sreq[2];
      sreq[0] = world.isend(2, 0);
      sreq[1] = world.isend(3, 0);
      mpi::wait_all(sreq, sreq + 2);
      std::cout << "1: Sent notice to succ 2" << std::endl;
      std::cout << "1: Sent notice to succ 3" << std::endl;
      break; }
    case 2: {
      std::cout << "2: Awake" << std::endl;
      mpi::request req[1];
      req[0] = world.irecv(1, 0);
      mpi::wait_all(req, req + 1);
      std::cout << "2: Recv notice from pred 1" << std::endl;
      time_t start;
      start = time(NULL);
      std::cout << "2: Started compute" << std::endl;
      while ((time(NULL)-start) < 4);
      std::cout << "2: Finished compute in " << (time(NULL)-start) << std::endl;
      mpi::request sreq[2];
      sreq[0] = world.isend(3, 0);
      sreq[1] = world.isend(4, 0);
      mpi::wait_all(sreq, sreq + 2);
      std::cout << "2: Sent notice to succ 3" << std::endl;
      std::cout << "2: Sent notice to succ 4" << std::endl;
      break; }
    case 3: {
      std::cout << "3: Awake" << std::endl;
      mpi::request req[2];
      req[0] = world.irecv(1, 0);
      req[1] = world.irecv(2, 0);
      mpi::wait_all(req, req + 2);
      std::cout << "3: Recv notice from pred 1" << std::endl;
      std::cout << "3: Recv notice from pred 2" << std::endl;
      time_t start;
      start = time(NULL);
      std::cout << "3: Started compute" << std::endl;
      while ((time(NULL)-start) < 6);
      std::cout << "3: Finished compute in " << (time(NULL)-start) << std::endl;
      mpi::request sreq[1];
      sreq[0] = world.isend(4, 0);
      mpi::wait_all(sreq, sreq + 1);
      std::cout << "3: Sent notice to succ 4" << std::endl;
      break; }
    case 4: {
      std::cout << "4: Awake" << std::endl;
      mpi::request req[2];
      req[0] = world.irecv(2, 0);
      req[1] = world.irecv(3, 0);
      mpi::wait_all(req, req + 2);
      std::cout << "4: Recv notice from pred 2" << std::endl;
      std::cout << "4: Recv notice from pred 3" << std::endl;
      time_t start;
      start = time(NULL);
      std::cout << "4: Started compute" << std::endl;
      while ((time(NULL)-start) < 3);
      std::cout << "4: Finished compute in " << (time(NULL)-start) << std::endl;
      mpi::request sreq[1];
      sreq[0] = world.isend(5, 0);
      mpi::wait_all(sreq, sreq + 1);
      std::cout << "4: Sent notice to succ 5" << std::endl;
      break; }
    case 5: {
      std::cout << "5: Awake" << std::endl;
      mpi::request req[1];
      req[0] = world.irecv(4, 0);
      mpi::wait_all(req, req + 1);
      std::cout << "5: Recv notice from pred 4" << std::endl;
      time_t start;
      start = time(NULL);
      std::cout << "5: Started compute" << std::endl;
      while ((time(NULL)-start) < 0);
      std::cout << "5: Finished compute in " << (time(NULL)-start) << std::endl;
      std::cout << "5: DONE!!" << std::endl;
      break; }
  }
  return 0;
}

