

struct {
void* dst;
const void* src;
size_t sizeBytes;
hipMemcpyKind kind;
} hipMemcpy;

struct {
void* dst;
size_t dpitch;
const void* src;
size_t spitch;
size_t width;
size_t height;
hipMemcpyKind kind;
} hipMemcpy2D;

