#if !defined(_BTK_OPENGL_MACRO)
#define _BTK_OPENGL_MACRO

//Import GL const
#ifdef __gl2_h_
#error OpenGL ES 2 header already included, remove this include, glad already provides it
#endif
#define __gl2_h_

#ifdef __gl3_h_
#error OpenGL ES 3 header already included, remove this include, glad already provides it
#endif
#define __gl3_h_

#if defined(_WIN32) && !defined(APIENTRY) && !defined(__CYGWIN__) && !defined(__SCITECH_SNAP__)
#define APIENTRY __stdcall
#endif

#ifndef APIENTRY
#define APIENTRY
#endif
#ifndef APIENTRYP
#define APIENTRYP APIENTRY *
#endif

#ifndef GLAPIENTRY
#define GLAPIENTRY APIENTRY
#endif

#ifndef GLAPI
# if defined(GLAD_GLAPI_EXPORT)
#  if defined(_WIN32) || defined(__CYGWIN__)
#   if defined(GLAD_GLAPI_EXPORT_BUILD)
#    if defined(__GNUC__)
#     define GLAPI __attribute__ ((dllexport)) extern
#    else
#     define GLAPI __declspec(dllexport) extern
#    endif
#   else
#    if defined(__GNUC__)
#     define GLAPI __attribute__ ((dllimport)) extern
#    else
#     define GLAPI __declspec(dllimport) extern
#    endif
#   endif
#  elif defined(__GNUC__) && defined(GLAD_GLAPI_EXPORT_BUILD)
#   define GLAPI __attribute__ ((visibility ("default"))) extern
#  else
#   define GLAPI extern
#  endif
# else
#  define GLAPI extern
# endif
#endif

#ifndef __khrplatform_h_
#define __khrplatform_h_

/*
** Copyright (c) 2008-2018 The Khronos Group Inc.
**
** Permission is hereby granted, free of charge, to any person obtaining a
** copy of this software and/or associated documentation files (the
** "Materials"), to deal in the Materials without restriction, including
** without limitation the rights to use, copy, modify, merge, publish,
** distribute, sublicense, and/or sell copies of the Materials, and to
** permit persons to whom the Materials are furnished to do so, subject to
** the following conditions:
**
** The above copyright notice and this permission notice shall be included
** in all copies or substantial portions of the Materials.
**
** THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
** IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
** CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
** TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
** MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
*/

/* Khronos platform-specific types and definitions.
 *
 * The master copy of khrplatform.h is maintained in the Khronos EGL
 * Registry repository at https://github.com/KhronosGroup/EGL-Registry
 * The last semantic modification to khrplatform.h was at commit ID:
 *      67a3e0864c2d75ea5287b9f3d2eb74a745936692
 *
 * Adopters may modify this file to suit their platform. Adopters are
 * encouraged to submit platform specific modifications to the Khronos
 * group so that they can be included in future versions of this file.
 * Please submit changes by filing pull requests or issues on
 * the EGL Registry repository linked above.
 *
 *
 * See the Implementer's Guidelines for information about where this file
 * should be located on your system and for more details of its use:
 *    http://www.khronos.org/registry/implementers_guide.pdf
 *
 * This file should be included as
 *        #include <KHR/khrplatform.h>
 * by Khronos client API header files that use its types and defines.
 *
 * The types in khrplatform.h should only be used to define API-specific types.
 *
 * Types defined in khrplatform.h:
 *    khronos_int8_t              signed   8  bit
 *    khronos_uint8_t             unsigned 8  bit
 *    khronos_int16_t             signed   16 bit
 *    khronos_uint16_t            unsigned 16 bit
 *    khronos_int32_t             signed   32 bit
 *    khronos_uint32_t            unsigned 32 bit
 *    khronos_int64_t             signed   64 bit
 *    khronos_uint64_t            unsigned 64 bit
 *    khronos_intptr_t            signed   same number of bits as a pointer
 *    khronos_uintptr_t           unsigned same number of bits as a pointer
 *    khronos_ssize_t             signed   size
 *    khronos_usize_t             unsigned size
 *    khronos_float_t             signed   32 bit floating point
 *    khronos_time_ns_t           unsigned 64 bit time in nanoseconds
 *    khronos_utime_nanoseconds_t unsigned time interval or absolute time in
 *                                         nanoseconds
 *    khronos_stime_nanoseconds_t signed time interval in nanoseconds
 *    khronos_boolean_enum_t      enumerated boolean type. This should
 *      only be used as a base type when a client API's boolean type is
 *      an enum. Client APIs which use an integer or other type for
 *      booleans cannot use this as the base type for their boolean.
 *
 * Tokens defined in khrplatform.h:
 *
 *    KHRONOS_FALSE, KHRONOS_TRUE Enumerated boolean false/true values.
 *
 *    KHRONOS_SUPPORT_INT64 is 1 if 64 bit integers are supported; otherwise 0.
 *    KHRONOS_SUPPORT_FLOAT is 1 if floats are supported; otherwise 0.
 *
 * Calling convention macros defined in this file:
 *    KHRONOS_APICALL
 *    KHRONOS_APIENTRY
 *    KHRONOS_APIATTRIBUTES
 *
 * These may be used in function prototypes as:
 *
 *      KHRONOS_APICALL void KHRONOS_APIENTRY funcname(
 *                                  int arg1,
 *                                  int arg2) KHRONOS_APIATTRIBUTES;
 */

#if defined(__SCITECH_SNAP__) && !defined(KHRONOS_STATIC)
#   define KHRONOS_STATIC 1
#endif

/*-------------------------------------------------------------------------
 * Definition of KHRONOS_APICALL
 *-------------------------------------------------------------------------
 * This precedes the return type of the function in the function prototype.
 */
#if defined(KHRONOS_STATIC)
    /* If the preprocessor constant KHRONOS_STATIC is defined, make the
     * header compatible with static linking. */
#   define KHRONOS_APICALL
#elif defined(_WIN32)
#   define KHRONOS_APICALL __declspec(dllimport)
#elif defined (__SYMBIAN32__)
#   define KHRONOS_APICALL IMPORT_C
#elif defined(__ANDROID__)
#   define KHRONOS_APICALL __attribute__((visibility("default")))
#else
#   define KHRONOS_APICALL
#endif

/*-------------------------------------------------------------------------
 * Definition of KHRONOS_APIENTRY
 *-------------------------------------------------------------------------
 * This follows the return type of the function  and precedes the function
 * name in the function prototype.
 */
#if defined(_WIN32) && !defined(_WIN32_WCE) && !defined(__SCITECH_SNAP__)
    /* Win32 but not WinCE */
#   define KHRONOS_APIENTRY __stdcall
#else
#   define KHRONOS_APIENTRY
#endif

/*-------------------------------------------------------------------------
 * Definition of KHRONOS_APIATTRIBUTES
 *-------------------------------------------------------------------------
 * This follows the closing parenthesis of the function prototype arguments.
 */
#if defined (__ARMCC_2__)
#define KHRONOS_APIATTRIBUTES __softfp
#else
#define KHRONOS_APIATTRIBUTES
#endif

/*-------------------------------------------------------------------------
 * basic type definitions
 *-----------------------------------------------------------------------*/
#if (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L) || defined(__GNUC__) || defined(__SCO__) || defined(__USLC__)


/*
 * Using <stdint.h>
 */
#include <stdint.h>
typedef int32_t                 khronos_int32_t;
typedef uint32_t                khronos_uint32_t;
typedef int64_t                 khronos_int64_t;
typedef uint64_t                khronos_uint64_t;
#define KHRONOS_SUPPORT_INT64   1
#define KHRONOS_SUPPORT_FLOAT   1

#elif defined(__VMS ) || defined(__sgi)

/*
 * Using <inttypes.h>
 */
#include <inttypes.h>
typedef int32_t                 khronos_int32_t;
typedef uint32_t                khronos_uint32_t;
typedef int64_t                 khronos_int64_t;
typedef uint64_t                khronos_uint64_t;
#define KHRONOS_SUPPORT_INT64   1
#define KHRONOS_SUPPORT_FLOAT   1

#elif defined(_WIN32) && !defined(__SCITECH_SNAP__)

/*
 * Win32
 */
typedef __int32                 khronos_int32_t;
typedef unsigned __int32        khronos_uint32_t;
typedef __int64                 khronos_int64_t;
typedef unsigned __int64        khronos_uint64_t;
#define KHRONOS_SUPPORT_INT64   1
#define KHRONOS_SUPPORT_FLOAT   1

#elif defined(__sun__) || defined(__digital__)

/*
 * Sun or Digital
 */
typedef int                     khronos_int32_t;
typedef unsigned int            khronos_uint32_t;
#if defined(__arch64__) || defined(_LP64)
typedef long int                khronos_int64_t;
typedef unsigned long int       khronos_uint64_t;
#else
typedef long long int           khronos_int64_t;
typedef unsigned long long int  khronos_uint64_t;
#endif /* __arch64__ */
#define KHRONOS_SUPPORT_INT64   1
#define KHRONOS_SUPPORT_FLOAT   1

#elif 0

/*
 * Hypothetical platform with no float or int64 support
 */
typedef int                     khronos_int32_t;
typedef unsigned int            khronos_uint32_t;
#define KHRONOS_SUPPORT_INT64   0
#define KHRONOS_SUPPORT_FLOAT   0

#else

/*
 * Generic fallback
 */
#include <stdint.h>
typedef int32_t                 khronos_int32_t;
typedef uint32_t                khronos_uint32_t;
typedef int64_t                 khronos_int64_t;
typedef uint64_t                khronos_uint64_t;
#define KHRONOS_SUPPORT_INT64   1
#define KHRONOS_SUPPORT_FLOAT   1

#endif


/*
 * Types that are (so far) the same on all platforms
 */
typedef signed   char          khronos_int8_t;
typedef unsigned char          khronos_uint8_t;
typedef signed   short int     khronos_int16_t;
typedef unsigned short int     khronos_uint16_t;

/*
 * Types that differ between LLP64 and LP64 architectures - in LLP64,
 * pointers are 64 bits, but 'long' is still 32 bits. Win64 appears
 * to be the only LLP64 architecture in current use.
 */
#ifdef _WIN64
typedef signed   long long int khronos_intptr_t;
typedef unsigned long long int khronos_uintptr_t;
typedef signed   long long int khronos_ssize_t;
typedef unsigned long long int khronos_usize_t;
#else
typedef signed   long  int     khronos_intptr_t;
typedef unsigned long  int     khronos_uintptr_t;
typedef signed   long  int     khronos_ssize_t;
typedef unsigned long  int     khronos_usize_t;
#endif

#if KHRONOS_SUPPORT_FLOAT
/*
 * Float type
 */
typedef          float         khronos_float_t;
#endif

#if KHRONOS_SUPPORT_INT64
/* Time types
 *
 * These types can be used to represent a time interval in nanoseconds or
 * an absolute Unadjusted System Time.  Unadjusted System Time is the number
 * of nanoseconds since some arbitrary system event (e.g. since the last
 * time the system booted).  The Unadjusted System Time is an unsigned
 * 64 bit value that wraps back to 0 every 584 years.  Time intervals
 * may be either signed or unsigned.
 */
typedef khronos_uint64_t       khronos_utime_nanoseconds_t;
typedef khronos_int64_t        khronos_stime_nanoseconds_t;
#endif

/*
 * Dummy value used to pad enum types to 32 bits.
 */
#ifndef KHRONOS_MAX_ENUM
#define KHRONOS_MAX_ENUM 0x7FFFFFFF
#endif

/*
 * Enumerated boolean type
 *
 * Values other than zero should be considered to be true.  Therefore
 * comparisons should not be made against KHRONOS_TRUE.
 */
typedef enum {
    KHRONOS_FALSE = 0,
    KHRONOS_TRUE  = 1,
    KHRONOS_BOOLEAN_ENUM_FORCE_SIZE = KHRONOS_MAX_ENUM
} khronos_boolean_enum_t;

#endif /* __khrplatform_h_ */

typedef unsigned int GLenum;
typedef unsigned char GLboolean;
typedef unsigned int GLbitfield;
typedef void GLvoid;
typedef khronos_int8_t GLbyte;
typedef khronos_uint8_t GLubyte;
typedef khronos_int16_t GLshort;
typedef khronos_uint16_t GLushort;
typedef int GLint;
typedef unsigned int GLuint;
typedef khronos_int32_t GLclampx;
typedef int GLsizei;
typedef khronos_float_t GLfloat;
typedef khronos_float_t GLclampf;
typedef double GLdouble;
typedef double GLclampd;
typedef void *GLeglClientBufferEXT;
typedef void *GLeglImageOES;
typedef char GLchar;
typedef char GLcharARB;
#ifdef __APPLE__
typedef void *GLhandleARB;
#else
typedef unsigned int GLhandleARB;
#endif
typedef khronos_uint16_t GLhalf;
typedef khronos_uint16_t GLhalfARB;
typedef khronos_int32_t GLfixed;
typedef khronos_intptr_t GLintptr;
typedef khronos_intptr_t GLintptrARB;
typedef khronos_ssize_t GLsizeiptr;
typedef khronos_ssize_t GLsizeiptrARB;
typedef khronos_int64_t GLint64;
typedef khronos_int64_t GLint64EXT;
typedef khronos_uint64_t GLuint64;
typedef khronos_uint64_t GLuint64EXT;
typedef struct __GLsync *GLsync;
struct _cl_context;
struct _cl_event;
typedef void (APIENTRY *GLDEBUGPROC)(GLenum source,GLenum type,GLuint id,GLenum severity,GLsizei length,const GLchar *message,const void *userParam);
typedef void (APIENTRY *GLDEBUGPROCARB)(GLenum source,GLenum type,GLuint id,GLenum severity,GLsizei length,const GLchar *message,const void *userParam);
typedef void (APIENTRY *GLDEBUGPROCKHR)(GLenum source,GLenum type,GLuint id,GLenum severity,GLsizei length,const GLchar *message,const void *userParam);
typedef void (APIENTRY *GLDEBUGPROCAMD)(GLuint id,GLenum category,GLenum severity,GLsizei length,const GLchar *message,void *userParam);
typedef unsigned short GLhalfNV;
typedef GLintptr GLvdpauSurfaceNV;
typedef void (APIENTRY *GLVULKANPROCNV)(void);
#define GL_DEPTH_BUFFER_BIT 0x00000100
#define GL_STENCIL_BUFFER_BIT 0x00000400
#define GL_COLOR_BUFFER_BIT 0x00004000
#define GL_FALSE 0
#define GL_TRUE 1
#define GL_POINTS 0x0000
#define GL_LINES 0x0001
#define GL_LINE_LOOP 0x0002
#define GL_LINE_STRIP 0x0003
#define GL_TRIANGLES 0x0004
#define GL_TRIANGLE_STRIP 0x0005
#define GL_TRIANGLE_FAN 0x0006
#define GL_ZERO 0
#define GL_ONE 1
#define GL_SRC_COLOR 0x0300
#define GL_ONE_MINUS_SRC_COLOR 0x0301
#define GL_SRC_ALPHA 0x0302
#define GL_ONE_MINUS_SRC_ALPHA 0x0303
#define GL_DST_ALPHA 0x0304
#define GL_ONE_MINUS_DST_ALPHA 0x0305
#define GL_DST_COLOR 0x0306
#define GL_ONE_MINUS_DST_COLOR 0x0307
#define GL_SRC_ALPHA_SATURATE 0x0308
#define GL_FUNC_ADD 0x8006
#define GL_BLEND_EQUATION 0x8009
#define GL_BLEND_EQUATION_RGB 0x8009
#define GL_BLEND_EQUATION_ALPHA 0x883D
#define GL_FUNC_SUBTRACT 0x800A
#define GL_FUNC_REVERSE_SUBTRACT 0x800B
#define GL_BLEND_DST_RGB 0x80C8
#define GL_BLEND_SRC_RGB 0x80C9
#define GL_BLEND_DST_ALPHA 0x80CA
#define GL_BLEND_SRC_ALPHA 0x80CB
#define GL_CONSTANT_COLOR 0x8001
#define GL_ONE_MINUS_CONSTANT_COLOR 0x8002
#define GL_CONSTANT_ALPHA 0x8003
#define GL_ONE_MINUS_CONSTANT_ALPHA 0x8004
#define GL_BLEND_COLOR 0x8005
#define GL_ARRAY_BUFFER 0x8892
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#define GL_ARRAY_BUFFER_BINDING 0x8894
#define GL_ELEMENT_ARRAY_BUFFER_BINDING 0x8895
#define GL_STREAM_DRAW 0x88E0
#define GL_STATIC_DRAW 0x88E4
#define GL_DYNAMIC_DRAW 0x88E8
#define GL_BUFFER_SIZE 0x8764
#define GL_BUFFER_USAGE 0x8765
#define GL_CURRENT_VERTEX_ATTRIB 0x8626
#define GL_FRONT 0x0404
#define GL_BACK 0x0405
#define GL_FRONT_AND_BACK 0x0408
#define GL_TEXTURE_2D 0x0DE1
#define GL_CULL_FACE 0x0B44
#define GL_BLEND 0x0BE2
#define GL_DITHER 0x0BD0
#define GL_STENCIL_TEST 0x0B90
#define GL_DEPTH_TEST 0x0B71
#define GL_SCISSOR_TEST 0x0C11
#define GL_POLYGON_OFFSET_FILL 0x8037
#define GL_SAMPLE_ALPHA_TO_COVERAGE 0x809E
#define GL_SAMPLE_COVERAGE 0x80A0
#define GL_NO_ERROR 0
#define GL_INVALID_ENUM 0x0500
#define GL_INVALID_VALUE 0x0501
#define GL_INVALID_OPERATION 0x0502
#define GL_OUT_OF_MEMORY 0x0505
#define GL_CW 0x0900
#define GL_CCW 0x0901
#define GL_LINE_WIDTH 0x0B21
#define GL_ALIASED_POINT_SIZE_RANGE 0x846D
#define GL_ALIASED_LINE_WIDTH_RANGE 0x846E
#define GL_CULL_FACE_MODE 0x0B45
#define GL_FRONT_FACE 0x0B46
#define GL_DEPTH_RANGE 0x0B70
#define GL_DEPTH_WRITEMASK 0x0B72
#define GL_DEPTH_CLEAR_VALUE 0x0B73
#define GL_DEPTH_FUNC 0x0B74
#define GL_STENCIL_CLEAR_VALUE 0x0B91
#define GL_STENCIL_FUNC 0x0B92
#define GL_STENCIL_FAIL 0x0B94
#define GL_STENCIL_PASS_DEPTH_FAIL 0x0B95
#define GL_STENCIL_PASS_DEPTH_PASS 0x0B96
#define GL_STENCIL_REF 0x0B97
#define GL_STENCIL_VALUE_MASK 0x0B93
#define GL_STENCIL_WRITEMASK 0x0B98
#define GL_STENCIL_BACK_FUNC 0x8800
#define GL_STENCIL_BACK_FAIL 0x8801
#define GL_STENCIL_BACK_PASS_DEPTH_FAIL 0x8802
#define GL_STENCIL_BACK_PASS_DEPTH_PASS 0x8803
#define GL_STENCIL_BACK_REF 0x8CA3
#define GL_STENCIL_BACK_VALUE_MASK 0x8CA4
#define GL_STENCIL_BACK_WRITEMASK 0x8CA5
#define GL_VIEWPORT 0x0BA2
#define GL_SCISSOR_BOX 0x0C10
#define GL_COLOR_CLEAR_VALUE 0x0C22
#define GL_COLOR_WRITEMASK 0x0C23
#define GL_UNPACK_ALIGNMENT 0x0CF5
#define GL_PACK_ALIGNMENT 0x0D05
#define GL_MAX_TEXTURE_SIZE 0x0D33
#define GL_MAX_VIEWPORT_DIMS 0x0D3A
#define GL_SUBPIXEL_BITS 0x0D50
#define GL_RED_BITS 0x0D52
#define GL_GREEN_BITS 0x0D53
#define GL_BLUE_BITS 0x0D54
#define GL_ALPHA_BITS 0x0D55
#define GL_DEPTH_BITS 0x0D56
#define GL_STENCIL_BITS 0x0D57
#define GL_POLYGON_OFFSET_UNITS 0x2A00
#define GL_POLYGON_OFFSET_FACTOR 0x8038
#define GL_TEXTURE_BINDING_2D 0x8069
#define GL_SAMPLE_BUFFERS 0x80A8
#define GL_SAMPLES 0x80A9
#define GL_SAMPLE_COVERAGE_VALUE 0x80AA
#define GL_SAMPLE_COVERAGE_INVERT 0x80AB
#define GL_NUM_COMPRESSED_TEXTURE_FORMATS 0x86A2
#define GL_COMPRESSED_TEXTURE_FORMATS 0x86A3
#define GL_DONT_CARE 0x1100
#define GL_FASTEST 0x1101
#define GL_NICEST 0x1102
#define GL_GENERATE_MIPMAP_HINT 0x8192
#define GL_BYTE 0x1400
#define GL_UNSIGNED_BYTE 0x1401
#define GL_SHORT 0x1402
#define GL_UNSIGNED_SHORT 0x1403
#define GL_INT 0x1404
#define GL_UNSIGNED_INT 0x1405
#define GL_FLOAT 0x1406
#define GL_FIXED 0x140C
#define GL_DEPTH_COMPONENT 0x1902
#define GL_ALPHA 0x1906
#define GL_RGB 0x1907
#define GL_RGBA 0x1908
#define GL_LUMINANCE 0x1909
#define GL_LUMINANCE_ALPHA 0x190A
#define GL_UNSIGNED_SHORT_4_4_4_4 0x8033
#define GL_UNSIGNED_SHORT_5_5_5_1 0x8034
#define GL_UNSIGNED_SHORT_5_6_5 0x8363
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER 0x8B31
#define GL_MAX_VERTEX_ATTRIBS 0x8869
#define GL_MAX_VERTEX_UNIFORM_VECTORS 0x8DFB
#define GL_MAX_VARYING_VECTORS 0x8DFC
#define GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS 0x8B4D
#define GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS 0x8B4C
#define GL_MAX_TEXTURE_IMAGE_UNITS 0x8872
#define GL_MAX_FRAGMENT_UNIFORM_VECTORS 0x8DFD
#define GL_SHADER_TYPE 0x8B4F
#define GL_DELETE_STATUS 0x8B80
#define GL_LINK_STATUS 0x8B82
#define GL_VALIDATE_STATUS 0x8B83
#define GL_ATTACHED_SHADERS 0x8B85
#define GL_ACTIVE_UNIFORMS 0x8B86
#define GL_ACTIVE_UNIFORM_MAX_LENGTH 0x8B87
#define GL_ACTIVE_ATTRIBUTES 0x8B89
#define GL_ACTIVE_ATTRIBUTE_MAX_LENGTH 0x8B8A
#define GL_SHADING_LANGUAGE_VERSION 0x8B8C
#define GL_CURRENT_PROGRAM 0x8B8D
#define GL_NEVER 0x0200
#define GL_LESS 0x0201
#define GL_EQUAL 0x0202
#define GL_LEQUAL 0x0203
#define GL_GREATER 0x0204
#define GL_NOTEQUAL 0x0205
#define GL_GEQUAL 0x0206
#define GL_ALWAYS 0x0207
#define GL_KEEP 0x1E00
#define GL_REPLACE 0x1E01
#define GL_INCR 0x1E02
#define GL_DECR 0x1E03
#define GL_INVERT 0x150A
#define GL_INCR_WRAP 0x8507
#define GL_DECR_WRAP 0x8508
#define GL_VENDOR 0x1F00
#define GL_RENDERER 0x1F01
#define GL_VERSION 0x1F02
#define GL_EXTENSIONS 0x1F03
#define GL_NEAREST 0x2600
#define GL_LINEAR 0x2601
#define GL_NEAREST_MIPMAP_NEAREST 0x2700
#define GL_LINEAR_MIPMAP_NEAREST 0x2701
#define GL_NEAREST_MIPMAP_LINEAR 0x2702
#define GL_LINEAR_MIPMAP_LINEAR 0x2703
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_TEXTURE_WRAP_S 0x2802
#define GL_TEXTURE_WRAP_T 0x2803
#define GL_TEXTURE 0x1702
#define GL_TEXTURE_CUBE_MAP 0x8513
#define GL_TEXTURE_BINDING_CUBE_MAP 0x8514
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X 0x8515
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X 0x8516
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y 0x8517
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y 0x8518
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z 0x8519
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z 0x851A
#define GL_MAX_CUBE_MAP_TEXTURE_SIZE 0x851C
#define GL_TEXTURE0 0x84C0
#define GL_TEXTURE1 0x84C1
#define GL_TEXTURE2 0x84C2
#define GL_TEXTURE3 0x84C3
#define GL_TEXTURE4 0x84C4
#define GL_TEXTURE5 0x84C5
#define GL_TEXTURE6 0x84C6
#define GL_TEXTURE7 0x84C7
#define GL_TEXTURE8 0x84C8
#define GL_TEXTURE9 0x84C9
#define GL_TEXTURE10 0x84CA
#define GL_TEXTURE11 0x84CB
#define GL_TEXTURE12 0x84CC
#define GL_TEXTURE13 0x84CD
#define GL_TEXTURE14 0x84CE
#define GL_TEXTURE15 0x84CF
#define GL_TEXTURE16 0x84D0
#define GL_TEXTURE17 0x84D1
#define GL_TEXTURE18 0x84D2
#define GL_TEXTURE19 0x84D3
#define GL_TEXTURE20 0x84D4
#define GL_TEXTURE21 0x84D5
#define GL_TEXTURE22 0x84D6
#define GL_TEXTURE23 0x84D7
#define GL_TEXTURE24 0x84D8
#define GL_TEXTURE25 0x84D9
#define GL_TEXTURE26 0x84DA
#define GL_TEXTURE27 0x84DB
#define GL_TEXTURE28 0x84DC
#define GL_TEXTURE29 0x84DD
#define GL_TEXTURE30 0x84DE
#define GL_TEXTURE31 0x84DF
#define GL_ACTIVE_TEXTURE 0x84E0
#define GL_REPEAT 0x2901
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_MIRRORED_REPEAT 0x8370
#define GL_FLOAT_VEC2 0x8B50
#define GL_FLOAT_VEC3 0x8B51
#define GL_FLOAT_VEC4 0x8B52
#define GL_INT_VEC2 0x8B53
#define GL_INT_VEC3 0x8B54
#define GL_INT_VEC4 0x8B55
#define GL_BOOL 0x8B56
#define GL_BOOL_VEC2 0x8B57
#define GL_BOOL_VEC3 0x8B58
#define GL_BOOL_VEC4 0x8B59
#define GL_FLOAT_MAT2 0x8B5A
#define GL_FLOAT_MAT3 0x8B5B
#define GL_FLOAT_MAT4 0x8B5C
#define GL_SAMPLER_2D 0x8B5E
#define GL_SAMPLER_CUBE 0x8B60
#define GL_VERTEX_ATTRIB_ARRAY_ENABLED 0x8622
#define GL_VERTEX_ATTRIB_ARRAY_SIZE 0x8623
#define GL_VERTEX_ATTRIB_ARRAY_STRIDE 0x8624
#define GL_VERTEX_ATTRIB_ARRAY_TYPE 0x8625
#define GL_VERTEX_ATTRIB_ARRAY_NORMALIZED 0x886A
#define GL_VERTEX_ATTRIB_ARRAY_POINTER 0x8645
#define GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING 0x889F
#define GL_IMPLEMENTATION_COLOR_READ_TYPE 0x8B9A
#define GL_IMPLEMENTATION_COLOR_READ_FORMAT 0x8B9B
#define GL_COMPILE_STATUS 0x8B81
#define GL_INFO_LOG_LENGTH 0x8B84
#define GL_SHADER_SOURCE_LENGTH 0x8B88
#define GL_SHADER_COMPILER 0x8DFA
#define GL_SHADER_BINARY_FORMATS 0x8DF8
#define GL_NUM_SHADER_BINARY_FORMATS 0x8DF9
#define GL_LOW_FLOAT 0x8DF0
#define GL_MEDIUM_FLOAT 0x8DF1
#define GL_HIGH_FLOAT 0x8DF2
#define GL_LOW_INT 0x8DF3
#define GL_MEDIUM_INT 0x8DF4
#define GL_HIGH_INT 0x8DF5
#define GL_FRAMEBUFFER 0x8D40
#define GL_RENDERBUFFER 0x8D41
#define GL_RGBA4 0x8056
#define GL_RGB5_A1 0x8057
#define GL_RGB565 0x8D62
#define GL_DEPTH_COMPONENT16 0x81A5
#define GL_STENCIL_INDEX8 0x8D48
#define GL_RENDERBUFFER_WIDTH 0x8D42
#define GL_RENDERBUFFER_HEIGHT 0x8D43
#define GL_RENDERBUFFER_INTERNAL_FORMAT 0x8D44
#define GL_RENDERBUFFER_RED_SIZE 0x8D50
#define GL_RENDERBUFFER_GREEN_SIZE 0x8D51
#define GL_RENDERBUFFER_BLUE_SIZE 0x8D52
#define GL_RENDERBUFFER_ALPHA_SIZE 0x8D53
#define GL_RENDERBUFFER_DEPTH_SIZE 0x8D54
#define GL_RENDERBUFFER_STENCIL_SIZE 0x8D55
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE 0x8CD0
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME 0x8CD1
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL 0x8CD2
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE 0x8CD3
#define GL_COLOR_ATTACHMENT0 0x8CE0
#define GL_DEPTH_ATTACHMENT 0x8D00
#define GL_STENCIL_ATTACHMENT 0x8D20
#define GL_NONE 0
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
#define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT 0x8CD6
#define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT 0x8CD7
#define GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS 0x8CD9
#define GL_FRAMEBUFFER_UNSUPPORTED 0x8CDD
#define GL_FRAMEBUFFER_BINDING 0x8CA6
#define GL_RENDERBUFFER_BINDING 0x8CA7
#define GL_MAX_RENDERBUFFER_SIZE 0x84E8
#define GL_INVALID_FRAMEBUFFER_OPERATION 0x0506
#define GL_READ_BUFFER 0x0C02
#define GL_UNPACK_ROW_LENGTH 0x0CF2
#define GL_UNPACK_SKIP_ROWS 0x0CF3
#define GL_UNPACK_SKIP_PIXELS 0x0CF4
#define GL_PACK_ROW_LENGTH 0x0D02
#define GL_PACK_SKIP_ROWS 0x0D03
#define GL_PACK_SKIP_PIXELS 0x0D04
#define GL_COLOR 0x1800
#define GL_DEPTH 0x1801
#define GL_STENCIL 0x1802
#define GL_RED 0x1903
#define GL_RGB8 0x8051
#define GL_RGBA8 0x8058
#define GL_RGB10_A2 0x8059
#define GL_TEXTURE_BINDING_3D 0x806A
#define GL_UNPACK_SKIP_IMAGES 0x806D
#define GL_UNPACK_IMAGE_HEIGHT 0x806E
#define GL_TEXTURE_3D 0x806F
#define GL_TEXTURE_WRAP_R 0x8072
#define GL_MAX_3D_TEXTURE_SIZE 0x8073
#define GL_UNSIGNED_INT_2_10_10_10_REV 0x8368
#define GL_MAX_ELEMENTS_VERTICES 0x80E8
#define GL_MAX_ELEMENTS_INDICES 0x80E9
#define GL_TEXTURE_MIN_LOD 0x813A
#define GL_TEXTURE_MAX_LOD 0x813B
#define GL_TEXTURE_BASE_LEVEL 0x813C
#define GL_TEXTURE_MAX_LEVEL 0x813D
#define GL_MIN 0x8007
#define GL_MAX 0x8008
#define GL_DEPTH_COMPONENT24 0x81A6
#define GL_MAX_TEXTURE_LOD_BIAS 0x84FD
#define GL_TEXTURE_COMPARE_MODE 0x884C
#define GL_TEXTURE_COMPARE_FUNC 0x884D
#define GL_CURRENT_QUERY 0x8865
#define GL_QUERY_RESULT 0x8866
#define GL_QUERY_RESULT_AVAILABLE 0x8867
#define GL_BUFFER_MAPPED 0x88BC
#define GL_BUFFER_MAP_POINTER 0x88BD
#define GL_STREAM_READ 0x88E1
#define GL_STREAM_COPY 0x88E2
#define GL_STATIC_READ 0x88E5
#define GL_STATIC_COPY 0x88E6
#define GL_DYNAMIC_READ 0x88E9
#define GL_DYNAMIC_COPY 0x88EA
#define GL_MAX_DRAW_BUFFERS 0x8824
#define GL_DRAW_BUFFER0 0x8825
#define GL_DRAW_BUFFER1 0x8826
#define GL_DRAW_BUFFER2 0x8827
#define GL_DRAW_BUFFER3 0x8828
#define GL_DRAW_BUFFER4 0x8829
#define GL_DRAW_BUFFER5 0x882A
#define GL_DRAW_BUFFER6 0x882B
#define GL_DRAW_BUFFER7 0x882C
#define GL_DRAW_BUFFER8 0x882D
#define GL_DRAW_BUFFER9 0x882E
#define GL_DRAW_BUFFER10 0x882F
#define GL_DRAW_BUFFER11 0x8830
#define GL_DRAW_BUFFER12 0x8831
#define GL_DRAW_BUFFER13 0x8832
#define GL_DRAW_BUFFER14 0x8833
#define GL_DRAW_BUFFER15 0x8834
#define GL_MAX_FRAGMENT_UNIFORM_COMPONENTS 0x8B49
#define GL_MAX_VERTEX_UNIFORM_COMPONENTS 0x8B4A
#define GL_SAMPLER_3D 0x8B5F
#define GL_SAMPLER_2D_SHADOW 0x8B62
#define GL_FRAGMENT_SHADER_DERIVATIVE_HINT 0x8B8B
#define GL_PIXEL_PACK_BUFFER 0x88EB
#define GL_PIXEL_UNPACK_BUFFER 0x88EC
#define GL_PIXEL_PACK_BUFFER_BINDING 0x88ED
#define GL_PIXEL_UNPACK_BUFFER_BINDING 0x88EF
#define GL_FLOAT_MAT2x3 0x8B65
#define GL_FLOAT_MAT2x4 0x8B66
#define GL_FLOAT_MAT3x2 0x8B67
#define GL_FLOAT_MAT3x4 0x8B68
#define GL_FLOAT_MAT4x2 0x8B69
#define GL_FLOAT_MAT4x3 0x8B6A
#define GL_SRGB 0x8C40
#define GL_SRGB8 0x8C41
#define GL_SRGB8_ALPHA8 0x8C43
#define GL_COMPARE_REF_TO_TEXTURE 0x884E
#define GL_MAJOR_VERSION 0x821B
#define GL_MINOR_VERSION 0x821C
#define GL_NUM_EXTENSIONS 0x821D
#define GL_RGBA32F 0x8814
#define GL_RGB32F 0x8815
#define GL_RGBA16F 0x881A
#define GL_RGB16F 0x881B
#define GL_VERTEX_ATTRIB_ARRAY_INTEGER 0x88FD
#define GL_MAX_ARRAY_TEXTURE_LAYERS 0x88FF
#define GL_MIN_PROGRAM_TEXEL_OFFSET 0x8904
#define GL_MAX_PROGRAM_TEXEL_OFFSET 0x8905
#define GL_MAX_VARYING_COMPONENTS 0x8B4B
#define GL_TEXTURE_2D_ARRAY 0x8C1A
#define GL_TEXTURE_BINDING_2D_ARRAY 0x8C1D
#define GL_R11F_G11F_B10F 0x8C3A
#define GL_UNSIGNED_INT_10F_11F_11F_REV 0x8C3B
#define GL_RGB9_E5 0x8C3D
#define GL_UNSIGNED_INT_5_9_9_9_REV 0x8C3E
#define GL_TRANSFORM_FEEDBACK_VARYING_MAX_LENGTH 0x8C76
#define GL_TRANSFORM_FEEDBACK_BUFFER_MODE 0x8C7F
#define GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS 0x8C80
#define GL_TRANSFORM_FEEDBACK_VARYINGS 0x8C83
#define GL_TRANSFORM_FEEDBACK_BUFFER_START 0x8C84
#define GL_TRANSFORM_FEEDBACK_BUFFER_SIZE 0x8C85
#define GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN 0x8C88
#define GL_RASTERIZER_DISCARD 0x8C89
#define GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS 0x8C8A
#define GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS 0x8C8B
#define GL_INTERLEAVED_ATTRIBS 0x8C8C
#define GL_SEPARATE_ATTRIBS 0x8C8D
#define GL_TRANSFORM_FEEDBACK_BUFFER 0x8C8E
#define GL_TRANSFORM_FEEDBACK_BUFFER_BINDING 0x8C8F
#define GL_RGBA32UI 0x8D70
#define GL_RGB32UI 0x8D71
#define GL_RGBA16UI 0x8D76
#define GL_RGB16UI 0x8D77
#define GL_RGBA8UI 0x8D7C
#define GL_RGB8UI 0x8D7D
#define GL_RGBA32I 0x8D82
#define GL_RGB32I 0x8D83
#define GL_RGBA16I 0x8D88
#define GL_RGB16I 0x8D89
#define GL_RGBA8I 0x8D8E
#define GL_RGB8I 0x8D8F
#define GL_RED_INTEGER 0x8D94
#define GL_RGB_INTEGER 0x8D98
#define GL_RGBA_INTEGER 0x8D99
#define GL_SAMPLER_2D_ARRAY 0x8DC1
#define GL_SAMPLER_2D_ARRAY_SHADOW 0x8DC4
#define GL_SAMPLER_CUBE_SHADOW 0x8DC5
#define GL_UNSIGNED_INT_VEC2 0x8DC6
#define GL_UNSIGNED_INT_VEC3 0x8DC7
#define GL_UNSIGNED_INT_VEC4 0x8DC8
#define GL_INT_SAMPLER_2D 0x8DCA
#define GL_INT_SAMPLER_3D 0x8DCB
#define GL_INT_SAMPLER_CUBE 0x8DCC
#define GL_INT_SAMPLER_2D_ARRAY 0x8DCF
#define GL_UNSIGNED_INT_SAMPLER_2D 0x8DD2
#define GL_UNSIGNED_INT_SAMPLER_3D 0x8DD3
#define GL_UNSIGNED_INT_SAMPLER_CUBE 0x8DD4
#define GL_UNSIGNED_INT_SAMPLER_2D_ARRAY 0x8DD7
#define GL_BUFFER_ACCESS_FLAGS 0x911F
#define GL_BUFFER_MAP_LENGTH 0x9120
#define GL_BUFFER_MAP_OFFSET 0x9121
#define GL_DEPTH_COMPONENT32F 0x8CAC
#define GL_DEPTH32F_STENCIL8 0x8CAD
#define GL_FLOAT_32_UNSIGNED_INT_24_8_REV 0x8DAD
#define GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING 0x8210
#define GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE 0x8211
#define GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE 0x8212
#define GL_FRAMEBUFFER_ATTACHMENT_GREEN_SIZE 0x8213
#define GL_FRAMEBUFFER_ATTACHMENT_BLUE_SIZE 0x8214
#define GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE 0x8215
#define GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE 0x8216
#define GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE 0x8217
#define GL_FRAMEBUFFER_DEFAULT 0x8218
#define GL_FRAMEBUFFER_UNDEFINED 0x8219
#define GL_DEPTH_STENCIL_ATTACHMENT 0x821A
#define GL_DEPTH_STENCIL 0x84F9
#define GL_UNSIGNED_INT_24_8 0x84FA
#define GL_DEPTH24_STENCIL8 0x88F0
#define GL_UNSIGNED_NORMALIZED 0x8C17
#define GL_DRAW_FRAMEBUFFER_BINDING 0x8CA6
#define GL_READ_FRAMEBUFFER 0x8CA8
#define GL_DRAW_FRAMEBUFFER 0x8CA9
#define GL_READ_FRAMEBUFFER_BINDING 0x8CAA
#define GL_RENDERBUFFER_SAMPLES 0x8CAB
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LAYER 0x8CD4
#define GL_MAX_COLOR_ATTACHMENTS 0x8CDF
#define GL_COLOR_ATTACHMENT1 0x8CE1
#define GL_COLOR_ATTACHMENT2 0x8CE2
#define GL_COLOR_ATTACHMENT3 0x8CE3
#define GL_COLOR_ATTACHMENT4 0x8CE4
#define GL_COLOR_ATTACHMENT5 0x8CE5
#define GL_COLOR_ATTACHMENT6 0x8CE6
#define GL_COLOR_ATTACHMENT7 0x8CE7
#define GL_COLOR_ATTACHMENT8 0x8CE8
#define GL_COLOR_ATTACHMENT9 0x8CE9
#define GL_COLOR_ATTACHMENT10 0x8CEA
#define GL_COLOR_ATTACHMENT11 0x8CEB
#define GL_COLOR_ATTACHMENT12 0x8CEC
#define GL_COLOR_ATTACHMENT13 0x8CED
#define GL_COLOR_ATTACHMENT14 0x8CEE
#define GL_COLOR_ATTACHMENT15 0x8CEF
#define GL_COLOR_ATTACHMENT16 0x8CF0
#define GL_COLOR_ATTACHMENT17 0x8CF1
#define GL_COLOR_ATTACHMENT18 0x8CF2
#define GL_COLOR_ATTACHMENT19 0x8CF3
#define GL_COLOR_ATTACHMENT20 0x8CF4
#define GL_COLOR_ATTACHMENT21 0x8CF5
#define GL_COLOR_ATTACHMENT22 0x8CF6
#define GL_COLOR_ATTACHMENT23 0x8CF7
#define GL_COLOR_ATTACHMENT24 0x8CF8
#define GL_COLOR_ATTACHMENT25 0x8CF9
#define GL_COLOR_ATTACHMENT26 0x8CFA
#define GL_COLOR_ATTACHMENT27 0x8CFB
#define GL_COLOR_ATTACHMENT28 0x8CFC
#define GL_COLOR_ATTACHMENT29 0x8CFD
#define GL_COLOR_ATTACHMENT30 0x8CFE
#define GL_COLOR_ATTACHMENT31 0x8CFF
#define GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE 0x8D56
#define GL_MAX_SAMPLES 0x8D57
#define GL_HALF_FLOAT 0x140B
#define GL_MAP_READ_BIT 0x0001
#define GL_MAP_WRITE_BIT 0x0002
#define GL_MAP_INVALIDATE_RANGE_BIT 0x0004
#define GL_MAP_INVALIDATE_BUFFER_BIT 0x0008
#define GL_MAP_FLUSH_EXPLICIT_BIT 0x0010
#define GL_MAP_UNSYNCHRONIZED_BIT 0x0020
#define GL_RG 0x8227
#define GL_RG_INTEGER 0x8228
#define GL_R8 0x8229
#define GL_RG8 0x822B
#define GL_R16F 0x822D
#define GL_R32F 0x822E
#define GL_RG16F 0x822F
#define GL_RG32F 0x8230
#define GL_R8I 0x8231
#define GL_R8UI 0x8232
#define GL_R16I 0x8233
#define GL_R16UI 0x8234
#define GL_R32I 0x8235
#define GL_R32UI 0x8236
#define GL_RG8I 0x8237
#define GL_RG8UI 0x8238
#define GL_RG16I 0x8239
#define GL_RG16UI 0x823A
#define GL_RG32I 0x823B
#define GL_RG32UI 0x823C
#define GL_VERTEX_ARRAY_BINDING 0x85B5
#define GL_R8_SNORM 0x8F94
#define GL_RG8_SNORM 0x8F95
#define GL_RGB8_SNORM 0x8F96
#define GL_RGBA8_SNORM 0x8F97
#define GL_SIGNED_NORMALIZED 0x8F9C
#define GL_PRIMITIVE_RESTART_FIXED_INDEX 0x8D69
#define GL_COPY_READ_BUFFER 0x8F36
#define GL_COPY_WRITE_BUFFER 0x8F37
#define GL_COPY_READ_BUFFER_BINDING 0x8F36
#define GL_COPY_WRITE_BUFFER_BINDING 0x8F37
#define GL_UNIFORM_BUFFER 0x8A11
#define GL_UNIFORM_BUFFER_BINDING 0x8A28
#define GL_UNIFORM_BUFFER_START 0x8A29
#define GL_UNIFORM_BUFFER_SIZE 0x8A2A
#define GL_MAX_VERTEX_UNIFORM_BLOCKS 0x8A2B
#define GL_MAX_FRAGMENT_UNIFORM_BLOCKS 0x8A2D
#define GL_MAX_COMBINED_UNIFORM_BLOCKS 0x8A2E
#define GL_MAX_UNIFORM_BUFFER_BINDINGS 0x8A2F
#define GL_MAX_UNIFORM_BLOCK_SIZE 0x8A30
#define GL_MAX_COMBINED_VERTEX_UNIFORM_COMPONENTS 0x8A31
#define GL_MAX_COMBINED_FRAGMENT_UNIFORM_COMPONENTS 0x8A33
#define GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT 0x8A34
#define GL_ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH 0x8A35
#define GL_ACTIVE_UNIFORM_BLOCKS 0x8A36
#define GL_UNIFORM_TYPE 0x8A37
#define GL_UNIFORM_SIZE 0x8A38
#define GL_UNIFORM_NAME_LENGTH 0x8A39
#define GL_UNIFORM_BLOCK_INDEX 0x8A3A
#define GL_UNIFORM_OFFSET 0x8A3B
#define GL_UNIFORM_ARRAY_STRIDE 0x8A3C
#define GL_UNIFORM_MATRIX_STRIDE 0x8A3D
#define GL_UNIFORM_IS_ROW_MAJOR 0x8A3E
#define GL_UNIFORM_BLOCK_BINDING 0x8A3F
#define GL_UNIFORM_BLOCK_DATA_SIZE 0x8A40
#define GL_UNIFORM_BLOCK_NAME_LENGTH 0x8A41
#define GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS 0x8A42
#define GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES 0x8A43
#define GL_UNIFORM_BLOCK_REFERENCED_BY_VERTEX_SHADER 0x8A44
#define GL_UNIFORM_BLOCK_REFERENCED_BY_FRAGMENT_SHADER 0x8A46
#define GL_INVALID_INDEX 0xFFFFFFFF
#define GL_MAX_VERTEX_OUTPUT_COMPONENTS 0x9122
#define GL_MAX_FRAGMENT_INPUT_COMPONENTS 0x9125
#define GL_MAX_SERVER_WAIT_TIMEOUT 0x9111
#define GL_OBJECT_TYPE 0x9112
#define GL_SYNC_CONDITION 0x9113
#define GL_SYNC_STATUS 0x9114
#define GL_SYNC_FLAGS 0x9115
#define GL_SYNC_FENCE 0x9116
#define GL_SYNC_GPU_COMMANDS_COMPLETE 0x9117
#define GL_UNSIGNALED 0x9118
#define GL_SIGNALED 0x9119
#define GL_ALREADY_SIGNALED 0x911A
#define GL_TIMEOUT_EXPIRED 0x911B
#define GL_CONDITION_SATISFIED 0x911C
#define GL_WAIT_FAILED 0x911D
#define GL_SYNC_FLUSH_COMMANDS_BIT 0x00000001
#define GL_TIMEOUT_IGNORED 0xFFFFFFFFFFFFFFFF
#define GL_VERTEX_ATTRIB_ARRAY_DIVISOR 0x88FE
#define GL_ANY_SAMPLES_PASSED 0x8C2F
#define GL_ANY_SAMPLES_PASSED_CONSERVATIVE 0x8D6A
#define GL_SAMPLER_BINDING 0x8919
#define GL_RGB10_A2UI 0x906F
#define GL_TEXTURE_SWIZZLE_R 0x8E42
#define GL_TEXTURE_SWIZZLE_G 0x8E43
#define GL_TEXTURE_SWIZZLE_B 0x8E44
#define GL_TEXTURE_SWIZZLE_A 0x8E45
#define GL_GREEN 0x1904
#define GL_BLUE 0x1905
#define GL_INT_2_10_10_10_REV 0x8D9F
#define GL_TRANSFORM_FEEDBACK 0x8E22
#define GL_TRANSFORM_FEEDBACK_PAUSED 0x8E23
#define GL_TRANSFORM_FEEDBACK_ACTIVE 0x8E24
#define GL_TRANSFORM_FEEDBACK_BINDING 0x8E25
#define GL_PROGRAM_BINARY_RETRIEVABLE_HINT 0x8257
#define GL_PROGRAM_BINARY_LENGTH 0x8741
#define GL_NUM_PROGRAM_BINARY_FORMATS 0x87FE
#define GL_PROGRAM_BINARY_FORMATS 0x87FF
#define GL_COMPRESSED_R11_EAC 0x9270
#define GL_COMPRESSED_SIGNED_R11_EAC 0x9271
#define GL_COMPRESSED_RG11_EAC 0x9272
#define GL_COMPRESSED_SIGNED_RG11_EAC 0x9273
#define GL_COMPRESSED_RGB8_ETC2 0x9274
#define GL_COMPRESSED_SRGB8_ETC2 0x9275
#define GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2 0x9276
#define GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2 0x9277
#define GL_COMPRESSED_RGBA8_ETC2_EAC 0x9278
#define GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC 0x9279
#define GL_TEXTURE_IMMUTABLE_FORMAT 0x912F
#define GL_MAX_ELEMENT_INDEX 0x8D6B
#define GL_NUM_SAMPLE_COUNTS 0x9380
#define GL_TEXTURE_IMMUTABLE_LEVELS 0x82DF
#define GL_COMPUTE_SHADER 0x91B9
#define GL_MAX_COMPUTE_UNIFORM_BLOCKS 0x91BB
#define GL_MAX_COMPUTE_TEXTURE_IMAGE_UNITS 0x91BC
#define GL_MAX_COMPUTE_IMAGE_UNIFORMS 0x91BD
#define GL_MAX_COMPUTE_SHARED_MEMORY_SIZE 0x8262
#define GL_MAX_COMPUTE_UNIFORM_COMPONENTS 0x8263
#define GL_MAX_COMPUTE_ATOMIC_COUNTER_BUFFERS 0x8264
#define GL_MAX_COMPUTE_ATOMIC_COUNTERS 0x8265
#define GL_MAX_COMBINED_COMPUTE_UNIFORM_COMPONENTS 0x8266
#define GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS 0x90EB
#define GL_MAX_COMPUTE_WORK_GROUP_COUNT 0x91BE
#define GL_MAX_COMPUTE_WORK_GROUP_SIZE 0x91BF
#define GL_COMPUTE_WORK_GROUP_SIZE 0x8267
#define GL_DISPATCH_INDIRECT_BUFFER 0x90EE
#define GL_DISPATCH_INDIRECT_BUFFER_BINDING 0x90EF
#define GL_COMPUTE_SHADER_BIT 0x00000020
#define GL_DRAW_INDIRECT_BUFFER 0x8F3F
#define GL_DRAW_INDIRECT_BUFFER_BINDING 0x8F43
#define GL_MAX_UNIFORM_LOCATIONS 0x826E
#define GL_FRAMEBUFFER_DEFAULT_WIDTH 0x9310
#define GL_FRAMEBUFFER_DEFAULT_HEIGHT 0x9311
#define GL_FRAMEBUFFER_DEFAULT_SAMPLES 0x9313
#define GL_FRAMEBUFFER_DEFAULT_FIXED_SAMPLE_LOCATIONS 0x9314
#define GL_MAX_FRAMEBUFFER_WIDTH 0x9315
#define GL_MAX_FRAMEBUFFER_HEIGHT 0x9316
#define GL_MAX_FRAMEBUFFER_SAMPLES 0x9318
#define GL_UNIFORM 0x92E1
#define GL_UNIFORM_BLOCK 0x92E2
#define GL_PROGRAM_INPUT 0x92E3
#define GL_PROGRAM_OUTPUT 0x92E4
#define GL_BUFFER_VARIABLE 0x92E5
#define GL_SHADER_STORAGE_BLOCK 0x92E6
#define GL_ATOMIC_COUNTER_BUFFER 0x92C0
#define GL_TRANSFORM_FEEDBACK_VARYING 0x92F4
#define GL_ACTIVE_RESOURCES 0x92F5
#define GL_MAX_NAME_LENGTH 0x92F6
#define GL_MAX_NUM_ACTIVE_VARIABLES 0x92F7
#define GL_NAME_LENGTH 0x92F9
#define GL_TYPE 0x92FA
#define GL_ARRAY_SIZE 0x92FB
#define GL_OFFSET 0x92FC
#define GL_BLOCK_INDEX 0x92FD
#define GL_ARRAY_STRIDE 0x92FE
#define GL_MATRIX_STRIDE 0x92FF
#define GL_IS_ROW_MAJOR 0x9300
#define GL_ATOMIC_COUNTER_BUFFER_INDEX 0x9301
#define GL_BUFFER_BINDING 0x9302
#define GL_BUFFER_DATA_SIZE 0x9303
#define GL_NUM_ACTIVE_VARIABLES 0x9304
#define GL_ACTIVE_VARIABLES 0x9305
#define GL_REFERENCED_BY_VERTEX_SHADER 0x9306
#define GL_REFERENCED_BY_FRAGMENT_SHADER 0x930A
#define GL_REFERENCED_BY_COMPUTE_SHADER 0x930B
#define GL_TOP_LEVEL_ARRAY_SIZE 0x930C
#define GL_TOP_LEVEL_ARRAY_STRIDE 0x930D
#define GL_LOCATION 0x930E
#define GL_VERTEX_SHADER_BIT 0x00000001
#define GL_FRAGMENT_SHADER_BIT 0x00000002
#define GL_ALL_SHADER_BITS 0xFFFFFFFF
#define GL_PROGRAM_SEPARABLE 0x8258
#define GL_ACTIVE_PROGRAM 0x8259
#define GL_PROGRAM_PIPELINE_BINDING 0x825A
#define GL_ATOMIC_COUNTER_BUFFER_BINDING 0x92C1
#define GL_ATOMIC_COUNTER_BUFFER_START 0x92C2
#define GL_ATOMIC_COUNTER_BUFFER_SIZE 0x92C3
#define GL_MAX_VERTEX_ATOMIC_COUNTER_BUFFERS 0x92CC
#define GL_MAX_FRAGMENT_ATOMIC_COUNTER_BUFFERS 0x92D0
#define GL_MAX_COMBINED_ATOMIC_COUNTER_BUFFERS 0x92D1
#define GL_MAX_VERTEX_ATOMIC_COUNTERS 0x92D2
#define GL_MAX_FRAGMENT_ATOMIC_COUNTERS 0x92D6
#define GL_MAX_COMBINED_ATOMIC_COUNTERS 0x92D7
#define GL_MAX_ATOMIC_COUNTER_BUFFER_SIZE 0x92D8
#define GL_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS 0x92DC
#define GL_ACTIVE_ATOMIC_COUNTER_BUFFERS 0x92D9
#define GL_UNSIGNED_INT_ATOMIC_COUNTER 0x92DB
#define GL_MAX_IMAGE_UNITS 0x8F38
#define GL_MAX_VERTEX_IMAGE_UNIFORMS 0x90CA
#define GL_MAX_FRAGMENT_IMAGE_UNIFORMS 0x90CE
#define GL_MAX_COMBINED_IMAGE_UNIFORMS 0x90CF
#define GL_IMAGE_BINDING_NAME 0x8F3A
#define GL_IMAGE_BINDING_LEVEL 0x8F3B
#define GL_IMAGE_BINDING_LAYERED 0x8F3C
#define GL_IMAGE_BINDING_LAYER 0x8F3D
#define GL_IMAGE_BINDING_ACCESS 0x8F3E
#define GL_IMAGE_BINDING_FORMAT 0x906E
#define GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT 0x00000001
#define GL_ELEMENT_ARRAY_BARRIER_BIT 0x00000002
#define GL_UNIFORM_BARRIER_BIT 0x00000004
#define GL_TEXTURE_FETCH_BARRIER_BIT 0x00000008
#define GL_SHADER_IMAGE_ACCESS_BARRIER_BIT 0x00000020
#define GL_COMMAND_BARRIER_BIT 0x00000040
#define GL_PIXEL_BUFFER_BARRIER_BIT 0x00000080
#define GL_TEXTURE_UPDATE_BARRIER_BIT 0x00000100
#define GL_BUFFER_UPDATE_BARRIER_BIT 0x00000200
#define GL_FRAMEBUFFER_BARRIER_BIT 0x00000400
#define GL_TRANSFORM_FEEDBACK_BARRIER_BIT 0x00000800
#define GL_ATOMIC_COUNTER_BARRIER_BIT 0x00001000
#define GL_ALL_BARRIER_BITS 0xFFFFFFFF
#define GL_IMAGE_2D 0x904D
#define GL_IMAGE_3D 0x904E
#define GL_IMAGE_CUBE 0x9050
#define GL_IMAGE_2D_ARRAY 0x9053
#define GL_INT_IMAGE_2D 0x9058
#define GL_INT_IMAGE_3D 0x9059
#define GL_INT_IMAGE_CUBE 0x905B
#define GL_INT_IMAGE_2D_ARRAY 0x905E
#define GL_UNSIGNED_INT_IMAGE_2D 0x9063
#define GL_UNSIGNED_INT_IMAGE_3D 0x9064
#define GL_UNSIGNED_INT_IMAGE_CUBE 0x9066
#define GL_UNSIGNED_INT_IMAGE_2D_ARRAY 0x9069
#define GL_IMAGE_FORMAT_COMPATIBILITY_TYPE 0x90C7
#define GL_IMAGE_FORMAT_COMPATIBILITY_BY_SIZE 0x90C8
#define GL_IMAGE_FORMAT_COMPATIBILITY_BY_CLASS 0x90C9
#define GL_READ_ONLY 0x88B8
#define GL_WRITE_ONLY 0x88B9
#define GL_READ_WRITE 0x88BA
#define GL_SHADER_STORAGE_BUFFER 0x90D2
#define GL_SHADER_STORAGE_BUFFER_BINDING 0x90D3
#define GL_SHADER_STORAGE_BUFFER_START 0x90D4
#define GL_SHADER_STORAGE_BUFFER_SIZE 0x90D5
#define GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS 0x90D6
#define GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS 0x90DA
#define GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS 0x90DB
#define GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS 0x90DC
#define GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS 0x90DD
#define GL_MAX_SHADER_STORAGE_BLOCK_SIZE 0x90DE
#define GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT 0x90DF
#define GL_SHADER_STORAGE_BARRIER_BIT 0x00002000
#define GL_MAX_COMBINED_SHADER_OUTPUT_RESOURCES 0x8F39
#define GL_DEPTH_STENCIL_TEXTURE_MODE 0x90EA
#define GL_STENCIL_INDEX 0x1901
#define GL_MIN_PROGRAM_TEXTURE_GATHER_OFFSET 0x8E5E
#define GL_MAX_PROGRAM_TEXTURE_GATHER_OFFSET 0x8E5F
#define GL_SAMPLE_POSITION 0x8E50
#define GL_SAMPLE_MASK 0x8E51
#define GL_SAMPLE_MASK_VALUE 0x8E52
#define GL_TEXTURE_2D_MULTISAMPLE 0x9100
#define GL_MAX_SAMPLE_MASK_WORDS 0x8E59
#define GL_MAX_COLOR_TEXTURE_SAMPLES 0x910E
#define GL_MAX_DEPTH_TEXTURE_SAMPLES 0x910F
#define GL_MAX_INTEGER_SAMPLES 0x9110
#define GL_TEXTURE_BINDING_2D_MULTISAMPLE 0x9104
#define GL_TEXTURE_SAMPLES 0x9106
#define GL_TEXTURE_FIXED_SAMPLE_LOCATIONS 0x9107
#define GL_TEXTURE_WIDTH 0x1000
#define GL_TEXTURE_HEIGHT 0x1001
#define GL_TEXTURE_DEPTH 0x8071
#define GL_TEXTURE_INTERNAL_FORMAT 0x1003
#define GL_TEXTURE_RED_SIZE 0x805C
#define GL_TEXTURE_GREEN_SIZE 0x805D
#define GL_TEXTURE_BLUE_SIZE 0x805E
#define GL_TEXTURE_ALPHA_SIZE 0x805F
#define GL_TEXTURE_DEPTH_SIZE 0x884A
#define GL_TEXTURE_STENCIL_SIZE 0x88F1
#define GL_TEXTURE_SHARED_SIZE 0x8C3F
#define GL_TEXTURE_RED_TYPE 0x8C10
#define GL_TEXTURE_GREEN_TYPE 0x8C11
#define GL_TEXTURE_BLUE_TYPE 0x8C12
#define GL_TEXTURE_ALPHA_TYPE 0x8C13
#define GL_TEXTURE_DEPTH_TYPE 0x8C16
#define GL_TEXTURE_COMPRESSED 0x86A1
#define GL_SAMPLER_2D_MULTISAMPLE 0x9108
#define GL_INT_SAMPLER_2D_MULTISAMPLE 0x9109
#define GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE 0x910A
#define GL_VERTEX_ATTRIB_BINDING 0x82D4
#define GL_VERTEX_ATTRIB_RELATIVE_OFFSET 0x82D5
#define GL_VERTEX_BINDING_DIVISOR 0x82D6
#define GL_VERTEX_BINDING_OFFSET 0x82D7
#define GL_VERTEX_BINDING_STRIDE 0x82D8
#define GL_VERTEX_BINDING_BUFFER 0x8F4F
#define GL_MAX_VERTEX_ATTRIB_RELATIVE_OFFSET 0x82D9
#define GL_MAX_VERTEX_ATTRIB_BINDINGS 0x82DA
#define GL_MAX_VERTEX_ATTRIB_STRIDE 0x82E5
#define GL_MULTISAMPLE_LINE_WIDTH_RANGE 0x9381
#define GL_MULTISAMPLE_LINE_WIDTH_GRANULARITY 0x9382
#define GL_MULTIPLY 0x9294
#define GL_SCREEN 0x9295
#define GL_OVERLAY 0x9296
#define GL_DARKEN 0x9297
#define GL_LIGHTEN 0x9298
#define GL_COLORDODGE 0x9299
#define GL_COLORBURN 0x929A
#define GL_HARDLIGHT 0x929B
#define GL_SOFTLIGHT 0x929C
#define GL_DIFFERENCE 0x929E
#define GL_EXCLUSION 0x92A0
#define GL_HSL_HUE 0x92AD
#define GL_HSL_SATURATION 0x92AE
#define GL_HSL_COLOR 0x92AF
#define GL_HSL_LUMINOSITY 0x92B0
#define GL_DEBUG_OUTPUT_SYNCHRONOUS 0x8242
#define GL_DEBUG_NEXT_LOGGED_MESSAGE_LENGTH 0x8243
#define GL_DEBUG_CALLBACK_FUNCTION 0x8244
#define GL_DEBUG_CALLBACK_USER_PARAM 0x8245
#define GL_DEBUG_SOURCE_API 0x8246
#define GL_DEBUG_SOURCE_WINDOW_SYSTEM 0x8247
#define GL_DEBUG_SOURCE_SHADER_COMPILER 0x8248
#define GL_DEBUG_SOURCE_THIRD_PARTY 0x8249
#define GL_DEBUG_SOURCE_APPLICATION 0x824A
#define GL_DEBUG_SOURCE_OTHER 0x824B
#define GL_DEBUG_TYPE_ERROR 0x824C
#define GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR 0x824D
#define GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR 0x824E
#define GL_DEBUG_TYPE_PORTABILITY 0x824F
#define GL_DEBUG_TYPE_PERFORMANCE 0x8250
#define GL_DEBUG_TYPE_OTHER 0x8251
#define GL_DEBUG_TYPE_MARKER 0x8268
#define GL_DEBUG_TYPE_PUSH_GROUP 0x8269
#define GL_DEBUG_TYPE_POP_GROUP 0x826A
#define GL_DEBUG_SEVERITY_NOTIFICATION 0x826B
#define GL_MAX_DEBUG_GROUP_STACK_DEPTH 0x826C
#define GL_DEBUG_GROUP_STACK_DEPTH 0x826D
#define GL_BUFFER 0x82E0
#define GL_SHADER 0x82E1
#define GL_PROGRAM 0x82E2
#define GL_VERTEX_ARRAY 0x8074
#define GL_QUERY 0x82E3
#define GL_PROGRAM_PIPELINE 0x82E4
#define GL_SAMPLER 0x82E6
#define GL_MAX_LABEL_LENGTH 0x82E8
#define GL_MAX_DEBUG_MESSAGE_LENGTH 0x9143
#define GL_MAX_DEBUG_LOGGED_MESSAGES 0x9144
#define GL_DEBUG_LOGGED_MESSAGES 0x9145
#define GL_DEBUG_SEVERITY_HIGH 0x9146
#define GL_DEBUG_SEVERITY_MEDIUM 0x9147
#define GL_DEBUG_SEVERITY_LOW 0x9148
#define GL_DEBUG_OUTPUT 0x92E0
#define GL_CONTEXT_FLAG_DEBUG_BIT 0x00000002
#define GL_STACK_OVERFLOW 0x0503
#define GL_STACK_UNDERFLOW 0x0504
#define GL_GEOMETRY_SHADER 0x8DD9
#define GL_GEOMETRY_SHADER_BIT 0x00000004
#define GL_GEOMETRY_VERTICES_OUT 0x8916
#define GL_GEOMETRY_INPUT_TYPE 0x8917
#define GL_GEOMETRY_OUTPUT_TYPE 0x8918
#define GL_GEOMETRY_SHADER_INVOCATIONS 0x887F
#define GL_LAYER_PROVOKING_VERTEX 0x825E
#define GL_LINES_ADJACENCY 0x000A
#define GL_LINE_STRIP_ADJACENCY 0x000B
#define GL_TRIANGLES_ADJACENCY 0x000C
#define GL_TRIANGLE_STRIP_ADJACENCY 0x000D
#define GL_MAX_GEOMETRY_UNIFORM_COMPONENTS 0x8DDF
#define GL_MAX_GEOMETRY_UNIFORM_BLOCKS 0x8A2C
#define GL_MAX_COMBINED_GEOMETRY_UNIFORM_COMPONENTS 0x8A32
#define GL_MAX_GEOMETRY_INPUT_COMPONENTS 0x9123
#define GL_MAX_GEOMETRY_OUTPUT_COMPONENTS 0x9124
#define GL_MAX_GEOMETRY_OUTPUT_VERTICES 0x8DE0
#define GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS 0x8DE1
#define GL_MAX_GEOMETRY_SHADER_INVOCATIONS 0x8E5A
#define GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS 0x8C29
#define GL_MAX_GEOMETRY_ATOMIC_COUNTER_BUFFERS 0x92CF
#define GL_MAX_GEOMETRY_ATOMIC_COUNTERS 0x92D5
#define GL_MAX_GEOMETRY_IMAGE_UNIFORMS 0x90CD
#define GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS 0x90D7
#define GL_FIRST_VERTEX_CONVENTION 0x8E4D
#define GL_LAST_VERTEX_CONVENTION 0x8E4E
#define GL_UNDEFINED_VERTEX 0x8260
#define GL_PRIMITIVES_GENERATED 0x8C87
#define GL_FRAMEBUFFER_DEFAULT_LAYERS 0x9312
#define GL_MAX_FRAMEBUFFER_LAYERS 0x9317
#define GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS 0x8DA8
#define GL_FRAMEBUFFER_ATTACHMENT_LAYERED 0x8DA7
#define GL_REFERENCED_BY_GEOMETRY_SHADER 0x9309
#define GL_PRIMITIVE_BOUNDING_BOX 0x92BE
#define GL_CONTEXT_FLAG_ROBUST_ACCESS_BIT 0x00000004
#define GL_CONTEXT_FLAGS 0x821E
#define GL_LOSE_CONTEXT_ON_RESET 0x8252
#define GL_GUILTY_CONTEXT_RESET 0x8253
#define GL_INNOCENT_CONTEXT_RESET 0x8254
#define GL_UNKNOWN_CONTEXT_RESET 0x8255
#define GL_RESET_NOTIFICATION_STRATEGY 0x8256
#define GL_NO_RESET_NOTIFICATION 0x8261
#define GL_CONTEXT_LOST 0x0507
#define GL_SAMPLE_SHADING 0x8C36
#define GL_MIN_SAMPLE_SHADING_VALUE 0x8C37
#define GL_MIN_FRAGMENT_INTERPOLATION_OFFSET 0x8E5B
#define GL_MAX_FRAGMENT_INTERPOLATION_OFFSET 0x8E5C
#define GL_FRAGMENT_INTERPOLATION_OFFSET_BITS 0x8E5D
#define GL_PATCHES 0x000E
#define GL_PATCH_VERTICES 0x8E72
#define GL_TESS_CONTROL_OUTPUT_VERTICES 0x8E75
#define GL_TESS_GEN_MODE 0x8E76
#define GL_TESS_GEN_SPACING 0x8E77
#define GL_TESS_GEN_VERTEX_ORDER 0x8E78
#define GL_TESS_GEN_POINT_MODE 0x8E79
#define GL_ISOLINES 0x8E7A
#define GL_QUADS 0x0007
#define GL_FRACTIONAL_ODD 0x8E7B
#define GL_FRACTIONAL_EVEN 0x8E7C
#define GL_MAX_PATCH_VERTICES 0x8E7D
#define GL_MAX_TESS_GEN_LEVEL 0x8E7E
#define GL_MAX_TESS_CONTROL_UNIFORM_COMPONENTS 0x8E7F
#define GL_MAX_TESS_EVALUATION_UNIFORM_COMPONENTS 0x8E80
#define GL_MAX_TESS_CONTROL_TEXTURE_IMAGE_UNITS 0x8E81
#define GL_MAX_TESS_EVALUATION_TEXTURE_IMAGE_UNITS 0x8E82
#define GL_MAX_TESS_CONTROL_OUTPUT_COMPONENTS 0x8E83
#define GL_MAX_TESS_PATCH_COMPONENTS 0x8E84
#define GL_MAX_TESS_CONTROL_TOTAL_OUTPUT_COMPONENTS 0x8E85
#define GL_MAX_TESS_EVALUATION_OUTPUT_COMPONENTS 0x8E86
#define GL_MAX_TESS_CONTROL_UNIFORM_BLOCKS 0x8E89
#define GL_MAX_TESS_EVALUATION_UNIFORM_BLOCKS 0x8E8A
#define GL_MAX_TESS_CONTROL_INPUT_COMPONENTS 0x886C
#define GL_MAX_TESS_EVALUATION_INPUT_COMPONENTS 0x886D
#define GL_MAX_COMBINED_TESS_CONTROL_UNIFORM_COMPONENTS 0x8E1E
#define GL_MAX_COMBINED_TESS_EVALUATION_UNIFORM_COMPONENTS 0x8E1F
#define GL_MAX_TESS_CONTROL_ATOMIC_COUNTER_BUFFERS 0x92CD
#define GL_MAX_TESS_EVALUATION_ATOMIC_COUNTER_BUFFERS 0x92CE
#define GL_MAX_TESS_CONTROL_ATOMIC_COUNTERS 0x92D3
#define GL_MAX_TESS_EVALUATION_ATOMIC_COUNTERS 0x92D4
#define GL_MAX_TESS_CONTROL_IMAGE_UNIFORMS 0x90CB
#define GL_MAX_TESS_EVALUATION_IMAGE_UNIFORMS 0x90CC
#define GL_MAX_TESS_CONTROL_SHADER_STORAGE_BLOCKS 0x90D8
#define GL_MAX_TESS_EVALUATION_SHADER_STORAGE_BLOCKS 0x90D9
#define GL_PRIMITIVE_RESTART_FOR_PATCHES_SUPPORTED 0x8221
#define GL_IS_PER_PATCH 0x92E7
#define GL_REFERENCED_BY_TESS_CONTROL_SHADER 0x9307
#define GL_REFERENCED_BY_TESS_EVALUATION_SHADER 0x9308
#define GL_TESS_CONTROL_SHADER 0x8E88
#define GL_TESS_EVALUATION_SHADER 0x8E87
#define GL_TESS_CONTROL_SHADER_BIT 0x00000008
#define GL_TESS_EVALUATION_SHADER_BIT 0x00000010
#define GL_TEXTURE_BORDER_COLOR 0x1004
#define GL_CLAMP_TO_BORDER 0x812D
#define GL_TEXTURE_BUFFER 0x8C2A
#define GL_TEXTURE_BUFFER_BINDING 0x8C2A
#define GL_MAX_TEXTURE_BUFFER_SIZE 0x8C2B
#define GL_TEXTURE_BINDING_BUFFER 0x8C2C
#define GL_TEXTURE_BUFFER_DATA_STORE_BINDING 0x8C2D
#define GL_TEXTURE_BUFFER_OFFSET_ALIGNMENT 0x919F
#define GL_SAMPLER_BUFFER 0x8DC2
#define GL_INT_SAMPLER_BUFFER 0x8DD0
#define GL_UNSIGNED_INT_SAMPLER_BUFFER 0x8DD8
#define GL_IMAGE_BUFFER 0x9051
#define GL_INT_IMAGE_BUFFER 0x905C
#define GL_UNSIGNED_INT_IMAGE_BUFFER 0x9067
#define GL_TEXTURE_BUFFER_OFFSET 0x919D
#define GL_TEXTURE_BUFFER_SIZE 0x919E
#define GL_COMPRESSED_RGBA_ASTC_4x4 0x93B0
#define GL_COMPRESSED_RGBA_ASTC_5x4 0x93B1
#define GL_COMPRESSED_RGBA_ASTC_5x5 0x93B2
#define GL_COMPRESSED_RGBA_ASTC_6x5 0x93B3
#define GL_COMPRESSED_RGBA_ASTC_6x6 0x93B4
#define GL_COMPRESSED_RGBA_ASTC_8x5 0x93B5
#define GL_COMPRESSED_RGBA_ASTC_8x6 0x93B6
#define GL_COMPRESSED_RGBA_ASTC_8x8 0x93B7
#define GL_COMPRESSED_RGBA_ASTC_10x5 0x93B8
#define GL_COMPRESSED_RGBA_ASTC_10x6 0x93B9
#define GL_COMPRESSED_RGBA_ASTC_10x8 0x93BA
#define GL_COMPRESSED_RGBA_ASTC_10x10 0x93BB
#define GL_COMPRESSED_RGBA_ASTC_12x10 0x93BC
#define GL_COMPRESSED_RGBA_ASTC_12x12 0x93BD
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4 0x93D0
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4 0x93D1
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5 0x93D2
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5 0x93D3
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6 0x93D4
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5 0x93D5
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6 0x93D6
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8 0x93D7
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5 0x93D8
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6 0x93D9
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8 0x93DA
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10 0x93DB
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10 0x93DC
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12 0x93DD
#define GL_TEXTURE_CUBE_MAP_ARRAY 0x9009
#define GL_TEXTURE_BINDING_CUBE_MAP_ARRAY 0x900A
#define GL_SAMPLER_CUBE_MAP_ARRAY 0x900C
#define GL_SAMPLER_CUBE_MAP_ARRAY_SHADOW 0x900D
#define GL_INT_SAMPLER_CUBE_MAP_ARRAY 0x900E
#define GL_UNSIGNED_INT_SAMPLER_CUBE_MAP_ARRAY 0x900F
#define GL_IMAGE_CUBE_MAP_ARRAY 0x9054
#define GL_INT_IMAGE_CUBE_MAP_ARRAY 0x905F
#define GL_UNSIGNED_INT_IMAGE_CUBE_MAP_ARRAY 0x906A
#define GL_TEXTURE_2D_MULTISAMPLE_ARRAY 0x9102
#define GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY 0x9105
#define GL_SAMPLER_2D_MULTISAMPLE_ARRAY 0x910B
#define GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY 0x910C
#define GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY 0x910D
#ifndef GL_ES_VERSION_2_0
#define GL_ES_VERSION_2_0 1
#endif

//GL2 PFNS
#define GL_APIENTRYP APIENTRYP
typedef void (GL_APIENTRYP PFNGLACTIVETEXTUREPROC) (GLenum texture);
typedef void (GL_APIENTRYP PFNGLATTACHSHADERPROC) (GLuint program, GLuint shader);
typedef void (GL_APIENTRYP PFNGLBINDATTRIBLOCATIONPROC) (GLuint program, GLuint index, const GLchar *name);
typedef void (GL_APIENTRYP PFNGLBINDBUFFERPROC) (GLenum target, GLuint buffer);
typedef void (GL_APIENTRYP PFNGLBINDFRAMEBUFFERPROC) (GLenum target, GLuint framebuffer);
typedef void (GL_APIENTRYP PFNGLBINDRENDERBUFFERPROC) (GLenum target, GLuint renderbuffer);
typedef void (GL_APIENTRYP PFNGLBINDTEXTUREPROC) (GLenum target, GLuint texture);
typedef void (GL_APIENTRYP PFNGLBLENDCOLORPROC) (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
typedef void (GL_APIENTRYP PFNGLBLENDEQUATIONPROC) (GLenum mode);
typedef void (GL_APIENTRYP PFNGLBLENDEQUATIONSEPARATEPROC) (GLenum modeRGB, GLenum modeAlpha);
typedef void (GL_APIENTRYP PFNGLBLENDFUNCPROC) (GLenum sfactor, GLenum dfactor);
typedef void (GL_APIENTRYP PFNGLBLENDFUNCSEPARATEPROC) (GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha);
typedef void (GL_APIENTRYP PFNGLBUFFERDATAPROC) (GLenum target, GLsizeiptr size, const void *data, GLenum usage);
typedef void (GL_APIENTRYP PFNGLBUFFERSUBDATAPROC) (GLenum target, GLintptr offset, GLsizeiptr size, const void *data);
typedef GLenum (GL_APIENTRYP PFNGLCHECKFRAMEBUFFERSTATUSPROC) (GLenum target);
typedef void (GL_APIENTRYP PFNGLCLEARPROC) (GLbitfield mask);
typedef void (GL_APIENTRYP PFNGLCLEARCOLORPROC) (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
typedef void (GL_APIENTRYP PFNGLCLEARDEPTHFPROC) (GLfloat d);
typedef void (GL_APIENTRYP PFNGLCLEARSTENCILPROC) (GLint s);
typedef void (GL_APIENTRYP PFNGLCOLORMASKPROC) (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
typedef void (GL_APIENTRYP PFNGLCOMPILESHADERPROC) (GLuint shader);
typedef void (GL_APIENTRYP PFNGLCOMPRESSEDTEXIMAGE2DPROC) (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *data);
typedef void (GL_APIENTRYP PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data);
typedef void (GL_APIENTRYP PFNGLCOPYTEXIMAGE2DPROC) (GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
typedef void (GL_APIENTRYP PFNGLCOPYTEXSUBIMAGE2DPROC) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
typedef GLuint (GL_APIENTRYP PFNGLCREATEPROGRAMPROC) (void);
typedef GLuint (GL_APIENTRYP PFNGLCREATESHADERPROC) (GLenum type);
typedef void (GL_APIENTRYP PFNGLCULLFACEPROC) (GLenum mode);
typedef void (GL_APIENTRYP PFNGLDELETEBUFFERSPROC) (GLsizei n, const GLuint *buffers);
typedef void (GL_APIENTRYP PFNGLDELETEFRAMEBUFFERSPROC) (GLsizei n, const GLuint *framebuffers);
typedef void (GL_APIENTRYP PFNGLDELETEPROGRAMPROC) (GLuint program);
typedef void (GL_APIENTRYP PFNGLDELETERENDERBUFFERSPROC) (GLsizei n, const GLuint *renderbuffers);
typedef void (GL_APIENTRYP PFNGLDELETESHADERPROC) (GLuint shader);
typedef void (GL_APIENTRYP PFNGLDELETETEXTURESPROC) (GLsizei n, const GLuint *textures);
typedef void (GL_APIENTRYP PFNGLDEPTHFUNCPROC) (GLenum func);
typedef void (GL_APIENTRYP PFNGLDEPTHMASKPROC) (GLboolean flag);
typedef void (GL_APIENTRYP PFNGLDEPTHRANGEFPROC) (GLfloat n, GLfloat f);
typedef void (GL_APIENTRYP PFNGLDETACHSHADERPROC) (GLuint program, GLuint shader);
typedef void (GL_APIENTRYP PFNGLDISABLEPROC) (GLenum cap);
typedef void (GL_APIENTRYP PFNGLDISABLEVERTEXATTRIBARRAYPROC) (GLuint index);
typedef void (GL_APIENTRYP PFNGLDRAWARRAYSPROC) (GLenum mode, GLint first, GLsizei count);
typedef void (GL_APIENTRYP PFNGLDRAWELEMENTSPROC) (GLenum mode, GLsizei count, GLenum type, const void *indices);
typedef void (GL_APIENTRYP PFNGLENABLEPROC) (GLenum cap);
typedef void (GL_APIENTRYP PFNGLENABLEVERTEXATTRIBARRAYPROC) (GLuint index);
typedef void (GL_APIENTRYP PFNGLFINISHPROC) (void);
typedef void (GL_APIENTRYP PFNGLFLUSHPROC) (void);
typedef void (GL_APIENTRYP PFNGLFRAMEBUFFERRENDERBUFFERPROC) (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
typedef void (GL_APIENTRYP PFNGLFRAMEBUFFERTEXTURE2DPROC) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef void (GL_APIENTRYP PFNGLFRONTFACEPROC) (GLenum mode);
typedef void (GL_APIENTRYP PFNGLGENBUFFERSPROC) (GLsizei n, GLuint *buffers);
typedef void (GL_APIENTRYP PFNGLGENERATEMIPMAPPROC) (GLenum target);
typedef void (GL_APIENTRYP PFNGLGENFRAMEBUFFERSPROC) (GLsizei n, GLuint *framebuffers);
typedef void (GL_APIENTRYP PFNGLGENRENDERBUFFERSPROC) (GLsizei n, GLuint *renderbuffers);
typedef void (GL_APIENTRYP PFNGLGENTEXTURESPROC) (GLsizei n, GLuint *textures);
typedef void (GL_APIENTRYP PFNGLGETACTIVEATTRIBPROC) (GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name);
typedef void (GL_APIENTRYP PFNGLGETACTIVEUNIFORMPROC) (GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name);
typedef void (GL_APIENTRYP PFNGLGETATTACHEDSHADERSPROC) (GLuint program, GLsizei maxCount, GLsizei *count, GLuint *shaders);
typedef GLint (GL_APIENTRYP PFNGLGETATTRIBLOCATIONPROC) (GLuint program, const GLchar *name);
typedef void (GL_APIENTRYP PFNGLGETBOOLEANVPROC) (GLenum pname, GLboolean *data);
typedef void (GL_APIENTRYP PFNGLGETBUFFERPARAMETERIVPROC) (GLenum target, GLenum pname, GLint *params);
typedef GLenum (GL_APIENTRYP PFNGLGETERRORPROC) (void);
typedef void (GL_APIENTRYP PFNGLGETFLOATVPROC) (GLenum pname, GLfloat *data);
typedef void (GL_APIENTRYP PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC) (GLenum target, GLenum attachment, GLenum pname, GLint *params);
typedef void (GL_APIENTRYP PFNGLGETINTEGERVPROC) (GLenum pname, GLint *data);
typedef void (GL_APIENTRYP PFNGLGETPROGRAMIVPROC) (GLuint program, GLenum pname, GLint *params);
typedef void (GL_APIENTRYP PFNGLGETPROGRAMINFOLOGPROC) (GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void (GL_APIENTRYP PFNGLGETRENDERBUFFERPARAMETERIVPROC) (GLenum target, GLenum pname, GLint *params);
typedef void (GL_APIENTRYP PFNGLGETSHADERIVPROC) (GLuint shader, GLenum pname, GLint *params);
typedef void (GL_APIENTRYP PFNGLGETSHADERINFOLOGPROC) (GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void (GL_APIENTRYP PFNGLGETSHADERPRECISIONFORMATPROC) (GLenum shadertype, GLenum precisiontype, GLint *range, GLint *precision);
typedef void (GL_APIENTRYP PFNGLGETSHADERSOURCEPROC) (GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *source);
typedef const GLubyte *(GL_APIENTRYP PFNGLGETSTRINGPROC) (GLenum name);
typedef void (GL_APIENTRYP PFNGLGETTEXPARAMETERFVPROC) (GLenum target, GLenum pname, GLfloat *params);
typedef void (GL_APIENTRYP PFNGLGETTEXPARAMETERIVPROC) (GLenum target, GLenum pname, GLint *params);
typedef void (GL_APIENTRYP PFNGLGETUNIFORMFVPROC) (GLuint program, GLint location, GLfloat *params);
typedef void (GL_APIENTRYP PFNGLGETUNIFORMIVPROC) (GLuint program, GLint location, GLint *params);
typedef GLint (GL_APIENTRYP PFNGLGETUNIFORMLOCATIONPROC) (GLuint program, const GLchar *name);
typedef void (GL_APIENTRYP PFNGLGETVERTEXATTRIBFVPROC) (GLuint index, GLenum pname, GLfloat *params);
typedef void (GL_APIENTRYP PFNGLGETVERTEXATTRIBIVPROC) (GLuint index, GLenum pname, GLint *params);
typedef void (GL_APIENTRYP PFNGLGETVERTEXATTRIBPOINTERVPROC) (GLuint index, GLenum pname, void **pointer);
typedef void (GL_APIENTRYP PFNGLHINTPROC) (GLenum target, GLenum mode);
typedef GLboolean (GL_APIENTRYP PFNGLISBUFFERPROC) (GLuint buffer);
typedef GLboolean (GL_APIENTRYP PFNGLISENABLEDPROC) (GLenum cap);
typedef GLboolean (GL_APIENTRYP PFNGLISFRAMEBUFFERPROC) (GLuint framebuffer);
typedef GLboolean (GL_APIENTRYP PFNGLISPROGRAMPROC) (GLuint program);
typedef GLboolean (GL_APIENTRYP PFNGLISRENDERBUFFERPROC) (GLuint renderbuffer);
typedef GLboolean (GL_APIENTRYP PFNGLISSHADERPROC) (GLuint shader);
typedef GLboolean (GL_APIENTRYP PFNGLISTEXTUREPROC) (GLuint texture);
typedef void (GL_APIENTRYP PFNGLLINEWIDTHPROC) (GLfloat width);
typedef void (GL_APIENTRYP PFNGLLINKPROGRAMPROC) (GLuint program);
typedef void (GL_APIENTRYP PFNGLPIXELSTOREIPROC) (GLenum pname, GLint param);
typedef void (GL_APIENTRYP PFNGLPOLYGONOFFSETPROC) (GLfloat factor, GLfloat units);
typedef void (GL_APIENTRYP PFNGLREADPIXELSPROC) (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void *pixels);
typedef void (GL_APIENTRYP PFNGLRELEASESHADERCOMPILERPROC) (void);
typedef void (GL_APIENTRYP PFNGLRENDERBUFFERSTORAGEPROC) (GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
typedef void (GL_APIENTRYP PFNGLSAMPLECOVERAGEPROC) (GLfloat value, GLboolean invert);
typedef void (GL_APIENTRYP PFNGLSCISSORPROC) (GLint x, GLint y, GLsizei width, GLsizei height);
typedef void (GL_APIENTRYP PFNGLSHADERBINARYPROC) (GLsizei count, const GLuint *shaders, GLenum binaryformat, const void *binary, GLsizei length);
typedef void (GL_APIENTRYP PFNGLSHADERSOURCEPROC) (GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length);
typedef void (GL_APIENTRYP PFNGLSTENCILFUNCPROC) (GLenum func, GLint ref, GLuint mask);
typedef void (GL_APIENTRYP PFNGLSTENCILFUNCSEPARATEPROC) (GLenum face, GLenum func, GLint ref, GLuint mask);
typedef void (GL_APIENTRYP PFNGLSTENCILMASKPROC) (GLuint mask);
typedef void (GL_APIENTRYP PFNGLSTENCILMASKSEPARATEPROC) (GLenum face, GLuint mask);
typedef void (GL_APIENTRYP PFNGLSTENCILOPPROC) (GLenum fail, GLenum zfail, GLenum zpass);
typedef void (GL_APIENTRYP PFNGLSTENCILOPSEPARATEPROC) (GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass);
typedef void (GL_APIENTRYP PFNGLTEXIMAGE2DPROC) (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels);
typedef void (GL_APIENTRYP PFNGLTEXPARAMETERFPROC) (GLenum target, GLenum pname, GLfloat param);
typedef void (GL_APIENTRYP PFNGLTEXPARAMETERFVPROC) (GLenum target, GLenum pname, const GLfloat *params);
typedef void (GL_APIENTRYP PFNGLTEXPARAMETERIPROC) (GLenum target, GLenum pname, GLint param);
typedef void (GL_APIENTRYP PFNGLTEXPARAMETERIVPROC) (GLenum target, GLenum pname, const GLint *params);
typedef void (GL_APIENTRYP PFNGLTEXSUBIMAGE2DPROC) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels);
typedef void (GL_APIENTRYP PFNGLUNIFORM1FPROC) (GLint location, GLfloat v0);
typedef void (GL_APIENTRYP PFNGLUNIFORM1FVPROC) (GLint location, GLsizei count, const GLfloat *value);
typedef void (GL_APIENTRYP PFNGLUNIFORM1IPROC) (GLint location, GLint v0);
typedef void (GL_APIENTRYP PFNGLUNIFORM1IVPROC) (GLint location, GLsizei count, const GLint *value);
typedef void (GL_APIENTRYP PFNGLUNIFORM2FPROC) (GLint location, GLfloat v0, GLfloat v1);
typedef void (GL_APIENTRYP PFNGLUNIFORM2FVPROC) (GLint location, GLsizei count, const GLfloat *value);
typedef void (GL_APIENTRYP PFNGLUNIFORM2IPROC) (GLint location, GLint v0, GLint v1);
typedef void (GL_APIENTRYP PFNGLUNIFORM2IVPROC) (GLint location, GLsizei count, const GLint *value);
typedef void (GL_APIENTRYP PFNGLUNIFORM3FPROC) (GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
typedef void (GL_APIENTRYP PFNGLUNIFORM3FVPROC) (GLint location, GLsizei count, const GLfloat *value);
typedef void (GL_APIENTRYP PFNGLUNIFORM3IPROC) (GLint location, GLint v0, GLint v1, GLint v2);
typedef void (GL_APIENTRYP PFNGLUNIFORM3IVPROC) (GLint location, GLsizei count, const GLint *value);
typedef void (GL_APIENTRYP PFNGLUNIFORM4FPROC) (GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
typedef void (GL_APIENTRYP PFNGLUNIFORM4FVPROC) (GLint location, GLsizei count, const GLfloat *value);
typedef void (GL_APIENTRYP PFNGLUNIFORM4IPROC) (GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
typedef void (GL_APIENTRYP PFNGLUNIFORM4IVPROC) (GLint location, GLsizei count, const GLint *value);
typedef void (GL_APIENTRYP PFNGLUNIFORMMATRIX2FVPROC) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void (GL_APIENTRYP PFNGLUNIFORMMATRIX3FVPROC) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void (GL_APIENTRYP PFNGLUNIFORMMATRIX4FVPROC) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void (GL_APIENTRYP PFNGLUSEPROGRAMPROC) (GLuint program);
typedef void (GL_APIENTRYP PFNGLVALIDATEPROGRAMPROC) (GLuint program);
typedef void (GL_APIENTRYP PFNGLVERTEXATTRIB1FPROC) (GLuint index, GLfloat x);
typedef void (GL_APIENTRYP PFNGLVERTEXATTRIB1FVPROC) (GLuint index, const GLfloat *v);
typedef void (GL_APIENTRYP PFNGLVERTEXATTRIB2FPROC) (GLuint index, GLfloat x, GLfloat y);
typedef void (GL_APIENTRYP PFNGLVERTEXATTRIB2FVPROC) (GLuint index, const GLfloat *v);
typedef void (GL_APIENTRYP PFNGLVERTEXATTRIB3FPROC) (GLuint index, GLfloat x, GLfloat y, GLfloat z);
typedef void (GL_APIENTRYP PFNGLVERTEXATTRIB3FVPROC) (GLuint index, const GLfloat *v);
typedef void (GL_APIENTRYP PFNGLVERTEXATTRIB4FPROC) (GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
typedef void (GL_APIENTRYP PFNGLVERTEXATTRIB4FVPROC) (GLuint index, const GLfloat *v);
typedef void (GL_APIENTRYP PFNGLVERTEXATTRIBPOINTERPROC) (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);
typedef void (GL_APIENTRYP PFNGLVIEWPORTPROC) (GLint x, GLint y, GLsizei width, GLsizei height);
//GLES3 pfns
typedef void (GL_APIENTRYP PFNGLREADBUFFERPROC) (GLenum src);
typedef void (GL_APIENTRYP PFNGLDRAWRANGEELEMENTSPROC) (GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices);
typedef void (GL_APIENTRYP PFNGLTEXIMAGE3DPROC) (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels);
typedef void (GL_APIENTRYP PFNGLTEXSUBIMAGE3DPROC) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels);
typedef void (GL_APIENTRYP PFNGLCOPYTEXSUBIMAGE3DPROC) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height);
typedef void (GL_APIENTRYP PFNGLCOMPRESSEDTEXIMAGE3DPROC) (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *data);
typedef void (GL_APIENTRYP PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data);
typedef void (GL_APIENTRYP PFNGLGENQUERIESPROC) (GLsizei n, GLuint *ids);
typedef void (GL_APIENTRYP PFNGLDELETEQUERIESPROC) (GLsizei n, const GLuint *ids);
typedef GLboolean (GL_APIENTRYP PFNGLISQUERYPROC) (GLuint id);
typedef void (GL_APIENTRYP PFNGLBEGINQUERYPROC) (GLenum target, GLuint id);
typedef void (GL_APIENTRYP PFNGLENDQUERYPROC) (GLenum target);
typedef void (GL_APIENTRYP PFNGLGETQUERYIVPROC) (GLenum target, GLenum pname, GLint *params);
typedef void (GL_APIENTRYP PFNGLGETQUERYOBJECTUIVPROC) (GLuint id, GLenum pname, GLuint *params);
typedef GLboolean (GL_APIENTRYP PFNGLUNMAPBUFFERPROC) (GLenum target);
typedef void (GL_APIENTRYP PFNGLGETBUFFERPOINTERVPROC) (GLenum target, GLenum pname, void **params);
typedef void (GL_APIENTRYP PFNGLDRAWBUFFERSPROC) (GLsizei n, const GLenum *bufs);
typedef void (GL_APIENTRYP PFNGLUNIFORMMATRIX2X3FVPROC) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void (GL_APIENTRYP PFNGLUNIFORMMATRIX3X2FVPROC) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void (GL_APIENTRYP PFNGLUNIFORMMATRIX2X4FVPROC) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void (GL_APIENTRYP PFNGLUNIFORMMATRIX4X2FVPROC) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void (GL_APIENTRYP PFNGLUNIFORMMATRIX3X4FVPROC) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void (GL_APIENTRYP PFNGLUNIFORMMATRIX4X3FVPROC) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void (GL_APIENTRYP PFNGLBLITFRAMEBUFFERPROC) (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
typedef void (GL_APIENTRYP PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC) (GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
typedef void (GL_APIENTRYP PFNGLFRAMEBUFFERTEXTURELAYERPROC) (GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer);
typedef void *(GL_APIENTRYP PFNGLMAPBUFFERRANGEPROC) (GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access);
typedef void (GL_APIENTRYP PFNGLFLUSHMAPPEDBUFFERRANGEPROC) (GLenum target, GLintptr offset, GLsizeiptr length);
typedef void (GL_APIENTRYP PFNGLBINDVERTEXARRAYPROC) (GLuint array);
typedef void (GL_APIENTRYP PFNGLDELETEVERTEXARRAYSPROC) (GLsizei n, const GLuint *arrays);
typedef void (GL_APIENTRYP PFNGLGENVERTEXARRAYSPROC) (GLsizei n, GLuint *arrays);
typedef GLboolean (GL_APIENTRYP PFNGLISVERTEXARRAYPROC) (GLuint array);
typedef void (GL_APIENTRYP PFNGLGETINTEGERI_VPROC) (GLenum target, GLuint index, GLint *data);
typedef void (GL_APIENTRYP PFNGLBEGINTRANSFORMFEEDBACKPROC) (GLenum primitiveMode);
typedef void (GL_APIENTRYP PFNGLENDTRANSFORMFEEDBACKPROC) (void);
typedef void (GL_APIENTRYP PFNGLBINDBUFFERRANGEPROC) (GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size);
typedef void (GL_APIENTRYP PFNGLBINDBUFFERBASEPROC) (GLenum target, GLuint index, GLuint buffer);
typedef void (GL_APIENTRYP PFNGLTRANSFORMFEEDBACKVARYINGSPROC) (GLuint program, GLsizei count, const GLchar *const*varyings, GLenum bufferMode);
typedef void (GL_APIENTRYP PFNGLGETTRANSFORMFEEDBACKVARYINGPROC) (GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLsizei *size, GLenum *type, GLchar *name);
typedef void (GL_APIENTRYP PFNGLVERTEXATTRIBIPOINTERPROC) (GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer);
typedef void (GL_APIENTRYP PFNGLGETVERTEXATTRIBIIVPROC) (GLuint index, GLenum pname, GLint *params);
typedef void (GL_APIENTRYP PFNGLGETVERTEXATTRIBIUIVPROC) (GLuint index, GLenum pname, GLuint *params);
typedef void (GL_APIENTRYP PFNGLVERTEXATTRIBI4IPROC) (GLuint index, GLint x, GLint y, GLint z, GLint w);
typedef void (GL_APIENTRYP PFNGLVERTEXATTRIBI4UIPROC) (GLuint index, GLuint x, GLuint y, GLuint z, GLuint w);
typedef void (GL_APIENTRYP PFNGLVERTEXATTRIBI4IVPROC) (GLuint index, const GLint *v);
typedef void (GL_APIENTRYP PFNGLVERTEXATTRIBI4UIVPROC) (GLuint index, const GLuint *v);
typedef void (GL_APIENTRYP PFNGLGETUNIFORMUIVPROC) (GLuint program, GLint location, GLuint *params);
typedef GLint (GL_APIENTRYP PFNGLGETFRAGDATALOCATIONPROC) (GLuint program, const GLchar *name);
typedef void (GL_APIENTRYP PFNGLUNIFORM1UIPROC) (GLint location, GLuint v0);
typedef void (GL_APIENTRYP PFNGLUNIFORM2UIPROC) (GLint location, GLuint v0, GLuint v1);
typedef void (GL_APIENTRYP PFNGLUNIFORM3UIPROC) (GLint location, GLuint v0, GLuint v1, GLuint v2);
typedef void (GL_APIENTRYP PFNGLUNIFORM4UIPROC) (GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3);
typedef void (GL_APIENTRYP PFNGLUNIFORM1UIVPROC) (GLint location, GLsizei count, const GLuint *value);
typedef void (GL_APIENTRYP PFNGLUNIFORM2UIVPROC) (GLint location, GLsizei count, const GLuint *value);
typedef void (GL_APIENTRYP PFNGLUNIFORM3UIVPROC) (GLint location, GLsizei count, const GLuint *value);
typedef void (GL_APIENTRYP PFNGLUNIFORM4UIVPROC) (GLint location, GLsizei count, const GLuint *value);
typedef void (GL_APIENTRYP PFNGLCLEARBUFFERIVPROC) (GLenum buffer, GLint drawbuffer, const GLint *value);
typedef void (GL_APIENTRYP PFNGLCLEARBUFFERUIVPROC) (GLenum buffer, GLint drawbuffer, const GLuint *value);
typedef void (GL_APIENTRYP PFNGLCLEARBUFFERFVPROC) (GLenum buffer, GLint drawbuffer, const GLfloat *value);
typedef void (GL_APIENTRYP PFNGLCLEARBUFFERFIPROC) (GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil);
typedef const GLubyte *(GL_APIENTRYP PFNGLGETSTRINGIPROC) (GLenum name, GLuint index);
typedef void (GL_APIENTRYP PFNGLCOPYBUFFERSUBDATAPROC) (GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size);
typedef void (GL_APIENTRYP PFNGLGETUNIFORMINDICESPROC) (GLuint program, GLsizei uniformCount, const GLchar *const*uniformNames, GLuint *uniformIndices);
typedef void (GL_APIENTRYP PFNGLGETACTIVEUNIFORMSIVPROC) (GLuint program, GLsizei uniformCount, const GLuint *uniformIndices, GLenum pname, GLint *params);
typedef GLuint (GL_APIENTRYP PFNGLGETUNIFORMBLOCKINDEXPROC) (GLuint program, const GLchar *uniformBlockName);
typedef void (GL_APIENTRYP PFNGLGETACTIVEUNIFORMBLOCKIVPROC) (GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint *params);
typedef void (GL_APIENTRYP PFNGLGETACTIVEUNIFORMBLOCKNAMEPROC) (GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei *length, GLchar *uniformBlockName);
typedef void (GL_APIENTRYP PFNGLUNIFORMBLOCKBINDINGPROC) (GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding);
typedef void (GL_APIENTRYP PFNGLDRAWARRAYSINSTANCEDPROC) (GLenum mode, GLint first, GLsizei count, GLsizei instancecount);
typedef void (GL_APIENTRYP PFNGLDRAWELEMENTSINSTANCEDPROC) (GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount);
typedef GLsync (GL_APIENTRYP PFNGLFENCESYNCPROC) (GLenum condition, GLbitfield flags);
typedef GLboolean (GL_APIENTRYP PFNGLISSYNCPROC) (GLsync sync);
typedef void (GL_APIENTRYP PFNGLDELETESYNCPROC) (GLsync sync);
typedef GLenum (GL_APIENTRYP PFNGLCLIENTWAITSYNCPROC) (GLsync sync, GLbitfield flags, GLuint64 timeout);
typedef void (GL_APIENTRYP PFNGLWAITSYNCPROC) (GLsync sync, GLbitfield flags, GLuint64 timeout);
typedef void (GL_APIENTRYP PFNGLGETINTEGER64VPROC) (GLenum pname, GLint64 *data);
typedef void (GL_APIENTRYP PFNGLGETSYNCIVPROC) (GLsync sync, GLenum pname, GLsizei bufSize, GLsizei *length, GLint *values);
typedef void (GL_APIENTRYP PFNGLGETINTEGER64I_VPROC) (GLenum target, GLuint index, GLint64 *data);
typedef void (GL_APIENTRYP PFNGLGETBUFFERPARAMETERI64VPROC) (GLenum target, GLenum pname, GLint64 *params);
typedef void (GL_APIENTRYP PFNGLGENSAMPLERSPROC) (GLsizei count, GLuint *samplers);
typedef void (GL_APIENTRYP PFNGLDELETESAMPLERSPROC) (GLsizei count, const GLuint *samplers);
typedef GLboolean (GL_APIENTRYP PFNGLISSAMPLERPROC) (GLuint sampler);
typedef void (GL_APIENTRYP PFNGLBINDSAMPLERPROC) (GLuint unit, GLuint sampler);
typedef void (GL_APIENTRYP PFNGLSAMPLERPARAMETERIPROC) (GLuint sampler, GLenum pname, GLint param);
typedef void (GL_APIENTRYP PFNGLSAMPLERPARAMETERIVPROC) (GLuint sampler, GLenum pname, const GLint *param);
typedef void (GL_APIENTRYP PFNGLSAMPLERPARAMETERFPROC) (GLuint sampler, GLenum pname, GLfloat param);
typedef void (GL_APIENTRYP PFNGLSAMPLERPARAMETERFVPROC) (GLuint sampler, GLenum pname, const GLfloat *param);
typedef void (GL_APIENTRYP PFNGLGETSAMPLERPARAMETERIVPROC) (GLuint sampler, GLenum pname, GLint *params);
typedef void (GL_APIENTRYP PFNGLGETSAMPLERPARAMETERFVPROC) (GLuint sampler, GLenum pname, GLfloat *params);
typedef void (GL_APIENTRYP PFNGLVERTEXATTRIBDIVISORPROC) (GLuint index, GLuint divisor);
typedef void (GL_APIENTRYP PFNGLBINDTRANSFORMFEEDBACKPROC) (GLenum target, GLuint id);
typedef void (GL_APIENTRYP PFNGLDELETETRANSFORMFEEDBACKSPROC) (GLsizei n, const GLuint *ids);
typedef void (GL_APIENTRYP PFNGLGENTRANSFORMFEEDBACKSPROC) (GLsizei n, GLuint *ids);
typedef GLboolean (GL_APIENTRYP PFNGLISTRANSFORMFEEDBACKPROC) (GLuint id);
typedef void (GL_APIENTRYP PFNGLPAUSETRANSFORMFEEDBACKPROC) (void);
typedef void (GL_APIENTRYP PFNGLRESUMETRANSFORMFEEDBACKPROC) (void);
typedef void (GL_APIENTRYP PFNGLGETPROGRAMBINARYPROC) (GLuint program, GLsizei bufSize, GLsizei *length, GLenum *binaryFormat, void *binary);
typedef void (GL_APIENTRYP PFNGLPROGRAMBINARYPROC) (GLuint program, GLenum binaryFormat, const void *binary, GLsizei length);
typedef void (GL_APIENTRYP PFNGLPROGRAMPARAMETERIPROC) (GLuint program, GLenum pname, GLint value);
typedef void (GL_APIENTRYP PFNGLINVALIDATEFRAMEBUFFERPROC) (GLenum target, GLsizei numAttachments, const GLenum *attachments);
typedef void (GL_APIENTRYP PFNGLINVALIDATESUBFRAMEBUFFERPROC) (GLenum target, GLsizei numAttachments, const GLenum *attachments, GLint x, GLint y, GLsizei width, GLsizei height);
typedef void (GL_APIENTRYP PFNGLTEXSTORAGE2DPROC) (GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height);
typedef void (GL_APIENTRYP PFNGLTEXSTORAGE3DPROC) (GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth);
typedef void (GL_APIENTRYP PFNGLGETINTERNALFORMATIVPROC) (GLenum target, GLenum internalformat, GLenum pname, GLsizei bufSize, GLint *params);





//GLES2
#define BTK_OPENGLES2_FUNCTIONS_PART \
    BTK_GL_PROCESS(PFNGLACTIVETEXTUREPROC,glActiveTexture) \
    BTK_GL_PROCESS(PFNGLATTACHSHADERPROC,glAttachShader) \
    BTK_GL_PROCESS(PFNGLBINDATTRIBLOCATIONPROC,glBindAttribLocation) \
    BTK_GL_PROCESS(PFNGLBINDBUFFERPROC,glBindBuffer) \
    BTK_GL_PROCESS(PFNGLBINDFRAMEBUFFERPROC,glBindFramebuffer) \
    BTK_GL_PROCESS(PFNGLBINDRENDERBUFFERPROC,glBindRenderbuffer) \
    BTK_GL_PROCESS(PFNGLBINDTEXTUREPROC,glBindTexture) \
    BTK_GL_PROCESS(PFNGLBLENDCOLORPROC,glBlendColor) \
    BTK_GL_PROCESS(PFNGLBLENDEQUATIONPROC,glBlendEquation) \
    BTK_GL_PROCESS(PFNGLBLENDEQUATIONSEPARATEPROC,glBlendEquationSeparate) \
    BTK_GL_PROCESS(PFNGLBLENDFUNCPROC,glBlendFunc) \
    BTK_GL_PROCESS(PFNGLBLENDFUNCSEPARATEPROC,glBlendFuncSeparate) \
    BTK_GL_PROCESS(PFNGLBUFFERDATAPROC,glBufferData) \
    BTK_GL_PROCESS(PFNGLBUFFERSUBDATAPROC,glBufferSubData) \
    BTK_GL_PROCESS(PFNGLCHECKFRAMEBUFFERSTATUSPROC,glCheckFramebufferStatus) \
    BTK_GL_PROCESS(PFNGLCLEARPROC,glClear) \
    BTK_GL_PROCESS(PFNGLCLEARCOLORPROC,glClearColor) \
    BTK_GL_PROCESS(PFNGLCLEARDEPTHFPROC,glClearDepthf) \
    BTK_GL_PROCESS(PFNGLCLEARSTENCILPROC,glClearStencil) \
    BTK_GL_PROCESS(PFNGLCOLORMASKPROC,glColorMask) \
    BTK_GL_PROCESS(PFNGLCOMPILESHADERPROC,glCompileShader) \
    BTK_GL_PROCESS(PFNGLCOMPRESSEDTEXIMAGE2DPROC,glCompressedTexImage2D) \
    BTK_GL_PROCESS(PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC,glCompressedTexSubImage2D) \
    BTK_GL_PROCESS(PFNGLCOPYTEXIMAGE2DPROC,glCopyTexImage2D) \
    BTK_GL_PROCESS(PFNGLCOPYTEXSUBIMAGE2DPROC,glCopyTexSubImage2D) \
    BTK_GL_PROCESS(PFNGLCREATEPROGRAMPROC,glCreateProgram) \
    BTK_GL_PROCESS(PFNGLCREATESHADERPROC,glCreateShader) \
    BTK_GL_PROCESS(PFNGLCULLFACEPROC,glCullFace) \
    BTK_GL_PROCESS(PFNGLDELETEBUFFERSPROC,glDeleteBuffers) \
    BTK_GL_PROCESS(PFNGLDELETEFRAMEBUFFERSPROC,glDeleteFramebuffers) \
    BTK_GL_PROCESS(PFNGLDELETEPROGRAMPROC,glDeleteProgram) \
    BTK_GL_PROCESS(PFNGLDELETERENDERBUFFERSPROC,glDeleteRenderbuffers) \
    BTK_GL_PROCESS(PFNGLDELETESHADERPROC,glDeleteShader) \
    BTK_GL_PROCESS(PFNGLDELETETEXTURESPROC,glDeleteTextures) \
    BTK_GL_PROCESS(PFNGLDEPTHFUNCPROC,glDepthFunc) \
    BTK_GL_PROCESS(PFNGLDEPTHMASKPROC,glDepthMask) \
    BTK_GL_PROCESS(PFNGLDEPTHRANGEFPROC,glDepthRangef) \
    BTK_GL_PROCESS(PFNGLDETACHSHADERPROC,glDetachShader) \
    BTK_GL_PROCESS(PFNGLDISABLEPROC,glDisable) \
    BTK_GL_PROCESS(PFNGLDISABLEVERTEXATTRIBARRAYPROC,glDisableVertexAttribArray) \
    BTK_GL_PROCESS(PFNGLDRAWARRAYSPROC,glDrawArrays) \
    BTK_GL_PROCESS(PFNGLDRAWELEMENTSPROC,glDrawElements) \
    BTK_GL_PROCESS(PFNGLENABLEPROC,glEnable) \
    BTK_GL_PROCESS(PFNGLENABLEVERTEXATTRIBARRAYPROC,glEnableVertexAttribArray) \
    BTK_GL_PROCESS(PFNGLFINISHPROC,glFinish) \
    BTK_GL_PROCESS(PFNGLFLUSHPROC,glFlush) \
    BTK_GL_PROCESS(PFNGLFRAMEBUFFERRENDERBUFFERPROC,glFramebufferRenderbuffer) \
    BTK_GL_PROCESS(PFNGLFRAMEBUFFERTEXTURE2DPROC,glFramebufferTexture2D) \
    BTK_GL_PROCESS(PFNGLFRONTFACEPROC,glFrontFace) \
    BTK_GL_PROCESS(PFNGLGENBUFFERSPROC,glGenBuffers) \
    BTK_GL_PROCESS(PFNGLGENERATEMIPMAPPROC,glGenerateMipmap) \
    BTK_GL_PROCESS(PFNGLGENFRAMEBUFFERSPROC,glGenFramebuffers) \
    BTK_GL_PROCESS(PFNGLGENRENDERBUFFERSPROC,glGenRenderbuffers) \
    BTK_GL_PROCESS(PFNGLGENTEXTURESPROC,glGenTextures) \
    BTK_GL_PROCESS(PFNGLGETACTIVEATTRIBPROC,glGetActiveAttrib) \
    BTK_GL_PROCESS(PFNGLGETACTIVEUNIFORMPROC,glGetActiveUniform) \
    BTK_GL_PROCESS(PFNGLGETATTACHEDSHADERSPROC,glGetAttachedShaders) \
    BTK_GL_PROCESS(PFNGLGETATTRIBLOCATIONPROC,glGetAttribLocation) \
    BTK_GL_PROCESS(PFNGLGETBOOLEANVPROC,glGetBooleanv) \
    BTK_GL_PROCESS(PFNGLGETBUFFERPARAMETERIVPROC,glGetBufferParameteriv) \
    BTK_GL_PROCESS(PFNGLGETERRORPROC,glGetError) \
    BTK_GL_PROCESS(PFNGLGETFLOATVPROC,glGetFloatv) \
    BTK_GL_PROCESS(PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC,glGetFramebufferAttachmentParameteriv) \
    BTK_GL_PROCESS(PFNGLGETINTEGERVPROC,glGetIntegerv) \
    BTK_GL_PROCESS(PFNGLGETPROGRAMIVPROC,glGetProgramiv) \
    BTK_GL_PROCESS(PFNGLGETPROGRAMINFOLOGPROC,glGetProgramInfoLog) \
    BTK_GL_PROCESS(PFNGLGETRENDERBUFFERPARAMETERIVPROC,glGetRenderbufferParameteriv) \
    BTK_GL_PROCESS(PFNGLGETSHADERIVPROC,glGetShaderiv) \
    BTK_GL_PROCESS(PFNGLGETSHADERINFOLOGPROC,glGetShaderInfoLog) \
    BTK_GL_PROCESS(PFNGLGETSHADERPRECISIONFORMATPROC,glGetShaderPrecisionFormat) \
    BTK_GL_PROCESS(PFNGLGETSHADERSOURCEPROC,glGetShaderSource) \
    BTK_GL_PROCESS(PFNGLGETSTRINGPROC,glGetString) \
    BTK_GL_PROCESS(PFNGLGETTEXPARAMETERFVPROC,glGetTexParameterfv) \
    BTK_GL_PROCESS(PFNGLGETTEXPARAMETERIVPROC,glGetTexParameteriv) \
    BTK_GL_PROCESS(PFNGLGETUNIFORMFVPROC,glGetUniformfv) \
    BTK_GL_PROCESS(PFNGLGETUNIFORMIVPROC,glGetUniformiv) \
    BTK_GL_PROCESS(PFNGLGETUNIFORMLOCATIONPROC,glGetUniformLocation) \
    BTK_GL_PROCESS(PFNGLGETVERTEXATTRIBFVPROC,glGetVertexAttribfv) \
    BTK_GL_PROCESS(PFNGLGETVERTEXATTRIBIVPROC,glGetVertexAttribiv) \
    BTK_GL_PROCESS(PFNGLGETVERTEXATTRIBPOINTERVPROC,glGetVertexAttribPointerv) \
    BTK_GL_PROCESS(PFNGLHINTPROC,glHint) \
    BTK_GL_PROCESS(PFNGLISBUFFERPROC,glIsBuffer) \
    BTK_GL_PROCESS(PFNGLISENABLEDPROC,glIsEnabled) \
    BTK_GL_PROCESS(PFNGLISFRAMEBUFFERPROC,glIsFramebuffer) \
    BTK_GL_PROCESS(PFNGLISPROGRAMPROC,glIsProgram) \
    BTK_GL_PROCESS(PFNGLISRENDERBUFFERPROC,glIsRenderbuffer) \
    BTK_GL_PROCESS(PFNGLISSHADERPROC,glIsShader) \
    BTK_GL_PROCESS(PFNGLISTEXTUREPROC,glIsTexture) \
    BTK_GL_PROCESS(PFNGLLINEWIDTHPROC,glLineWidth) \
    BTK_GL_PROCESS(PFNGLLINKPROGRAMPROC,glLinkProgram) \
    BTK_GL_PROCESS(PFNGLPIXELSTOREIPROC,glPixelStorei) \
    BTK_GL_PROCESS(PFNGLPOLYGONOFFSETPROC,glPolygonOffset) \
    BTK_GL_PROCESS(PFNGLREADPIXELSPROC,glReadPixels) \
    BTK_GL_PROCESS(PFNGLRELEASESHADERCOMPILERPROC,glReleaseShaderCompiler) \
    BTK_GL_PROCESS(PFNGLRENDERBUFFERSTORAGEPROC,glRenderbufferStorage) \
    BTK_GL_PROCESS(PFNGLSAMPLECOVERAGEPROC,glSampleCoverage) \
    BTK_GL_PROCESS(PFNGLSCISSORPROC,glScissor) \
    BTK_GL_PROCESS(PFNGLSHADERBINARYPROC,glShaderBinary) \
    BTK_GL_PROCESS(PFNGLSHADERSOURCEPROC,glShaderSource) \
    BTK_GL_PROCESS(PFNGLSTENCILFUNCPROC,glStencilFunc) \
    BTK_GL_PROCESS(PFNGLSTENCILFUNCSEPARATEPROC,glStencilFuncSeparate) \
    BTK_GL_PROCESS(PFNGLSTENCILMASKPROC,glStencilMask) \
    BTK_GL_PROCESS(PFNGLSTENCILMASKSEPARATEPROC,glStencilMaskSeparate) \
    BTK_GL_PROCESS(PFNGLSTENCILOPPROC,glStencilOp) \
    BTK_GL_PROCESS(PFNGLSTENCILOPSEPARATEPROC,glStencilOpSeparate) \
    BTK_GL_PROCESS(PFNGLTEXIMAGE2DPROC,glTexImage2D) \
    BTK_GL_PROCESS(PFNGLTEXPARAMETERFPROC,glTexParameterf) \
    BTK_GL_PROCESS(PFNGLTEXPARAMETERFVPROC,glTexParameterfv) \
    BTK_GL_PROCESS(PFNGLTEXPARAMETERIPROC,glTexParameteri) \
    BTK_GL_PROCESS(PFNGLTEXPARAMETERIVPROC,glTexParameteriv) \
    BTK_GL_PROCESS(PFNGLTEXSUBIMAGE2DPROC,glTexSubImage2D) \
    BTK_GL_PROCESS(PFNGLUNIFORM1FPROC,glUniform1f) \
    BTK_GL_PROCESS(PFNGLUNIFORM1FVPROC,glUniform1fv) \
    BTK_GL_PROCESS(PFNGLUNIFORM1IPROC,glUniform1i) \
    BTK_GL_PROCESS(PFNGLUNIFORM1IVPROC,glUniform1iv) \
    BTK_GL_PROCESS(PFNGLUNIFORM2FPROC,glUniform2f) \
    BTK_GL_PROCESS(PFNGLUNIFORM2FVPROC,glUniform2fv) \
    BTK_GL_PROCESS(PFNGLUNIFORM2IPROC,glUniform2i) \
    BTK_GL_PROCESS(PFNGLUNIFORM2IVPROC,glUniform2iv) \
    BTK_GL_PROCESS(PFNGLUNIFORM3FPROC,glUniform3f) \
    BTK_GL_PROCESS(PFNGLUNIFORM3FVPROC,glUniform3fv) \
    BTK_GL_PROCESS(PFNGLUNIFORM3IPROC,glUniform3i) \
    BTK_GL_PROCESS(PFNGLUNIFORM3IVPROC,glUniform3iv) \
    BTK_GL_PROCESS(PFNGLUNIFORM4FPROC,glUniform4f) \
    BTK_GL_PROCESS(PFNGLUNIFORM4FVPROC,glUniform4fv) \
    BTK_GL_PROCESS(PFNGLUNIFORM4IPROC,glUniform4i) \
    BTK_GL_PROCESS(PFNGLUNIFORM4IVPROC,glUniform4iv) \
    BTK_GL_PROCESS(PFNGLUNIFORMMATRIX2FVPROC,glUniformMatrix2fv) \
    BTK_GL_PROCESS(PFNGLUNIFORMMATRIX3FVPROC,glUniformMatrix3fv) \
    BTK_GL_PROCESS(PFNGLUNIFORMMATRIX4FVPROC,glUniformMatrix4fv) \
    BTK_GL_PROCESS(PFNGLUSEPROGRAMPROC,glUseProgram) \
    BTK_GL_PROCESS(PFNGLVALIDATEPROGRAMPROC,glValidateProgram) \
    BTK_GL_PROCESS(PFNGLVERTEXATTRIB1FPROC,glVertexAttrib1f) \
    BTK_GL_PROCESS(PFNGLVERTEXATTRIB1FVPROC,glVertexAttrib1fv) \
    BTK_GL_PROCESS(PFNGLVERTEXATTRIB2FPROC,glVertexAttrib2f) \
    BTK_GL_PROCESS(PFNGLVERTEXATTRIB2FVPROC,glVertexAttrib2fv) \
    BTK_GL_PROCESS(PFNGLVERTEXATTRIB3FPROC,glVertexAttrib3f) \
    BTK_GL_PROCESS(PFNGLVERTEXATTRIB3FVPROC,glVertexAttrib3fv) \
    BTK_GL_PROCESS(PFNGLVERTEXATTRIB4FPROC,glVertexAttrib4f) \
    BTK_GL_PROCESS(PFNGLVERTEXATTRIB4FVPROC,glVertexAttrib4fv) \
    BTK_GL_PROCESS(PFNGLVERTEXATTRIBPOINTERPROC,glVertexAttribPointer) \
//GLES3
#define BTK_OPENGLES3_FUNCTIONS_PART \
    BTK_GL_PROCESS(PFNGLVIEWPORTPROC,glViewport) \
    BTK_GL_PROCESS(PFNGLREADBUFFERPROC,glReadBuffer) \
    BTK_GL_PROCESS(PFNGLDRAWRANGEELEMENTSPROC,glDrawRangeElements) \
    BTK_GL_PROCESS(PFNGLTEXIMAGE3DPROC,glTexImage3D) \
    BTK_GL_PROCESS(PFNGLTEXSUBIMAGE3DPROC,glTexSubImage3D) \
    BTK_GL_PROCESS(PFNGLCOPYTEXSUBIMAGE3DPROC,glCopyTexSubImage3D) \
    BTK_GL_PROCESS(PFNGLCOMPRESSEDTEXIMAGE3DPROC,glCompressedTexImage3D) \
    BTK_GL_PROCESS(PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC,glCompressedTexSubImage3D) \
    BTK_GL_PROCESS(PFNGLGENQUERIESPROC,glGenQueries) \
    BTK_GL_PROCESS(PFNGLDELETEQUERIESPROC,glDeleteQueries) \
    BTK_GL_PROCESS(PFNGLISQUERYPROC,glIsQuery) \
    BTK_GL_PROCESS(PFNGLBEGINQUERYPROC,glBeginQuery) \
    BTK_GL_PROCESS(PFNGLENDQUERYPROC,glEndQuery) \
    BTK_GL_PROCESS(PFNGLGETQUERYIVPROC,glGetQueryiv) \
    BTK_GL_PROCESS(PFNGLGETQUERYOBJECTUIVPROC,glGetQueryObjectuiv) \
    BTK_GL_PROCESS(PFNGLUNMAPBUFFERPROC,glUnmapBuffer) \
    BTK_GL_PROCESS(PFNGLGETBUFFERPOINTERVPROC,glGetBufferPointerv) \
    BTK_GL_PROCESS(PFNGLDRAWBUFFERSPROC,glDrawBuffers) \
    BTK_GL_PROCESS(PFNGLUNIFORMMATRIX2X3FVPROC,glUniformMatrix2x3fv) \
    BTK_GL_PROCESS(PFNGLUNIFORMMATRIX3X2FVPROC,glUniformMatrix3x2fv) \
    BTK_GL_PROCESS(PFNGLUNIFORMMATRIX2X4FVPROC,glUniformMatrix2x4fv) \
    BTK_GL_PROCESS(PFNGLUNIFORMMATRIX4X2FVPROC,glUniformMatrix4x2fv) \
    BTK_GL_PROCESS(PFNGLUNIFORMMATRIX3X4FVPROC,glUniformMatrix3x4fv) \
    BTK_GL_PROCESS(PFNGLUNIFORMMATRIX4X3FVPROC,glUniformMatrix4x3fv) \
    BTK_GL_PROCESS(PFNGLBLITFRAMEBUFFERPROC,glBlitFramebuffer) \
    BTK_GL_PROCESS(PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC,glRenderbufferStorageMultisample) \
    BTK_GL_PROCESS(PFNGLFRAMEBUFFERTEXTURELAYERPROC,glFramebufferTextureLayer) \
    BTK_GL_PROCESS(PFNGLMAPBUFFERRANGEPROC,glMapBufferRange) \
    BTK_GL_PROCESS(PFNGLFLUSHMAPPEDBUFFERRANGEPROC,glFlushMappedBufferRange) \
    BTK_GL_PROCESS(PFNGLBINDVERTEXARRAYPROC,glBindVertexArray) \
    BTK_GL_PROCESS(PFNGLDELETEVERTEXARRAYSPROC,glDeleteVertexArrays) \
    BTK_GL_PROCESS(PFNGLGENVERTEXARRAYSPROC,glGenVertexArrays) \
    BTK_GL_PROCESS(PFNGLISVERTEXARRAYPROC,glIsVertexArray) \
    BTK_GL_PROCESS(PFNGLGETINTEGERI_VPROC,glGetIntegeri_v) \
    BTK_GL_PROCESS(PFNGLBEGINTRANSFORMFEEDBACKPROC,glBeginTransformFeedback) \
    BTK_GL_PROCESS(PFNGLENDTRANSFORMFEEDBACKPROC,glEndTransformFeedback) \
    BTK_GL_PROCESS(PFNGLBINDBUFFERRANGEPROC,glBindBufferRange) \
    BTK_GL_PROCESS(PFNGLBINDBUFFERBASEPROC,glBindBufferBase) \
    BTK_GL_PROCESS(PFNGLTRANSFORMFEEDBACKVARYINGSPROC,glTransformFeedbackVaryings) \
    BTK_GL_PROCESS(PFNGLGETTRANSFORMFEEDBACKVARYINGPROC,glGetTransformFeedbackVarying) \
    BTK_GL_PROCESS(PFNGLVERTEXATTRIBIPOINTERPROC,glVertexAttribIPointer) \
    BTK_GL_PROCESS(PFNGLGETVERTEXATTRIBIIVPROC,glGetVertexAttribIiv) \
    BTK_GL_PROCESS(PFNGLGETVERTEXATTRIBIUIVPROC,glGetVertexAttribIuiv) \
    BTK_GL_PROCESS(PFNGLVERTEXATTRIBI4IPROC,glVertexAttribI4i) \
    BTK_GL_PROCESS(PFNGLVERTEXATTRIBI4UIPROC,glVertexAttribI4ui) \
    BTK_GL_PROCESS(PFNGLVERTEXATTRIBI4IVPROC,glVertexAttribI4iv) \
    BTK_GL_PROCESS(PFNGLVERTEXATTRIBI4UIVPROC,glVertexAttribI4uiv) \
    BTK_GL_PROCESS(PFNGLGETUNIFORMUIVPROC,glGetUniformuiv) \
    BTK_GL_PROCESS(PFNGLGETFRAGDATALOCATIONPROC,glGetFragDataLocation) \
    BTK_GL_PROCESS(PFNGLUNIFORM1UIPROC,glUniform1ui) \
    BTK_GL_PROCESS(PFNGLUNIFORM2UIPROC,glUniform2ui) \
    BTK_GL_PROCESS(PFNGLUNIFORM3UIPROC,glUniform3ui) \
    BTK_GL_PROCESS(PFNGLUNIFORM4UIPROC,glUniform4ui) \
    BTK_GL_PROCESS(PFNGLUNIFORM1UIVPROC,glUniform1uiv) \
    BTK_GL_PROCESS(PFNGLUNIFORM2UIVPROC,glUniform2uiv) \
    BTK_GL_PROCESS(PFNGLUNIFORM3UIVPROC,glUniform3uiv) \
    BTK_GL_PROCESS(PFNGLUNIFORM4UIVPROC,glUniform4uiv) \
    BTK_GL_PROCESS(PFNGLCLEARBUFFERIVPROC,glClearBufferiv) \
    BTK_GL_PROCESS(PFNGLCLEARBUFFERUIVPROC,glClearBufferuiv) \
    BTK_GL_PROCESS(PFNGLCLEARBUFFERFVPROC,glClearBufferfv) \
    BTK_GL_PROCESS(PFNGLCLEARBUFFERFIPROC,glClearBufferfi) \
    BTK_GL_PROCESS(PFNGLGETSTRINGIPROC,glGetStringi) \
    BTK_GL_PROCESS(PFNGLCOPYBUFFERSUBDATAPROC,glCopyBufferSubData) \
    BTK_GL_PROCESS(PFNGLGETUNIFORMINDICESPROC,glGetUniformIndices) \
    BTK_GL_PROCESS(PFNGLGETACTIVEUNIFORMSIVPROC,glGetActiveUniformsiv) \
    BTK_GL_PROCESS(PFNGLGETUNIFORMBLOCKINDEXPROC,glGetUniformBlockIndex) \
    BTK_GL_PROCESS(PFNGLGETACTIVEUNIFORMBLOCKIVPROC,glGetActiveUniformBlockiv) \
    BTK_GL_PROCESS(PFNGLGETACTIVEUNIFORMBLOCKNAMEPROC,glGetActiveUniformBlockName) \
    BTK_GL_PROCESS(PFNGLUNIFORMBLOCKBINDINGPROC,glUniformBlockBinding) \
    BTK_GL_PROCESS(PFNGLDRAWARRAYSINSTANCEDPROC,glDrawArraysInstanced) \
    BTK_GL_PROCESS(PFNGLDRAWELEMENTSINSTANCEDPROC,glDrawElementsInstanced) \
    BTK_GL_PROCESS(PFNGLFENCESYNCPROC,glFenceSync) \
    BTK_GL_PROCESS(PFNGLISSYNCPROC,glIsSync) \
    BTK_GL_PROCESS(PFNGLDELETESYNCPROC,glDeleteSync) \
    BTK_GL_PROCESS(PFNGLCLIENTWAITSYNCPROC,glClientWaitSync) \
    BTK_GL_PROCESS(PFNGLWAITSYNCPROC,glWaitSync) \
    BTK_GL_PROCESS(PFNGLGETINTEGER64VPROC,glGetInteger64v) \
    BTK_GL_PROCESS(PFNGLGETSYNCIVPROC,glGetSynciv) \
    BTK_GL_PROCESS(PFNGLGETINTEGER64I_VPROC,glGetInteger64i_v) \
    BTK_GL_PROCESS(PFNGLGETBUFFERPARAMETERI64VPROC,glGetBufferParameteri64v) \
    BTK_GL_PROCESS(PFNGLGENSAMPLERSPROC,glGenSamplers) \
    BTK_GL_PROCESS(PFNGLDELETESAMPLERSPROC,glDeleteSamplers) \
    BTK_GL_PROCESS(PFNGLISSAMPLERPROC,glIsSampler) \
    BTK_GL_PROCESS(PFNGLBINDSAMPLERPROC,glBindSampler) \
    BTK_GL_PROCESS(PFNGLSAMPLERPARAMETERIPROC,glSamplerParameteri) \
    BTK_GL_PROCESS(PFNGLSAMPLERPARAMETERIVPROC,glSamplerParameteriv) \
    BTK_GL_PROCESS(PFNGLSAMPLERPARAMETERFPROC,glSamplerParameterf) \
    BTK_GL_PROCESS(PFNGLSAMPLERPARAMETERFVPROC,glSamplerParameterfv) \
    BTK_GL_PROCESS(PFNGLGETSAMPLERPARAMETERIVPROC,glGetSamplerParameteriv) \
    BTK_GL_PROCESS(PFNGLGETSAMPLERPARAMETERFVPROC,glGetSamplerParameterfv) \
    BTK_GL_PROCESS(PFNGLVERTEXATTRIBDIVISORPROC,glVertexAttribDivisor) \
    BTK_GL_PROCESS(PFNGLBINDTRANSFORMFEEDBACKPROC,glBindTransformFeedback) \
    BTK_GL_PROCESS(PFNGLDELETETRANSFORMFEEDBACKSPROC,glDeleteTransformFeedbacks) \
    BTK_GL_PROCESS(PFNGLGENTRANSFORMFEEDBACKSPROC,glGenTransformFeedbacks) \
    BTK_GL_PROCESS(PFNGLISTRANSFORMFEEDBACKPROC,glIsTransformFeedback) \
    BTK_GL_PROCESS(PFNGLPAUSETRANSFORMFEEDBACKPROC,glPauseTransformFeedback) \
    BTK_GL_PROCESS(PFNGLRESUMETRANSFORMFEEDBACKPROC,glResumeTransformFeedback) \
    BTK_GL_PROCESS(PFNGLGETPROGRAMBINARYPROC,glGetProgramBinary) \
    BTK_GL_PROCESS(PFNGLPROGRAMBINARYPROC,glProgramBinary) \
    BTK_GL_PROCESS(PFNGLPROGRAMPARAMETERIPROC,glProgramParameteri) \
    BTK_GL_PROCESS(PFNGLINVALIDATEFRAMEBUFFERPROC,glInvalidateFramebuffer) \
    BTK_GL_PROCESS(PFNGLINVALIDATESUBFRAMEBUFFERPROC,glInvalidateSubFramebuffer) \
    BTK_GL_PROCESS(PFNGLTEXSTORAGE2DPROC,glTexStorage2D) \
    BTK_GL_PROCESS(PFNGLTEXSTORAGE3DPROC,glTexStorage3D) \
    
#define BTK_OPENGLES2_FUNCTIONS \
    BTK_OPENGLES2_FUNCTIONS_PART

#define BTK_OPENGLES3_FUNCTIONS \
    BTK_OPENGLES2_FUNCTIONS_PART \
    BTK_OPENGLES3_FUNCTIONS_PART\

#endif // _BTK_OPENGL_MACRO
