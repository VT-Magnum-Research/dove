#include <ctime>
#include <libaco/util.h>

unsigned int Util::random_number(unsigned int range) {
  static bool seeded = false;
  if(!seeded) {
    srand((int)time(0));
    seeded = true;
  }
  return (rand() % range);
}
