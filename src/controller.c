/***************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************
 * Introduction
 **************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************/

/**********************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************//**
 * @file      controller.c
 * 
 * @version   1.0
 *
 * @date      09-10-2025
 *
 * @brief     Controller primitives and dynamic library function calls
 *  
 * @author    Fábio D. Pacheco, 
 * @email     fabio.d.pacheco@inesctec.pt or pacheco.castro.fabio@gmail.com
 *
 * @copyright Copyright (c) [2025] [Fábio D. Pacheco]
 * 
 * @note      Manuals:
 * 
 **************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************/

/***************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************
 * Imported libraries
 **************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************/

#include <string.h>
#include <errno.h>

#include "controller.h"

/***************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************
 * Local Prototypes
 **************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************/

int8_t controller_valid( 
  const mixlink_controller_t * controller           
);

int8_t try_init_ser(
  struct serial_handler * ser, 
  mixlink_param_dev_t param
);

struct serial_handler * mixlink_controller_driver_pipeline_handler(
  const enum direction dir, 
  mixlink_controller_t * controller
);

/***************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************
 * Functions description
 **************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************/

/**************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************/
int8_t 
controller_valid( 
  const mixlink_controller_t * controller           
){
  if( !controller ){
    errno = EINVAL;
    return -1;
  }
  return 0;
}

/**************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************/
int8_t 
try_init_ser(
  struct serial_handler * ser, 
  mixlink_param_dev_t param
){
  if( !ser ){
    errno = EINVAL;
    return -1;
  }

  if( !strcmp( param.device, "" ) )
    return -1;

  serial_t tmp;
  int8_t ret = serial_open( 
    &tmp,
    param.device,
    0,     // Read&Write
    NULL,  // Load default configuration
    NULL,  // Load no hotplug feature 
    NULL   // Load in sync mode
  );

  if( -1 != ret ){
    (void) memcpy( &ser->sr, &tmp, sizeof(serial_t) );
    ser->enabled = true;

    (void) mixlink_mod_load( 
      param.driver, 
      MIXLINK_STACK_SECTION_CONTROLLER_DRIVER,
      &ser->driver
    );
    return 0;
  }

  return -1;
}


/**************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************/
int8_t 
mixlink_controller_init( 
  const mixlink_param_controller_t param,
  mixlink_controller_t * controller
){
  if( -1 == controller_valid( controller ) )  
    return -1;

  (void) memset( 
    controller, 
    0, 
    sizeof(mixlink_controller_t) 
  );

  bool failed2ser = true;

  int8_t ret = try_init_ser(
    &controller->def,
    param.dev.def
  );

  if( -1 == ret ){
    if( !try_init_ser( &controller->pair.rx, param.dev.pair.rx ) )
      failed2ser = false;
    if( !try_init_ser( &controller->pair.tx, param.dev.pair.tx ) )
      failed2ser = false;
  }
  else
    failed2ser = false; 
  
  if( failed2ser ){
    errno = EINVAL;
    return -1;
  }  

  (void) mixlink_mod_load( 
    param.qos, 
    MIXLINK_STACK_SECTION_CONTROLLER_QOS,
    &controller->qos
  );

  (void) mixlink_mod_load( 
    param.framer, 
    MIXLINK_STACK_SECTION_CONTROLLER_FRAMER,
    &controller->framer
  );

  (void) mixlink_mod_load( 
    param.segm, 
    MIXLINK_STACK_SECTION_CONTROLLER_SEGM,
    &controller->segm
  );

  return 0;
}

/**************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************/
int8_t 
mixlink_controller_close(
  mixlink_controller_t * controller
){
  if( -1 == controller_valid( controller ) )
    return -1;

  const int8_t n_sers = 3;
  struct serial_handler * iface[ ] = {
    &controller->def,
    &controller->pair.tx,
    &controller->pair.rx
  };

  for( int8_t i = 0; i < n_sers ; ++i ){
    if( iface[i]->enabled ){
      (void) serial_close( &iface[i]->sr );
      (void) mixlink_mod_unload( &iface[i]->driver );
      iface[i]->enabled = false;
    }
  }

  (void) mixlink_mod_unload( &controller->segm );
  (void) mixlink_mod_unload( &controller->qos );
  (void) mixlink_mod_unload( &controller->framer );

  return 0;
}

/**************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************/
size_t 
mixlink_controller_write(
  mixlink_controller_t * controller,
  mixlink_buf8_t * data
){
  if( -1 == controller_valid( controller ) || !data ){
    errno = EINVAL;
    return 0;
  }

  serial_t * ser = NULL;

  if( controller->def.enabled )
    ser = &controller->def.sr;
  else if( controller->pair.tx.enabled )
    ser = &controller->pair.tx.sr;
  else{
    errno = EINVAL;
    return 0;
  }

  size_t len = serial_write( 
    ser,
    data->val,
    data->len
  );

  if( !len && ((errno == ENODEV) || (errno == EIO)) ){
    uint16_t iterations = 1e3;
    if( -1 == serial_reopen( ser, iterations ) )
      errno = ENODEV;
  }

  return len;
}

/**************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************/
size_t 
mixlink_controller_read( 
  mixlink_buf8_t * data,
  const size_t offset,
  const size_t total,
  mixlink_controller_t * controller
){
  if( -1 == controller_valid( controller ) || !data ){
    errno = EINVAL;
    return 0;
  }

  serial_t * ser = NULL;

  if( controller->def.enabled )
    ser = &controller->def.sr;  
  else if( controller->pair.rx.enabled )
    ser = &controller->pair.rx.sr;
  else{
    errno = EINVAL;
    return 0;
  }

  size_t len = serial_read(
    (char *) &data->val[offset],
    data->size,
    0,
    total,
    ser
  );

  if( !len && ((errno == ENODEV) || (errno == EIO)) ){
    uint16_t iterations = 1e3;
    if( -1 == serial_reopen( ser, iterations ) )
      errno = ENODEV;
  }

  return len;
}

/**************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************/
struct serial_handler *
mixlink_controller_driver_pipeline_handler(
  const enum direction dir, 
  mixlink_controller_t * controller
){
  if( controller->def.enabled )
    return &controller->def;

  if( dir == MIXLINK_DIRECTION_FROM_NIC ){
    if( controller->pair.tx.enabled )
      return &controller->pair.tx;
  }

  if( dir == MIXLINK_DIRECTION_TO_NIC ){
    if( controller->pair.rx.enabled )
      return &controller->pair.rx;
  }

  return NULL;
}

/**************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************/
int8_t 
mixlink_controller_driver_io(
  mixlink_abi_gen_io_t data,
  const enum direction dir, 
  mixlink_controller_t * controller
){
  if( -1 == controller_valid( controller ) )
    return -1;

  struct serial_handler * handler = mixlink_controller_driver_pipeline_handler(
    dir,
    controller
  );

  if( !handler )
    return -1;

  mixlink_abi_io_serial_t abi;
  abi.sr = &( handler->sr ); 
  abi.data = &( data );

  mixlink_callback_t * cb = NULL;
  if( dir == MIXLINK_DIRECTION_FROM_NIC )
    cb = &( handler->driver.tx );

  if( dir == MIXLINK_DIRECTION_TO_NIC )
    cb = &( handler->driver.rx );
    
  if( !cb )
    return -1;

  return mixlink_mod_exec( 
    (void *) &abi,
    cb
  );
}

/***************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************
 * Macros
**************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************/

#define X(name) \
  MIXLINK_GEN_DEF_MODULES_IMPL( controller, name, init  , mixlink_controller_t ) \
  MIXLINK_GEN_DEF_MODULES_IMPL( controller, name, deinit, mixlink_controller_t ) \
  MIXLINK_GEN_DEF_MODULES_IMPL( controller, name, loop  , mixlink_controller_t )
MIXLINK_CONTROLLER_MODULES
#undef X

#define X(name) \
  MIXLINK_GEN_IO_MODULES_IMPL( controller, name, io, mixlink_controller_t )
MIXLINK_CONTROLLER_MODULES
#undef X

#define MIXLINK_GEN_DEF_DRIVER( suffix )          \
  int8_t \
  mixlink_controller_driver_##suffix(             \
    const enum direction dir,                     \
    mixlink_controller_t * controller             \
  ){ \
    if( -1 == controller_valid( controller ) )    \
      return -1;                                  \
    struct serial_handler * handler =             \
      mixlink_controller_driver_pipeline_handler( \
        dir,                                      \
        controller                                \
      );                                          \
    if( !handler ) return -1;                     \
    mixlink_abi_def_serial_t abi;                 \
    abi.sr = &( handler->sr );                    \
    return mixlink_mod_exec(                      \
      (void *) &abi,                              \
      &( handler->driver.suffix )                 \
    );                                            \
  } 

#define X(suffix)  \
  MIXLINK_GEN_DEF_DRIVER(suffix)
MIXLINK_DEF_SUFFIXES
#undef X


/***************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************
 * End of file
 **************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************/
