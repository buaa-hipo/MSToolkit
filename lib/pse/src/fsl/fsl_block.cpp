#include <fsl/fsl_block.h>
#include <string.h>

using namespace pse::fsl;

void pse::fsl::SuperBlock::_init()
{
    this->magic = MAGIC;
    this->rootId = 1;
}
