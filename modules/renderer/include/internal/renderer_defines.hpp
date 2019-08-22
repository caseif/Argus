#define __VERTEX_POSITION_LEN 2
#define __VERTEX_COLOR_LEN 4
#define __VERTEX_TEXCOORD_LEN 3
#define __VERTEX_LEN (__VERTEX_POSITION_LEN + __VERTEX_COLOR_LEN + __VERTEX_TEXCOORD_LEN)
#define __VERTEX_WORD_LEN sizeof(float)

#define __GL_LOG_MAX_LEN 255

#define __UNIFORM_PROJECTION "_argus_uni_projection_matrix"
#define __UNIFORM_TEXTURE "_argus_uni_sampler_array"
#define __UNIFORM_LAYER_TRANSFORM "_argus_uni_layer_transform"
#define __UNIFORM_GROUP_TRANSFORM "_argus_uni_group_transform"

#define __OUT_FRAGDATA "_argus_out_frag_data"

#define __ATTRIB_POSITION "_argus_in_position"
#define __ATTRIB_COLOR "_argus_in_color"
#define __ATTRIB_TEXCOORD "_argus_in_texCoord"

#define __ATTRIB_LOC_POSITION 0
#define __ATTRIB_LOC_COLOR 1
#define __ATTRIB_LOC_TEXCOORD 2
