// module lowlevel
#include "argus/lowlevel/memory.hpp"

// module render_opengl
#include "internal/render_opengl/glext.hpp"
#include "internal/render_opengl/render_bucket.hpp"

namespace argus {
    // forward declarations
    class Material;

    static AllocPool g_bucket_pool(sizeof(RenderBucket));

    RenderBucket &RenderBucket::create(const Material &material) {
        return g_bucket_pool.construct<RenderBucket>(material);
    }

    void RenderBucket::destroy(void) {
        g_bucket_pool.free(this);
    }
}
