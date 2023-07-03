#include "husky.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#ifdef _WIN32
# include <win32/dlfcn.h>
#else
# include <dlfcn.h>
#endif

const char * husky_error_as_string (u32_t err_code)
{
  static const char * error_as_string [] = {
    "Success"               ,
    "Failure"               ,
    "Division by zero"      ,
    "Out of memory"         ,
    "Stack overflow"        ,
    "Stack underflow"       ,
    "Undefined instruction" ,
    "Undefined error"       ,
    "Undefined state"       ,
    "Invalid frame"         ,
    "Invalid module"        ,
    "Invalid native"        ,
    "Invalid address"       ,
    "Invalid string"
  } ;

  if (HUSKY_N_ERRORS <= err_code)
    return NULL ;

  return error_as_string[err_code] ;
}

u32_t husky_error_set (husky_t * husky, u32_t err_code)
{
  if (HUSKY_N_ERRORS <= err_code)
    return husky_error_set(husky, HUSKY_ERROR_UNDEFINED_ERROR) ;

  husky->err_code = err_code ;

  return husky_error_get(husky) ;
}

u32_t husky_error_get (husky_t * husky)
{
  if (HUSKY_SUCCESS == husky->err_code)
    return HUSKY_SUCCESS ;

  if (NULL != husky->err_func)
    return husky->err_func(husky) ;

  return HUSKY_FAILURE ;
}

u32_t husky_state_set (husky_t * husky, u32_t state)
{
  if (HUSKY_N_STATES <= state)
    return husky_error_get(husky) ;

  husky->state = state ;

  return husky_error_get(husky) ;
}

u32_t husky_state_get (husky_t * husky)
{
  return husky->state ;
}

u32_t husky_memory_write (husky_t * husky, u64_t addr, u64_t size, const ptr_t data)
{
  if (husky->mem_size < addr + size)
    return husky_error_set(husky, HUSKY_ERROR_OUT_OF_MEMORY) ;

  memcpy(husky->mem_data + addr, data, size) ;

  return husky_error_get(husky) ;
}

u32_t husky_memory_read (husky_t * husky, u64_t addr, u64_t size, ptr_t data)
{
  if (husky->mem_size < addr + size)
    return husky_error_set(husky, HUSKY_ERROR_OUT_OF_MEMORY) ;

  memcpy(data, husky->mem_data + addr, size) ;

  return husky_error_get(husky) ;
}

u32_t husky_memory_read_ip (husky_t * husky, u64_t size, ptr_t data)
{
  if (HUSKY_SUCCESS != husky_memory_read(husky, husky->ip, size, data))
    return husky_error_get(husky) ;

  husky->ip += size ;

  return husky_error_get(husky) ;
}

husky_object_t * husky_stack_peek (husky_t * husky, i64_t rel_addr)
{
  rel_addr *= sizeof(husky_object_t) ;

  if (rel_addr < 0) {
    rel_addr = -rel_addr ;

    if (husky->sp < rel_addr) {
      husky_error_set(husky, HUSKY_ERROR_STACK_UNDERFLOW) ;
      return NULL ;
    }

    return (husky_object_t *)(husky->mem_data + husky->sp - rel_addr) ;
  }

  if (husky->mem_size < husky->sp + rel_addr) {
    husky_error_set(husky, HUSKY_ERROR_STACK_OVERFLOW) ;
    return NULL ;
  }

  return (husky_object_t *)(husky->mem_data + husky->sp + rel_addr) ;
}

u32_t husky_stack_push (husky_t * husky, husky_object_t object)
{
  husky_object_t * stack_object = husky_stack_peek(husky, 0) ;

  if (NULL != stack_object) {
    *stack_object = object ;

    husky->sp += sizeof(husky_object_t) ;
  }

  return husky_error_get(husky) ;
}

u32_t husky_stack_pop (husky_t * husky, husky_object_t * object)
{
  husky_object_t * stack_object = husky_stack_peek(husky, -1) ;

  if (NULL != stack_object) {
    if (NULL != object) {
      *object = *stack_object ;
    }

    husky->sp -= sizeof(husky_object_t) ;
  }

  return husky_error_get(husky) ;
}

u32_t husky_frame_enter (husky_t * husky, i64_t size)
{
  if (size < 0)
    return husky_error_set(husky, HUSKY_ERROR_INVALID_FRAME) ;

  husky_object_t object ;

  object.u = husky->fp ;

  if (HUSKY_SUCCESS != husky_stack_push(husky, object))
    return husky_error_get(husky) ;

  husky->fp = husky->sp ;

  if (NULL == husky_stack_peek(husky, size))
    return husky_error_get(husky) ;

  husky->sp += size * sizeof(husky_object_t) ;

  return husky_error_get(husky) ;
}

u32_t husky_frame_leave (husky_t * husky)
{
  husky_object_t object ;

  husky->sp = husky->fp ;

  if (HUSKY_SUCCESS != husky_stack_pop(husky, &object))
    return husky_error_get(husky) ;

  husky->fp = object.u ;

  return husky_error_get(husky) ;
}

u32_t husky_string_verify(husky_t * husky, u64_t addr)
{
  if (husky->mem_size <= addr)
    return husky_error_set(husky, HUSKY_ERROR_INVALID_ADDRESS) ;

  if (NULL == memchr(husky->mem_data + addr, 0, husky->mem_size - addr))
    return husky_error_set(husky, HUSKY_ERROR_INVALID_STRING) ;

  return husky_error_get(husky) ;
}

#define _UNO(__0)
#define _NEG(__0)      (-(__0))
#define _NOT(__0)      (~(__0))
#define _BNO(__0, __1)
#define _ADD(__0, __1) ((__0) + (__1))
#define _SUB(__0, __1) ((__0) - (__1))
#define _MUL(__0, __1) ((__0) * (__1))
#define _DIV(__0, __1) ((__0) / (__1))
#define _MOD(__0, __1) ((__0) % (__1))
#define _AND(__0, __1) ((__0) & (__1))
#define _OR(__0, __1)  ((__0) | (__1))
#define _XOR(__0, __1) ((__0) ^ (__1))
#define _SHL(__0, __1) ((__0) << (__1))
#define _SHR(__0, __1) ((__0) >> (__1))
#define _IEQ(__0, __1) ((__0) == (__1))
#define _INE(__0, __1) ((__0) != (__1))
#define _ILS(__0, __1) ((__0) <  (__1))
#define _ILE(__0, __1) ((__0) <= (__1))
#define _IGT(__0, __1) ((__0) >  (__1))
#define _IGE(__0, __1) ((__0) >= (__1))

#define _IDZ(__0, __1)                                       \
  {                                                          \
    if (0 == (__1)) {                                        \
      husky_error_set(husky, HUSKY_ERROR_DIVISION_BY_ZERO) ; \
      break ;                                                \
    }                                                        \
  }

u32_t husky_clock (husky_t * husky)
{
  u8_t opr_code ;

  if (HUSKY_SUCCESS != husky_memory_read_ip(husky, sizeof(opr_code), &opr_code))
    return husky_error_get(husky) ;

  if (0 != husky->verbose) {
    fprintf(stderr, "%012" PRIX64 " | %02" PRIX8 "\n", husky->ip - sizeof(u8_t), opr_code) ;
  }

  husky_object_t object_0, object_1, object_2 ;

#define _UNAOP(__opr_code, __type_0, __type_2, __check, __func)           \
  case __opr_code : {                                                     \
    if (HUSKY_SUCCESS != husky_stack_pop(husky, &object_0))               \
      break ;                                                             \
                                                                          \
    __check(object_0.__type_0) ;                                          \
    object_2.__type_2 = __func(object_0.__type_0) ;                       \
                                                                          \
    husky_stack_push(husky, object_2) ;                                   \
  } break ;

#define _BINOP(__opr_code, __type_0, __type_1, __type_2, __check, __func) \
  case __opr_code : {                                                     \
    if (HUSKY_SUCCESS != husky_stack_pop(husky, &object_0))               \
      break ;                                                             \
                                                                          \
    if (HUSKY_SUCCESS != husky_stack_pop(husky, &object_1))               \
      break ;                                                             \
                                                                          \
    __check(object_0.__type_0, object_1.__type_1) ;                       \
    object_2.__type_2 = __func(object_0.__type_0, object_1.__type_1) ;    \
                                                                          \
    husky_stack_push(husky, object_2) ;                                   \
  } break ;

  switch (opr_code) {
  case HUSKY_INST_HALT : {
    husky_state_set(husky, HUSKY_STATE_HALTED) ;
  } break ;

  case HUSKY_INST_NOOP : {
    /* nothing */
  } break ;

  case HUSKY_INST_BREAKPOINT : {
    husky_state_set(husky, HUSKY_STATE_BREAKED) ;
  } break ;

  case HUSKY_INST_ERROR_SET : {
    if (HUSKY_SUCCESS != husky_stack_pop(husky, &object_0))
      break ;

    husky_error_set(husky, object_0.u) ;
  } break ;

  case HUSKY_INST_ERROR_GET : {
    object_0.u = husky->err_code ;

    husky_stack_push(husky, object_0) ;
  } break ;

  case HUSKY_INST_JUMP : {
    i32_t opr_data = 0 ;

    if (HUSKY_SUCCESS != husky_memory_read_ip(husky, sizeof(opr_data), &opr_data))
      break ;

    husky->ip += (i64_t)opr_data ;
  } break ;

  case HUSKY_INST_JUMP_INDIRECT : {
    if (HUSKY_SUCCESS != husky_stack_pop(husky, &object_0))
      break ;

    husky->ip += object_0.i ;
  } break ;

  case HUSKY_INST_JUMP_IF_FALSE : {
    i32_t opr_data = 0 ;

    if (HUSKY_SUCCESS != husky_memory_read_ip(husky, sizeof(opr_data), &opr_data))
      break ;

    if (HUSKY_SUCCESS != husky_stack_pop(husky, &object_0))
      break ;

    if (0 == object_0.u) {
      husky->ip += (i64_t)opr_data ;
    }
  } break ;

  case HUSKY_INST_JUMP_IF_TRUE : {
    i32_t opr_data = 0 ;

    if (HUSKY_SUCCESS != husky_memory_read_ip(husky, sizeof(opr_data), &opr_data))
      break ;

    if (HUSKY_SUCCESS != husky_stack_pop(husky, &object_0))
      break ;

    if (0 != object_0.u) {
      husky->ip += (i64_t)opr_data ;
    }
  } break ;

  case HUSKY_INST_CALL : {
    i32_t opr_data = 0 ;

    if (HUSKY_SUCCESS != husky_memory_read_ip(husky, sizeof(opr_data), &opr_data))
      break ;

    object_0.u = husky->ip ;

    if (HUSKY_SUCCESS != husky_stack_push(husky, object_0))
      break ;

    husky->ip += (i64_t)opr_data ;
  } break ;

  case HUSKY_INST_CALL_INDIRECT : {
    if (HUSKY_SUCCESS != husky_stack_pop(husky, &object_1))
      break ;

    object_0.u = husky->ip ;

    if (HUSKY_SUCCESS != husky_stack_push(husky, object_0))
      break ;

    husky->ip += object_1.i ;
  } break ;

  case HUSKY_INST_RETURN : {
    if (HUSKY_SUCCESS != husky_stack_pop(husky, &object_0))
      break ;

    husky->ip = object_0.u ;
  } break ;

  case HUSKY_INST_MODULE_OPEN : {
    if (HUSKY_SUCCESS != husky_stack_pop(husky, &object_0))
      break ;

    if (HUSKY_SUCCESS != husky_stack_pop(husky, &object_1))
      break ;

    if (HUSKY_SUCCESS != husky_string_verify(husky, object_0.u))
      break ;

    object_2.p = dlopen(husky->mem_data + object_0.u, object_1.i) ;

    husky_stack_push(husky, object_2) ;
  } break ;

  case HUSKY_INST_MODULE_CLOSE : {
    if (HUSKY_SUCCESS != husky_stack_pop(husky, &object_0))
      break ;

    if (NULL == object_0.p) {
      husky_error_set(husky, HUSKY_ERROR_INVALID_MODULE) ;
      break ;
    }

    dlclose(object_0.p) ;
  } break ;

  case HUSKY_INST_NATIVE_LOAD : {
    if (HUSKY_SUCCESS != husky_stack_pop(husky, &object_0))
      break ;

    if (HUSKY_SUCCESS != husky_stack_pop(husky, &object_1))
      break ;

    if (HUSKY_SUCCESS != husky_string_verify(husky, object_1.u))
      break ;

    if (NULL == object_0.p) {
      husky_error_set(husky, HUSKY_ERROR_INVALID_MODULE) ;
      break ;
    }

    object_2.p = dlsym(object_0.p, husky->mem_data + object_1.u) ;

    husky_stack_push(husky, object_2) ;
  } break ;

  case HUSKY_INST_NATIVE_CALL : {
    if (HUSKY_SUCCESS != husky_stack_pop(husky, &object_0))
      break ;

    if (NULL == object_0.p) {
      husky_error_set(husky, HUSKY_ERROR_INVALID_NATIVE) ;
      break ;
    }

    husky_native_t native = (husky_native_t)object_0.p ;

    native(husky) ;
  } break ;

  case HUSKY_INST_IS_NULL_POINTER : {
    if (HUSKY_SUCCESS != husky_stack_pop(husky, &object_0))
      break ;

    object_1.u = NULL == object_0.p ;

    husky_stack_push(husky, object_1) ;
  } break ;

  case HUSKY_INST_IS_NOT_NULL_POINTER : {
    if (HUSKY_SUCCESS != husky_stack_pop(husky, &object_0))
      break ;

    object_1.u = NULL != object_0.p ;

    husky_stack_push(husky, object_1) ;
  } break ;

  case HUSKY_INST_IS_STRING : {
    if (HUSKY_SUCCESS != husky_stack_pop(husky, &object_0))
      break ;

    if (HUSKY_SUCCESS != husky_string_verify(husky, object_0.u)) {
      object_1.u = 0 ;
      husky_error_set(husky, HUSKY_SUCCESS) ;
    } else {
      object_1.u = 1 ;
    }

    husky_stack_push(husky, object_1) ;
  } break ;

  case HUSKY_INST_ENTER : {
    u16_t opr_data = 0 ;

    if (HUSKY_SUCCESS != husky_memory_read_ip(husky, sizeof(opr_data), &opr_data))
      break ;

    husky_frame_enter(husky, (i64_t)opr_data) ;
  } break ;

  case HUSKY_INST_LEAVE : {
    husky_frame_leave(husky) ;
  } break ;

  case HUSKY_INST_PUSH_8  : 
  case HUSKY_INST_PUSH_16 : 
  case HUSKY_INST_PUSH_32 : 
  case HUSKY_INST_PUSH_64 : {
    u64_t opr_data = 0 ;

    if (HUSKY_SUCCESS != husky_memory_read_ip(husky, 1 << (opr_code - HUSKY_INST_PUSH_8), &opr_data))
      break ;

    object_0.u = opr_data ;

    husky_stack_push(husky, object_0) ;
  } break ;

  case HUSKY_INST_POP : {
    husky_stack_pop(husky, NULL) ;
  } break ;

  case HUSKY_INST_EXCHANGE : {
    i16_t opr_data = 0 ;

    if (HUSKY_SUCCESS != husky_memory_read_ip(husky, sizeof(opr_data), &opr_data))
      break ;

    if (HUSKY_SUCCESS != husky_stack_pop(husky, &object_0))
      break ;

    husky_object_t * object = husky_stack_peek(husky, (i64_t)opr_data) ;

    if (NULL == object)
      break ;

    if (HUSKY_SUCCESS != husky_stack_push(husky, *object))
      break ;

    *object = object_0 ;
  } break ;

  case HUSKY_INST_SET_AT_SP : {
    i16_t opr_data = 0 ;

    if (HUSKY_SUCCESS != husky_memory_read_ip(husky, sizeof(opr_data), &opr_data))
      break ;

    if (HUSKY_SUCCESS != husky_stack_pop(husky, &object_0))
      break ;

    husky_object_t * object = husky_stack_peek(husky, (i64_t)opr_data) ;

    if (NULL == object)
      break ;

    *object = object_0 ;
  } break ;

  case HUSKY_INST_GET_AT_SP : {
    i16_t opr_data = 0 ;

    if (HUSKY_SUCCESS != husky_memory_read_ip(husky, sizeof(opr_data), &opr_data))
      break ;

    husky_object_t * object = husky_stack_peek(husky, (i64_t)opr_data) ;

    if (NULL == object)
      break ;

    husky_stack_push(husky, *object) ;
  } break ;

  case HUSKY_INST_SET_AT_FP : {
    i16_t opr_data = 0 ;

    if (HUSKY_SUCCESS != husky_memory_read_ip(husky, sizeof(opr_data), &opr_data))
      break ;

    if (HUSKY_SUCCESS != husky_stack_pop(husky, &object_0))
      break ;

    u64_t last_sp = husky->sp ;
    husky->sp = husky->fp ;
    husky_object_t * object = husky_stack_peek(husky, (i64_t)opr_data) ;
    husky->sp = last_sp ;

    if (NULL == object)
      break ;

    *object = object_0 ;
  } break ;

  case HUSKY_INST_GET_AT_FP : {
    i16_t opr_data = 0 ;

    if (HUSKY_SUCCESS != husky_memory_read_ip(husky, sizeof(opr_data), &opr_data))
      break ;

    u64_t last_sp = husky->sp ;
    husky->sp = husky->fp ;
    husky_object_t * object = husky_stack_peek(husky, (i64_t)opr_data) ;
    husky->sp = last_sp ;

    if (NULL == object)
      break ;

    husky_stack_push(husky, *object) ;
  } break ;


  case HUSKY_INST_STORE_8  : 
  case HUSKY_INST_STORE_16 : 
  case HUSKY_INST_STORE_32 : 
  case HUSKY_INST_STORE_64 : {
    if (HUSKY_SUCCESS != husky_stack_pop(husky, &object_0))
      break ;

    if (HUSKY_SUCCESS != husky_stack_pop(husky, &object_1))
      break ;

    husky_memory_write(husky, object_0.u, 1 << (opr_code - HUSKY_INST_STORE_8), &object_1.u) ;
  } break ;

  case HUSKY_INST_LOAD_8  : 
  case HUSKY_INST_LOAD_16 : 
  case HUSKY_INST_LOAD_32 : 
  case HUSKY_INST_LOAD_64 : {
    if (HUSKY_SUCCESS != husky_stack_pop(husky, &object_0))
      break ;

    object_1.u = 0 ;

    if (HUSKY_SUCCESS != husky_memory_read(husky, object_0.u, 1 << (opr_code - HUSKY_INST_STORE_8), &object_1.u))
      break ;

    husky_stack_push(husky, object_1) ;
  } break ;

  _UNAOP( HUSKY_INST_NEGATE              , u ,     u , _UNO , _NEG )
  _BINOP( HUSKY_INST_ADD                 , u , u , u , _BNO , _ADD )
  _BINOP( HUSKY_INST_SUBTRACT            , u , u , u , _BNO , _SUB )
  _BINOP( HUSKY_INST_MULTIPLY            , u , u , u , _BNO , _MUL )
  _BINOP( HUSKY_INST_DIVIDE              , u , u , u , _IDZ , _DIV )
  _BINOP( HUSKY_INST_MODULO              , u , u , u , _IDZ , _MOD )
  _BINOP( HUSKY_INST_INT_MULTIPLY        , i , i , i , _BNO , _MUL )
  _BINOP( HUSKY_INST_INT_DIVIDE          , i , i , i , _IDZ , _DIV )
  _BINOP( HUSKY_INST_INT_MODULO          , i , i , i , _IDZ , _MOD )
  _BINOP( HUSKY_INST_IS_EQUAL            , u , u , u , _BNO , _IEQ )
  _BINOP( HUSKY_INST_IS_NOT_EQUAL        , u , u , u , _BNO , _INE )
  _BINOP( HUSKY_INST_IS_LESS             , u , u , u , _BNO , _ILS )
  _BINOP( HUSKY_INST_IS_LESS_OR_EQUAL    , u , u , u , _BNO , _ILE )
  _BINOP( HUSKY_INST_IS_GREATER          , u , u , u , _BNO , _IGT )
  _BINOP( HUSKY_INST_IS_GREATER_OR_EQUAL , u , u , u , _BNO , _IGE )
  _UNAOP( HUSKY_INST_BIT_NOT             , u ,     u , _UNO , _NOT )
  _BINOP( HUSKY_INST_BIT_AND             , u , u , u , _BNO , _AND )
  _BINOP( HUSKY_INST_BIT_OR              , u , u , u , _BNO , _OR  )
  _BINOP( HUSKY_INST_BIT_XOR             , u , u , u , _BNO , _XOR )
  _BINOP( HUSKY_INST_BIT_SHIFT_LEFT      , u , u , u , _IDZ , _SHL )
  _BINOP( HUSKY_INST_BIT_SHIFT_RIGHT     , u , u , u , _IDZ , _SHR )
  _BINOP( HUSKY_INST_BIT_INT_SHIFT_RIGHT , i , u , i , _BNO , _SHR )

  case HUSKY_INST_PRINT : {
    if (HUSKY_SUCCESS != husky_stack_pop(husky, &object_0))
      break ;

    if (HUSKY_SUCCESS != husky_stack_pop(husky, &object_1))
      break ;

    switch (object_0.u) {
    case 0x00 : {
      fprintf(stdout, "%" PRIu64, object_1.u) ;
    } break ;

    case 0x01 : {
      fprintf(stdout, "%" PRIi64, object_1.i) ;
    } break ;

    case 0x02 : {
      fprintf(stdout, "%" PRIx64, object_1.u) ;
    } break ;

    case 0x03 : {
      fprintf(stdout, "%" PRIX64, object_1.u) ;
    } break ;

    case 0x04 : {
      fprintf(stdout, "%c", object_1.u) ;
    } break ;

    case 0x05 : {
      if (HUSKY_SUCCESS != husky_string_verify(husky, object_1.u))
        break ;

      fprintf(stdout, "%s", husky->mem_data + object_1.u) ;
    } break ;

    default :
      break ;
    }
  } break ;

  default :
    husky_error_set(husky, HUSKY_ERROR_UNDEFINED_INST) ;
    break ;
  }

  return husky_error_get(husky) ;
}

u32_t husky_image_load (husky_t * husky, char * filename)
{
  FILE * fileptr ;

#ifdef _WIN32
  fileptr = fopen(filename, "r") ;
#else
  fileptr = fopen(filename, "rb") ;
#endif

  if (NULL == fileptr) {
    fprintf(stderr, "Error: Cannot open `%s`.\n", filename) ;
    fclose(fileptr) ;
    return HUSKY_FAILURE ;
  }

  if (
    HUSKY_FILE_MAG_NUM_0 != fgetc(fileptr) ||
    HUSKY_FILE_MAG_NUM_1 != fgetc(fileptr) ||
    HUSKY_FILE_MAG_NUM_2 != fgetc(fileptr) ||
    HUSKY_FILE_MAG_NUM_3 != fgetc(fileptr)
  ) {
    fprintf(stderr, "Error: Invalid magic number.\n") ;
    fclose(fileptr) ;
    return HUSKY_FAILURE ;
  }

  if (
    HUSKY_FILE_VERSION_0 != fgetc(fileptr) ||
    HUSKY_FILE_VERSION_1 != fgetc(fileptr) ||
    HUSKY_FILE_VERSION_2 != fgetc(fileptr) ||
    HUSKY_FILE_VERSION_3 != fgetc(fileptr)
  ) {
    fprintf(stderr, "Error: Ivalid version number.\n") ;
    fclose(fileptr) ;
    return HUSKY_FAILURE ;
  }

  u64_t addr, size ;

  if (1 != fread(&size, sizeof(size), 1,  fileptr)) {
    fprintf(stderr, "Error: Cannot read the instruction pointer.\n") ;
    fclose(fileptr) ;
    return HUSKY_FAILURE ;
  }

  if (husky->mem_size < size) {
    fprintf(stderr, "Error: The memory is not enough to run the program.\n") ;
    fclose(fileptr) ;
    return HUSKY_FAILURE ;
  }

  if (1 != fread(&addr, sizeof(addr), 1,  fileptr)) {
    fprintf(stderr, "Error: Cannot read the instruction pointer.\n") ;
    fclose(fileptr) ;
    return HUSKY_FAILURE ;
  }

  if (husky->mem_size <= addr) {
    fprintf(stderr, "Error: The instruction pointer is out of memory.\n") ;
    fclose(fileptr) ;
    return HUSKY_FAILURE ;
  }

  husky->ip = addr ;

  if (1 != fread(&addr, sizeof(addr), 1,  fileptr)) {
    fprintf(stderr, "Error: Cannot read the stack pointer.\n") ;
    fclose(fileptr) ;
    return HUSKY_FAILURE ;
  }

  if (husky->mem_size <= addr) {
    fprintf(stderr, "Error: The stack pointer is out of memory.\n") ;
    fclose(fileptr) ;
    return HUSKY_FAILURE ;
  }

  husky->fp = husky->sp = addr ;

  u16_t secs, i ;

  if (1 != fread(&secs, sizeof(secs), 1,  fileptr)) {
    fprintf(stderr, "Error: Cannot read the number of sections.\n") ;
    fclose(fileptr) ;
    return HUSKY_FAILURE ;
  }

  if (0 != husky->verbose) {
    fprintf(stderr, "Image `%s`:\n", filename) ;
    fprintf(stderr, "--- `ip` at 0x%012" PRIX64 "\n", husky->ip) ;
    fprintf(stderr, "--- `fp` at 0x%012" PRIX64 "\n", husky->fp) ;
    fprintf(stderr, "--- `sp` at 0x%012" PRIX64 "\n", husky->sp) ;
    fprintf(stderr, "--- %u sections\n", secs) ;
  }

  for (i = 0 ; i < secs ; ++i) {
    char name [32 + 1] ;
    int j = 0 ;

    do {
      int eof = fgetc(fileptr) ;

      if (EOF == eof) {
        fprintf(stderr, "Error: Section %u: Is out of binary.\n", i) ;
        fclose(fileptr) ;
        return HUSKY_FAILURE ;
      }

      name[j] = eof ;
    } while (j < 32 && 0 != name[j++]) ;

    if (0 != husky->verbose) {
      fprintf(stderr, "--- Reading section `%s`...\n", name) ;
    }

    name[j] = 0 ;

    if (1 != fread(&addr, sizeof(addr), 1,  fileptr)) {
      fprintf(stderr, "Error: Section `%s` (%u): Cannot read the address.\n", name, i) ;
      fclose(fileptr) ;
      return HUSKY_FAILURE ;
    }

    if (1 != fread(&size, sizeof(size), 1,  fileptr)) {
      fprintf(stderr, "Error: Section `%s` (%u): Cannot read the size.\n", name, i) ;
      fclose(fileptr) ;
      return HUSKY_FAILURE ;
    }

    if (husky->mem_size < addr + size) {
      fprintf(stderr, "Error: Section `%s` (%u): Is out of memory.\n", name, i) ;
      fclose(fileptr) ;
      return HUSKY_FAILURE ;
    }

    if (size != fread(husky->mem_data + addr, sizeof(u8_t), size,  fileptr)) {
      fprintf(stderr, "Error: Section `%s` (%u): Cannot read the data.\n", name, i) ;
      fclose(fileptr) ;
      return HUSKY_FAILURE ;
    }
  }

  fclose(fileptr) ;

  husky_state_set(husky, HUSKY_STATE_READY) ;
  husky_error_set(husky, HUSKY_SUCCESS) ;

  return HUSKY_SUCCESS ;
}
