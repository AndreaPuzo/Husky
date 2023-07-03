#ifndef __HUSKY_H
# define __HUSKY_H

# define __HUSKY_VERSION_MAJOR 0
# define __HUSKY_VERSION_MINOR 0
# define __HUSKY_VERSION_PATCH 0
# define __HUSKY_VERSION            \
  (                                 \
    (__HUSKY_VERSION_MAJOR << 24) | \
    (__HUSKY_VERSION_MINOR << 16) | \
    (__HUSKY_VERSION_PATCH <<  0)   \
  )

# define __HUSKY__ __HUSKY_VERSION

# include <stdint.h>

typedef uint8_t  u8_t  ;
typedef uint16_t u16_t ;
typedef uint32_t u32_t ;
typedef uint64_t u64_t ;
typedef int8_t   i8_t  ;
typedef int16_t  i16_t ;
typedef int32_t  i32_t ;
typedef int64_t  i64_t ;
typedef void *   ptr_t ;

# define HUSKY_FILE_MAG_NUM_0 0x45
# define HUSKY_FILE_MAG_NUM_1 0x70
# define HUSKY_FILE_MAG_NUM_2 0xFA
# define HUSKY_FILE_MAG_NUM_3 0xDE
# define HUSKY_FILE_VERSION_0 0x00
# define HUSKY_FILE_VERSION_1 0x00
# define HUSKY_FILE_VERSION_2 0x00
# define HUSKY_FILE_VERSION_3 0x01

# define HUSKY_MEMORY_SIZE_DEFAULT (8 << 20)

enum {
  HUSKY_SUCCESS                ,
  HUSKY_FAILURE                ,
  HUSKY_ERROR_DIVISION_BY_ZERO ,
  HUSKY_ERROR_OUT_OF_MEMORY    ,
  HUSKY_ERROR_STACK_OVERFLOW   ,
  HUSKY_ERROR_STACK_UNDERFLOW  ,
  HUSKY_ERROR_UNDEFINED_INST   ,
  HUSKY_ERROR_UNDEFINED_ERROR  ,
  HUSKY_ERROR_UNDEFINED_STATE  ,
  HUSKY_ERROR_INVALID_FRAME    ,
  HUSKY_ERROR_INVALID_MODULE   ,
  HUSKY_ERROR_INVALID_NATIVE   ,
  HUSKY_ERROR_INVALID_ADDRESS  ,
  HUSKY_ERROR_INVALID_STRING   ,

  HUSKY_N_ERRORS
} ;

enum {
  HUSKY_STATE_HALTED  ,
  HUSKY_STATE_BREAKED ,
  HUSKY_STATE_READY   ,

  HUSKY_N_STATES
} ;

enum {
  HUSKY_INST_HALT                ,
  HUSKY_INST_NOOP                ,
  HUSKY_INST_BREAKPOINT          ,
  HUSKY_INST_ERROR_SET           ,
  HUSKY_INST_ERROR_GET           ,
  HUSKY_INST_JUMP                ,
  HUSKY_INST_JUMP_INDIRECT       ,
  HUSKY_INST_JUMP_IF_FALSE       ,
  HUSKY_INST_JUMP_IF_TRUE        ,
  HUSKY_INST_CALL                ,
  HUSKY_INST_CALL_INDIRECT       ,
  HUSKY_INST_RETURN              ,
  HUSKY_INST_MODULE_OPEN         ,
  HUSKY_INST_MODULE_CLOSE        ,
  HUSKY_INST_NATIVE_LOAD         ,
  HUSKY_INST_NATIVE_CALL         ,
  HUSKY_INST_IS_NULL_POINTER     ,
  HUSKY_INST_IS_NOT_NULL_POINTER ,
  HUSKY_INST_IS_STRING           ,
  HUSKY_INST_ENTER               ,
  HUSKY_INST_LEAVE               ,
  HUSKY_INST_PUSH_8              ,
  HUSKY_INST_PUSH_16             ,
  HUSKY_INST_PUSH_32             ,
  HUSKY_INST_PUSH_64             ,
  HUSKY_INST_POP                 ,
  HUSKY_INST_EXCHANGE            ,
  HUSKY_INST_SET_AT_SP           ,
  HUSKY_INST_GET_AT_SP           ,
  HUSKY_INST_SET_AT_FP           ,
  HUSKY_INST_GET_AT_FP           ,
  HUSKY_INST_STORE_8             ,
  HUSKY_INST_STORE_16            ,
  HUSKY_INST_STORE_32            ,
  HUSKY_INST_STORE_64            ,
  HUSKY_INST_LOAD_8              ,
  HUSKY_INST_LOAD_16             ,
  HUSKY_INST_LOAD_32             ,
  HUSKY_INST_LOAD_64             ,
  HUSKY_INST_NEGATE              ,
  HUSKY_INST_ADD                 ,
  HUSKY_INST_SUBTRACT            ,
  HUSKY_INST_MULTIPLY            ,
  HUSKY_INST_DIVIDE              ,
  HUSKY_INST_MODULO              ,
  HUSKY_INST_INT_MULTIPLY        ,
  HUSKY_INST_INT_DIVIDE          ,
  HUSKY_INST_INT_MODULO          ,
  HUSKY_INST_IS_EQUAL            ,
  HUSKY_INST_IS_NOT_EQUAL        ,
  HUSKY_INST_IS_LESS             ,
  HUSKY_INST_IS_LESS_OR_EQUAL    ,
  HUSKY_INST_IS_GREATER          ,
  HUSKY_INST_IS_GREATER_OR_EQUAL ,
  HUSKY_INST_BIT_NOT             ,
  HUSKY_INST_BIT_AND             ,
  HUSKY_INST_BIT_OR              ,
  HUSKY_INST_BIT_XOR             ,
  HUSKY_INST_BIT_SHIFT_LEFT      ,
  HUSKY_INST_BIT_SHIFT_RIGHT     ,
  HUSKY_INST_BIT_INT_SHIFT_RIGHT ,
  HUSKY_INST_PRINT               ,

  HUSKY_N_INSTS
} ;

typedef union  husky_object_u husky_object_t ;
typedef struct husky_s        husky_t        ;
typedef u32_t ( * husky_native_t ) (husky_t *) ;

union husky_object_u {
  u64_t u ;
  i64_t i ;
  ptr_t p ;
} ;

struct husky_s {
  u32_t  err_code ;
  u32_t  state    ;
  u64_t  ip       ;
  u64_t  fp       ;
  u64_t  sp       ;
  u64_t  mem_size ;
  u8_t * mem_data ;
  ptr_t  ptr      ;
  u32_t  verbose  ;

  u32_t ( * err_func ) (husky_t *) ;
} ;

const char * husky_error_as_string (u32_t err_code) ;
u32_t husky_error_set (husky_t * husky, u32_t err_code) ;
u32_t husky_error_get (husky_t * husky) ;
u32_t husky_state_set (husky_t * husky, u32_t state) ;
u32_t husky_state_get (husky_t * husky) ;
u32_t husky_memory_write (husky_t * husky, u64_t addr, u64_t size, const ptr_t data) ;
u32_t husky_memory_read (husky_t * husky, u64_t addr, u64_t size, ptr_t data) ;
husky_object_t * husky_stack_peek (husky_t * husky, i64_t rel_addr) ;
u32_t husky_stack_push (husky_t * husky, husky_object_t object) ;
u32_t husky_stack_pop (husky_t * husky, husky_object_t * object) ;
u32_t husky_frame_enter (husky_t * husky, i64_t size) ;
u32_t husky_frame_leave (husky_t * husky) ;
u32_t husky_string_verify(husky_t * husky, u64_t addr) ;
u32_t husky_clock (husky_t * husky) ;
u32_t husky_image_load (husky_t * husky, char * filename) ;

#endif