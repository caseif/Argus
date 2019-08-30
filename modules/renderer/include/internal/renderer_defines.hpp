#define _VERTEX_POSITION_LEN 2
#define _VERTEX_COLOR_LEN 4
#define _VERTEX_TEXCOORD_LEN 3
#define _VERTEX_LEN (_VERTEX_POSITION_LEN + _VERTEX_COLOR_LEN + _VERTEX_TEXCOORD_LEN)
#define _VERTEX_WORD_LEN sizeof(float)

#define _GL_LOG_MAX_LEN 255

#define _UNIFORM_PROJECTION "_argus_uni_projection_matrix"
#define _UNIFORM_TEXTURE "_argus_uni_sampler_array"
#define _UNIFORM_LAYER_TRANSFORM "_argus_uni_layer_transform"
#define _UNIFORM_GROUP_TRANSFORM "_argus_uni_group_transform"

#define _OUT_FRAGDATA "_argus_out_frag_data"

#define _ATTRIB_POSITION "_argus_in_position"
#define _ATTRIB_COLOR "_argus_in_color"
#define _ATTRIB_TEXCOORD "_argus_in_texCoord"

#define _ATTRIB_LOC_POSITION 0
#define _ATTRIB_LOC_COLOR 1
#define _ATTRIB_LOC_TEXCOORD 2

#define RESOURCE_TYPE_TEXTURE_PNG "image/png"
