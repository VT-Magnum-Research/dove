//
//  main.cpp
//  DeploymentOptimization
//
//  Created by Hamilton Turner on 1/29/13.
//  Copyright (c) 2013 Virginia Tech. All rights reserved.
//
#include <boost/mpi.hpp>
#include <boost/mpi/collectives.hpp>
#include <boost/mpi/environment.hpp>
#include <boost/mpi/communicator.hpp>
#include <iostream>
namespace mpi = boost::mpi;

// TODO: Sends need to be non-blocking. Perhaps also replace the current recv with receive all so that there are
// not unexpected items. Also add in the ability to remove the data on the send
int main(int argc, char* argv[])
{

  mpi::environment env(argc, argv);
  mpi::communicator world;
  long execution_times [4] = {1, 6, 1, 2};

  switch (world.rank()) {
    case 0:
      std::cout << "0: Awake" << std::endl;
      time_t start0;
      start0 = time (NULL);
      
      std::cout << "0: Started compute" << std::endl;
      
      while ((time(NULL)-start0) < execution_times[0]);
      
      std::cout << "0: Finished compute in " << (time(NULL)-start0) << std::endl;
      
      world.send(1, 0 , 0); // Send '0' to rank 1 with tag '0'
      
      std::cout << "0: Sent tag 0 to rank 1" << std::endl;
      world.send(2, 0 , 0);
      std::cout << "0: Sent tag 1 to rank 2" << std::endl;
      
      
      break;
    case 1:
      std::cout << "1: Awake" << std::endl;
      int data1;
      world.recv(0, 0, data1); // Recv tag '0' from rank 0
      std::cout << "1: Recv tag 0 from rank 0" << std::endl;
      time_t start1;
      start1 = time (NULL);
      std::cout << "1: Started compute" << std::endl;
      while ((time(NULL)-start1) < execution_times[1]);
      std::cout << "1: Finished compute in " << (time(NULL)-start1) << std::endl;
      world.send(3, 0, 0);
      break;
    case 2:
      std::cout << "2: Awake" << std::endl;
      int data2;
      world.recv(0, 0, data2);
      std::cout << "2: Recv tag 0 from rank 0" << std::endl;
      time_t start2;
      start2 = time (NULL);
      std::cout << "2: Started compute" << std::endl;
      while ((time(NULL)-start2) < execution_times[2]);
      std::cout << "2: Finished compute in " << (time(NULL)-start2) << std::endl;
      world.send(3, 0, 0);
      break;
    case 3:
      std::cout << "3: Awake" << std::endl;
      int data30, data31;
      world.recv(1, 0, data30);
      std::cout << "3: Recv tag 0 from rank 1" << std::endl;
      world.recv(2, 0, data31);
      std::cout << "3: Recv tag 0 from rank 2" << std::endl;
      time_t start3;
      start3 = time (NULL);
      std::cout << "3: Started compute" << std::endl;
      while ((time(NULL)-start3) < execution_times[3]);
      std::cout << "3: Finished compute in " << (time(NULL)-start3) << std::endl;
      
      std::cout << "3: DONE!!" << std::endl;
      break;
  }
  
  return 0;
}
