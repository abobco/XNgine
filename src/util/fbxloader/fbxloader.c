/* 
    By Austin Bobco 9/22/2020

    Written using info from this article
    https://code.blender.org/2013/08/fbx-binary-file-format-specification/

    WIP
*/


#include "fbxloader.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>

#include <sys/types.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include <sys/stat.h>

#include "zlib.h"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "../bob_math.h"

#define FBX_DEBUG 0 // toggle printing loaded FBX nodes
#define FBX_LOGFILE NULL // filename or null for stdout

#ifndef DEFAULT_MESH_VERTEX_BUFFERS
    #define DEFAULT_MESH_VERTEX_BUFFERS    7    // Number of vertex buffers (VBO) per mesh
#endif

typedef struct FBX_Node {
    void *data;
    size_t size;
    char type;
    struct FBX_Node *next;
} FBX_Node;

typedef struct ObjectAttribute {
    FBX_Node *attributes;
    char *name;
    int id;
} ObjectAttribute;

static FBX_Node *FBX_load_file(const char *filename);
static Model FBX_list_to_Model(FBX_Node *attribute_list, FILE *log_file);
#if FBX_DEBUG
static void FBX_list_print(FBX_Node *attribute_list, FILE *out);
/* print a flat array of numbers as 3-tuples, uses a C11 Generic macro for formatting */
#define print_vertex_array(out, arr, size) ({ \
    for ( int i = 0; i < size/sizeof(*arr); i++ ) { \
        if ( i % 3 == 0)    \
            fprintf(out, "\n\t"); \
        fprintf(out, printf_dec_format(*arr),  arr[i]); \
        fprintf(out, " "); \
    } \
})
#endif

Model LoadModel_FBX(const char *filename) {
    FBX_Node *attrs = FBX_load_file(filename);
    FILE *log_file = stdout;
#if FBX_DEBUG
    if ( FBX_LOGFILE != NULL ) 
        log_file = fopen(FBX_LOGFILE, "w+");
    FBX_list_print(attrs, stdout);
#endif
    return FBX_list_to_Model(attrs, log_file);
}

static const char *RelevantNodes[] = {
    "Geometry",
    "Vertices",
    "PolygonVertexIndex",
    "Edges",
    "Normals",
    "UV",
    "UVIndex",
    "AnimationStack",
    "AnimationLayer",
    "AnimationCurve",
    "KeyValueFloat",
    // "KeyVer",
    // "KeyTime",
    // "KeyAttrFlags",
    // "KeyAttrDataFloat",
    // "KeyAttrRefCount",
    NULL
};

static bool string_in_list(const char* str, const char **str_list) {
    int i =0;
    while ( RelevantNodes[i] != NULL )
        if ( strcmp(str, str_list[i++]) == 0 )
            return true;
    return false;
}

void FBX_Node_list_append(FBX_Node **head, void *data, size_t data_size, char type) {
    FBX_Node *new_node = malloc(sizeof(FBX_Node));
    new_node->data = malloc(data_size);
    new_node->next = NULL;
    new_node->type = type;
    new_node->size = data_size;
    // copy each byte of memory
    for ( int i=0; i<data_size; i++ ) 
        *(char *)(new_node->data + i) = *(char *)(data + i); 

    if ( *head == NULL) {
        *head = new_node;
        return;
    }

    FBX_Node *last = *head;
    while ( last->next != NULL ) {
        last = last->next;
    }

    last->next = new_node;
}

static int FBX_list_max_id(FBX_Node *attribute_list) {
    FBX_Node *iter = attribute_list;
    int max_id = 0;
    while ( iter != NULL) {
        assert(iter->type == 'O');
        ObjectAttribute *attr = (ObjectAttribute*)iter->data;
        max_id = attr->id > max_id ? attr->id : max_id;
        iter = iter->next;
    }
    return max_id;
}

static FBX_Node *FBX_list_search(FBX_Node *attr_list, const char *attr_name, int id) {
    FBX_Node *iter = attr_list;
    while ( iter != NULL) {
        assert(iter->type == 'O');
        ObjectAttribute *attr = (ObjectAttribute*)iter->data;
        if ( strcmp(attr->name, attr_name) == 0 && attr->id == id ) {
            return attr->attributes;
        }
        iter = iter->next;
    }
    return NULL;
} 


#define Z_CHUNK 16384*16

/* FBX array typed nodes w/ encoding "1" are deflate zipped
   There are (several) copies of zlib in the default raspbian system headers, so using that to unzip */
static int inflate_array(void *buf, unsigned char *out, size_t defl_size, size_t infl_size) {
    /* allocate inflate state */
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;

    inflateInit(&strm);
    strm.avail_in = defl_size;
    strm.next_in = (unsigned char*) buf;

    strm.avail_out = infl_size;
    strm.next_out = out;
    return inflate(&strm, Z_NO_FLUSH);
}

static void visit_prop(char *buf, int *start_idx, FBX_Node **attr_list_head, FILE *node_summary) {
    int idx = *start_idx;
    char prop_type = buf[idx++];
    // printf("Prop type: %c\n", prop_type);
    switch (prop_type) {
    // arrays

    case 'f':   // 4 byte IEEE 754 floats
    case 'd':   // 8 byte IEEE 754 floats
    case 'i':   // 4 byte signed ints
    case 'l':   // 8 byte signed ints
    {
        // read array info from file
        uint32_t *ibuf = (uint32_t*) &buf[idx];
        uint32_t array_len = *ibuf++;
        uint32_t encoding = *ibuf++;
        uint32_t datasize = *ibuf++;
        idx += sizeof(uint32_t)*3;
    
        if ( encoding == 1 ) {
            // zipped array

            // $0.05 to anyone who knows a better way to map sizes to these characters
            size_t typesize = 0;
            switch ( prop_type ) {
            case 'f': typesize = sizeof(float); break;
            case 'd': typesize = sizeof(double); break;
            case 'i': typesize = sizeof(int32_t); break;
            case 'l': typesize = sizeof(int64_t); break;
            }

            const unsigned inflated_size = array_len*typesize;
            unsigned char out[inflated_size];
            inflate_array(ibuf, out, datasize, inflated_size);

            FBX_Node_list_append(attr_list_head, &out, inflated_size, prop_type);
        } else {
            // no need to unzip
            FBX_Node_list_append(attr_list_head, ibuf, datasize, prop_type);
        }
        idx += datasize;
    }
    break;

    // scalars

    // 1 bit bool, the LSB of a 1 byte value
    case 'C': idx++; break;
    // 2 byte signed int
    case 'Y': idx += sizeof(int16_t); break;
    // 4 byte signed int
    case 'I': idx += sizeof(int32_t); break;
    // 8 byte signed int
    case 'L': idx += sizeof(int64_t); break;
    // 4 byte IEEE 754 float
    case 'F': idx += sizeof(float); break;
    // 8 byte IEEE 754 float
    case 'D': idx += sizeof(double); break;

    /*
    FBX string type:
        - does not end with a string terminator
        - may contain several string terminators as regular characters
        - used as bitfields for various attributes (e.g. AnimationCurve)
        - no indicator of whether a given string is meant to be human readable or not
    */
    case 'S':
    {
        uint32_t slen = *((uint32_t*) &buf[idx]);
        idx += sizeof(slen);
        char *sval = malloc((slen+1));
        memcpy( sval, &buf[idx], slen);
        sval[slen] = '\0';
        // fprintf(node_summary,"\t%s\n", sval);
        FBX_Node_list_append(attr_list_head, sval, slen +1, 'S');
        free(sval);
        idx += slen;
    }
    break;

    // raw binary
    case 'R':
    default:
        exit(-1);
    break;  
    }
    *start_idx = idx;
}

static int visit_node(char *buf, int start_idx, FBX_Node **attr_list, FILE *node_summary, int *cur_id) {
    uint32_t *ibuf = (uint32_t*) &buf[start_idx];
    uint32_t end_offset = *ibuf++;
    uint32_t num_props = *ibuf++;
    uint32_t prop_list_len = *ibuf++;
    uint8_t  name_len = *((uint8_t*) ibuf++);
    int curr_pos = start_idx + 3*sizeof(uint32_t)+sizeof(uint8_t);
    
    char *name = malloc(name_len+1);
    memcpy( name,           &buf[start_idx+13], name_len);
    name[name_len] = '\0';  
    curr_pos += name_len;

    if ( string_in_list(name, RelevantNodes) ) {
        // TODO: replace w/ object name identifiers
        if ( strcmp(name, "Geometry") == 0 )  {
            (*cur_id)++;
        }

        // fprintf(node_summary, "\nNode '%s': bytes %d - %d\n", name, start_idx, end_offset);
        // fprintf(node_summary, "\tNumProperties: %d\n", num_props);
        // fprintf(node_summary, "\tPropertyListLen: %d\n", prop_list_len);

        ObjectAttribute new_attr;
        new_attr.attributes = NULL;
        new_attr.name = malloc(name_len+1);
        new_attr.id = *cur_id;
        strcpy(new_attr.name, name);

        for ( int i = 0; i < num_props; i++ ) {
            visit_prop(buf, &curr_pos, &new_attr.attributes, node_summary);
        }
        FBX_Node_list_append(attr_list, &new_attr, sizeof(new_attr), 'O');
        curr_pos -= prop_list_len;
    }

    free(name);
    curr_pos += prop_list_len;
    if ( curr_pos == end_offset )
        return curr_pos;

    // 13 byte NULL record indicates a nested node
    while ( curr_pos+13 <  end_offset ) {
        curr_pos = visit_node(buf, curr_pos, attr_list, node_summary, cur_id);
    }
    curr_pos+=13;
    return curr_pos;
}

static FBX_Node *FBX_load_file(const char *filename) {
    struct stat finfo;
    stat(filename, &finfo);
    printf("'%s' size: %ld bytes\n", filename, finfo.st_size); 
    
    int fd = open(filename, O_RDONLY);
    FILE *node_summary = fopen("test.txt", "w+");
    FBX_Node *attribute_list = NULL;
    if ( fd != -1 ) {
        char *fbuf = malloc(sizeof(char) * finfo.st_size);
        read(fd, fbuf, finfo.st_size);
        close(fd);
        
        // read header
        char header[20];
        memcpy( &header, &fbuf[0], 20);
        fprintf(node_summary, "%s\n", header);

        unsigned int version;
        memcpy( &version, &fbuf[23], 4);
        fprintf(node_summary, "Version: %d\n", version);

        // read node record
        int pos = 27;
        int cur_id = -1;
        while( pos < finfo.st_size-300 ) {
            pos = visit_node(fbuf, pos, &attribute_list, node_summary, &cur_id);
        }
        free(fbuf);
    }
    return attribute_list;
}

static Model FBX_list_to_Model(FBX_Node *attribute_list, FILE *log_file) {
    Model model = { 0 };
    
    model.meshCount = FBX_list_max_id(attribute_list) + 1;
    println(model.meshCount);
    model.meshes = RL_CALLOC(model.meshCount, sizeof(Mesh));
    
    model.materialCount = model.meshCount;
    model.materials = (Material *)RL_CALLOC(model.materialCount, sizeof(Material));
    model.meshMaterial = (int *)RL_CALLOC(model.meshCount, sizeof(int));

    for (int i = 0; i < model.meshCount; i++) {
        fprintf(log_file, "Mesh: %d", i);
        model.materials[i] = LoadMaterialDefault();

        // get vertices
        FBX_Node *v_node = FBX_list_search(attribute_list, "Vertices", i);
        assert(v_node->type == 'd'); 
        double *vertices = (double*) v_node->data;

        // get polygon indices
        FBX_Node *p_node = FBX_list_search(attribute_list, "PolygonVertexIndex", i);
        assert(p_node->type == 'i');
        int32_t *poly_vert_index = (int32_t*) p_node->data;

        FBX_Node *n_node = FBX_list_search(attribute_list, "Normals", i);
        assert(n_node->type == 'd');
        double *normals = (double*) n_node->data;

        FBX_Node *uv_node = FBX_list_search(attribute_list, "UV", i);
        assert(uv_node->type == 'd');
        double *uv = (double*) uv_node->data;

        model.meshes[i].vertexCount = v_node->size/(3*sizeof(double));
        model.meshes[i].vertices = RL_CALLOC(model.meshes[i].vertexCount*3, sizeof(float));
        model.meshes[i].normals = RL_CALLOC(model.meshes[i].vertexCount*3, sizeof(float));
        model.meshes[i].texcoords = RL_CALLOC(model.meshes[i].vertexCount*2, sizeof(float));
        model.meshes[i].boneIds = RL_CALLOC(model.meshes[i].vertexCount*4, sizeof(float));        // Up-to 4 bones supported!
        model.meshes[i].boneWeights = RL_CALLOC(model.meshes[i].vertexCount*4, sizeof(float));    // Up-to 4 bones supported!
        
        // Animated verted data, what we actually process for rendering
        // NOTE: Animated vertex should be re-uploaded to GPU (if not using GPU skinning)
        model.meshes[i].animVertices = RL_CALLOC(model.meshes[i].vertexCount*3, sizeof(float));
        model.meshes[i].animNormals = RL_CALLOC(model.meshes[i].vertexCount*3, sizeof(float));
       
#if FBX_DEBUG
        print_vertex_array(log_file, vertices, v_node->size);
        fprintf(log_file, "\n\tPolygonVertexIndex:");
        print_vertex_array(log_file, poly_vert_index, p_node->size);
        fprintf(log_file, "\n");
#endif
        
        // count faces
        int index_count = p_node->size/sizeof(int32_t);
        int face_count = 0;
        int curr_vert = 0;
        while ( curr_vert < index_count ) {
            int verts_in_face = 0;
            
            do {
                verts_in_face++;
            } while ( poly_vert_index[curr_vert++] >= 0 );
            curr_vert -= verts_in_face;
        
            float *face_verts = RL_CALLOC(verts_in_face*3, sizeof(float));
#if FBX_DEBUG
            fprintf(log_file, "\tFace: %d\n", face_count);
#endif
            for ( int j = 0; j < verts_in_face*3 ; j+=3 ) {
                bool end_vert = poly_vert_index[curr_vert] < 0;
                if ( end_vert ) {
                    poly_vert_index[curr_vert] = -poly_vert_index[curr_vert] - 1;
                    face_count++;
                }
                    
                face_verts[j] = vertices[3*poly_vert_index[curr_vert]];
                face_verts[j+1] = vertices[3*poly_vert_index[curr_vert]+1];
                face_verts[j+2] = vertices[3*poly_vert_index[curr_vert]+2];
#if FBX_DEBUG
                fprintf(log_file, "\t\t%d:\t", poly_vert_index[curr_vert]);
                fprintf(log_file, "%f, %f, %f\n", face_verts[j], face_verts[j+1], face_verts[j+2]);
#endif
                curr_vert++;
            }
            
        }
        fprintf(log_file, "\tTotal faces: %d\n\n", face_count);

        model.meshes[i].triangleCount = face_count;
        model.meshes[i].indices = RL_CALLOC(model.meshes[i].triangleCount*3, sizeof(unsigned short));

        // copy vertex data 
        // NOTE: loss of precision here!!
        for ( int j = 0; j < model.meshes[i].vertexCount*3; j++ ) {
            model.meshes[i].vertices[j] = (float) vertices[j];
            model.meshes[i].normals[j] = (float) normals[j];
        }
        for ( int j = 0; j < model.meshes[i].vertexCount*2; j++ ) {
            model.meshes[i].texcoords[j] = (float) uv[j];
        }
        for ( int j = 0; j < model.meshes[i].triangleCount*3; j++ ) {
            int idx = poly_vert_index[j];
            if ( idx < 0 ) idx = -idx-1;
            model.meshes[i].indices[j] = (unsigned short) idx;
        }

        model.meshes[i].vboId = (unsigned int *)RL_CALLOC(DEFAULT_MESH_VERTEX_BUFFERS, sizeof(unsigned int));

    }

    model.transform = MatrixIdentity();
    // Upload vertex data to GPU (static mesh)
    for (int i = 0; i < model.meshCount; i++) 
        rlLoadMesh(&model.meshes[i], false);
    if (model.materialCount == 0) {
            model.materialCount = 1;
        model.materials = (Material *)RL_CALLOC(model.materialCount, sizeof(Material));
        model.materials[0] = LoadMaterialDefault();

        if (model.meshMaterial == NULL) model.meshMaterial = (int *)RL_CALLOC(model.meshCount, sizeof(int));
    }
    return model;
}

#if FBX_DEBUG

static void FBX_list_print(FBX_Node *attribute_list, FILE *out) {
    // LSB of animation curve bitfields
    static const int64_t eInterpolationConstant = 0x00000002; 
    static const int64_t eInterpolationLinear = 0x00000004;    
    static const int64_t eInterpolationCubic = 0x00000008;  

    FBX_Node *iter = attribute_list;
    while ( iter != NULL) {
        assert(iter->type == 'O');
        ObjectAttribute *attr = (ObjectAttribute*)iter->data;
        fprintf(out, "\n\n%s: %d", attr->name, attr->id);
        FBX_Node *attr_iter = attr->attributes;
        while ( attr_iter != NULL ) {
            fprintf(out, "\n\tType: %c", attr_iter->type);
            switch (attr_iter->type) {
            case 'd':
            {
                double *view = (double*) attr_iter->data;
                print_vertex_array(out, view, attr_iter->size);
            }
            break;

            case 'f':
            {
                float *view = (float*) attr_iter->data;
                print_vertex_array(out, view, attr_iter->size);
            }
            break;
            
            case 'i':
            {
                int32_t *view = (int32_t*) attr_iter->data;
                print_vertex_array(out, view, attr_iter->size);
            }
            break;

            case 'l':
            {
                int64_t *view = (int64_t*) attr_iter->data;
                print_vertex_array(out, view, attr_iter->size);
            }
            break;

            // string
            case 'S':
            {
                fprintf(out, "\n\t%d", attr_iter->size);
                fprintf(out, "\n\t%s", attr_iter->data);
                int64_t data = *((int64_t*) attr_iter->data) << 32;
                if (attr_iter->size == 1 && (eInterpolationCubic & data)) {
                    fprintf(out, "\n\teInterpolationCubic");
                }
                fprintf(out, "\n\t%llx", *((int64_t*)attr_iter->data));
                fprintf(out, "\n\tshifted:%llx", data);
            }
            break;
            }
            attr_iter = attr_iter->next;
        }   
        iter = iter->next;
    }
}
#endif