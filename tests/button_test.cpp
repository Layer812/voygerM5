#include <cassert>
#include "button_latch.hpp"
int main(){
 button_latch::Key b;b.begin(false,0);
 b.update(true,5);b.update(true,16);assert(!b.held());
 b.update(true,18);assert(b.held());
 b.update(true,200);b.update(false,202);b.update(false,230);
 assert(!b.held());assert(b.takePress());assert(!b.takePress());
 b.update(true,280);b.update(true,299);assert(b.takePress());
}
