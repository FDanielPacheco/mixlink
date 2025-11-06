/***************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************
 * Introduction
 **************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************/

/**********************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************//**
 * @file      mixlink.c
 * 
 * @version   1.0
 *
 * @date      01-10-2025
 *
 * @brief     Top level of a Linux-based L2 stack for protocol-aware serial-based wireless communications . 
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
 * Libraries
 **************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************/

#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include <argp.h>
#include <xcxml.h>
#include <unistd.h>

#include "mixlink.h"
#include "translator.h"
#include "controller.h"

/***************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************
 * argp program interfaces 
 **************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************/

const char  * argp_program_version     = "mixlink v1.0.0";
const char  * argp_program_bug_address = "fabio.d.pacheco@inesctec.pt";
static char   doc[ ]                   = "Linux-based L2 stack for protocol-aware serial-based wireless communications";
static char   args_doc[ ]              = "";

static struct argp_option options[ ] = {
  {"path", 'p', "XML FILE" , 0, "The path to the XML file indicating the stack components" , 0 },
  { 0 }
};
  
struct argp_arguments{
  char * path;
};

static error_t parse_opt( int key, char * arg, struct argp_state * state );

static struct argp argp = { 
  .options = options, 
  .parser = parse_opt, 
  .args_doc = args_doc, 
  .doc = doc, 
  .children = 0,
  .help_filter = 0,
  .argp_domain = 0
};

static error_t
parse_opt( 
  int key, 
  char * arg, 
  struct argp_state * state
){

  struct argp_arguments * arguments = state->input;

  switch( key ){
    case 'p':
      arguments->path = arg;
      break;

    // Number of extra arguments not specified by an option key
    case ARGP_KEY_END:
      if( 0 != state->arg_num || !arguments->path )
        argp_usage( state );
      break;

    default:
      return ARGP_ERR_UNKNOWN;
  }
  
  return 0;
}

/***************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************
 * XML file definition 
 **************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************/

typedef struct{
  mixlink_param_controller_t controller;
  mixlink_param_translator_t translator;
} mixlink_args_t;

static const xml_field_t xml_fields[ ] = {
  XML_FIELD( "/instance/controller/tx/name"       , mixlink_args_t, controller.dev.pair.tx.name ),
  XML_FIELD( "/instance/controller/tx/device"     , mixlink_args_t, controller.dev.pair.tx.device ),
  XML_FIELD( "/instance/controller/tx/driver"     , mixlink_args_t, controller.dev.pair.tx.driver ),

  XML_FIELD( "/instance/controller/rx/name"       , mixlink_args_t, controller.dev.pair.rx.name ),
  XML_FIELD( "/instance/controller/rx/device"     , mixlink_args_t, controller.dev.pair.rx.device ),
  XML_FIELD( "/instance/controller/rx/driver"     , mixlink_args_t, controller.dev.pair.rx.driver ),
  
  XML_FIELD( "/instance/controller/default/name"  , mixlink_args_t, controller.dev.def.name ),
  XML_FIELD( "/instance/controller/default/device", mixlink_args_t, controller.dev.def.device ),
  XML_FIELD( "/instance/controller/default/driver", mixlink_args_t, controller.dev.def.driver ),

  XML_FIELD( "/instance/controller/qos"           , mixlink_args_t, controller.qos ),
  XML_FIELD( "/instance/controller/framer"        , mixlink_args_t, controller.framer ),
  XML_FIELD( "/instance/controller/segm"          , mixlink_args_t, controller.segm ),

  XML_FIELD( "/instance/translator/tx/name"       , mixlink_args_t, translator.nic.pair.tx.name ),
  XML_FIELD( "/instance/translator/tx/device"     , mixlink_args_t, translator.nic.pair.tx.name ),
  XML_FIELD( "/instance/translator/tx/driver"     , mixlink_args_t, translator.nic.pair.tx.name ),

  XML_FIELD( "/instance/translator/rx/name"       , mixlink_args_t, translator.nic.pair.rx.name ),
  XML_FIELD( "/instance/translator/rx/device"     , mixlink_args_t, translator.nic.pair.rx.name ),
  XML_FIELD( "/instance/translator/rx/driver"     , mixlink_args_t, translator.nic.pair.rx.name ),

  XML_FIELD( "/instance/translator/default/name"  , mixlink_args_t, translator.nic.def.name ),
  XML_FIELD( "/instance/translator/default/device", mixlink_args_t, translator.nic.def.name ),
  XML_FIELD( "/instance/translator/default/driver", mixlink_args_t, translator.nic.def.name ),

  XML_FIELD( "/instance/translator/opt"           , mixlink_args_t, translator.opt ),
  XML_FIELD( "/instance/translator/framer"        , mixlink_args_t, translator.framer ),
};

/***************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************
 * Functions helpers to simplify the main function 
 **************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************/

int8_t args_parse(
  int argc,
  char ** argv,
  struct argp_arguments * args
);

int8_t load_xml( 
  struct argp_arguments * args,
  mixlink_args_t * xml_args
);
 
int8_t 
args_parse(
  int argc,
  char ** argv,
  struct argp_arguments * args
){
  return (int8_t) argp_parse( 
    &argp,
    argc,
    argv,
    0,
    0,
    args
  );
}

int8_t
load_xml( 
  struct argp_arguments * args,
  mixlink_args_t * xml_args
){
  return xml_retrive_data(
    args->path,
    xml_fields,
    XML_ARR_SIZE(xml_fields), 
    (void *) xml_args,
    sizeof(mixlink_args_t)
  );
}

/***************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************
 * Stack Code 
 **************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************/

enum step_type {
  STEP_MOD_CORE,
  STEP_MOD_DRIVER
};

typedef struct {
  const char * name;
  enum step_type type;

  union {
    struct { 
      int8_t (*fn)(void *); 
      void *obj; 
    } mod_core;

    struct { 
      int8_t (*fn)( const enum direction, mixlink_controller_t * ); 
      enum direction dir; 
      mixlink_controller_t * obj; 
    } mod_driver;
    
  } call;
} stack_step_t;

#define MIXLINK_CORE_STEP( phase, name, obj )                  \
  { "mixlink_" #name "_" #phase, STEP_MOD_CORE,                \
    .call.mod_core = {                                         \
      (int8_t (*)(void *)) mixlink_##name##_##phase,           \
      obj                                                      \
    }                                                          \
  }

#define MIXLINK_DRIVER_STEP( phase, dir, obj )                 \
  { "mixlink_controller_driver_" #phase, STEP_MOD_DRIVER,      \
    .call.mod_driver = {                                       \
      mixlink_controller_driver_##phase,                       \
      dir,                                                     \
      obj                                                      \
    }                                                          \
  }                                                       

int8_t
run_stack_steps( 
  const char * phase, 
  stack_step_t * steps, 
  size_t nsteps
){
  for( size_t i = 0 ; i < nsteps ; ++i ){
    int8_t ret = 1;

    while( 0 != ret ){
      switch( steps[i].type ){
        case STEP_MOD_CORE:
          ret = steps[i].call.mod_core.fn(
            steps[i].call.mod_core.obj
          );
          break;

        case STEP_MOD_DRIVER:
          ret = steps[i].call.mod_driver.fn(
            steps[i].call.mod_driver.dir,
            steps[i].call.mod_driver.obj
          );
          break;
      }

      if( ret )
        usleep( 1e6 );  // retry on temporary failure

      if( -1 == ret ){
        if (errno == EINVAL)
          error_print("[%s] failed on step: %s", phase, steps[i].name );
        return -1;
      }
    }

  }
  return 0;
}


int8_t 
init_stack( 
  mixlink_translator_t * translator,
  mixlink_controller_t * controller
){
  const size_t maxsteps = 8;
  stack_step_t steps[ maxsteps ];  
  size_t nsteps = 0;

  steps[ nsteps ++ ] = (stack_step_t) MIXLINK_CORE_STEP( init, translator_opt,    translator );
  steps[ nsteps ++ ] = (stack_step_t) MIXLINK_CORE_STEP( init, translator_framer, translator );
  steps[ nsteps ++ ] = (stack_step_t) MIXLINK_CORE_STEP( init, controller_segm,   controller );
  steps[ nsteps ++ ] = (stack_step_t) MIXLINK_CORE_STEP( init, controller_framer, controller );
  steps[ nsteps ++ ] = (stack_step_t) MIXLINK_CORE_STEP( init, controller_qos,    controller );

  if( !controller->def.enabled ){
    steps[ nsteps ++ ] = (stack_step_t) MIXLINK_DRIVER_STEP( init, MIXLINK_DIRECTION_FROM_NIC, controller );
    steps[ nsteps ++ ] = (stack_step_t) MIXLINK_DRIVER_STEP( init, MIXLINK_DIRECTION_TO_NIC, controller );
  }
  else
    steps[ nsteps ++ ] = (stack_step_t) MIXLINK_DRIVER_STEP( init, MIXLINK_DIRECTION_FROM_NIC, controller );

  return run_stack_steps( 
    "init" , 
    steps,
    nsteps
  );
}

int8_t 
deinit_stack( 
  mixlink_translator_t * translator,
  mixlink_controller_t * controller
){
  const size_t maxsteps = 8;
  stack_step_t steps[ maxsteps ];  
  size_t nsteps = 0;

  steps[ nsteps ++ ] = (stack_step_t) MIXLINK_CORE_STEP( deinit, translator_opt,    translator );
  steps[ nsteps ++ ] = (stack_step_t) MIXLINK_CORE_STEP( deinit, translator_framer, translator );
  steps[ nsteps ++ ] = (stack_step_t) MIXLINK_CORE_STEP( deinit, controller_segm,   controller );
  steps[ nsteps ++ ] = (stack_step_t) MIXLINK_CORE_STEP( deinit, controller_framer, controller );
  steps[ nsteps ++ ] = (stack_step_t) MIXLINK_CORE_STEP( deinit, controller_qos,    controller );

  if( !controller->def.enabled ){
    steps[ nsteps ++ ] = (stack_step_t) MIXLINK_DRIVER_STEP( deinit, MIXLINK_DIRECTION_FROM_NIC, controller );
    steps[ nsteps ++ ] = (stack_step_t) MIXLINK_DRIVER_STEP( deinit, MIXLINK_DIRECTION_TO_NIC, controller );
  }
  else
    steps[ nsteps ++ ] = (stack_step_t) MIXLINK_DRIVER_STEP( deinit, MIXLINK_DIRECTION_FROM_NIC, controller );  

  return run_stack_steps( 
    "deinit" , 
    steps,
    nsteps
  );
}

int8_t 
loop_stack( 
  mixlink_translator_t * translator,
  mixlink_controller_t * controller
){
  stack_step_t steps[ ] = {
    MIXLINK_CORE_STEP( loop, translator_opt,    translator ),
    MIXLINK_CORE_STEP( loop, translator_framer, translator ),
    MIXLINK_CORE_STEP( loop, controller_segm,   controller ),
    MIXLINK_CORE_STEP( loop, controller_framer, controller ),
    MIXLINK_CORE_STEP( loop, controller_qos,    controller ),
    MIXLINK_DRIVER_STEP( loop, MIXLINK_DIRECTION_FROM_NIC, controller ),
    MIXLINK_DRIVER_STEP( loop, MIXLINK_DIRECTION_TO_NIC, controller )
  };
  size_t nsteps = sizeof( steps ) / sizeof( steps[0] );

  return run_stack_steps( 
    "loop" , 
    steps,
    nsteps
  );
}

/***************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************
 * Entry point
 **************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************/
int
main( 
  int argc, 
  char ** argv
){
  
	struct argp_arguments arguments  = { 0 }; 
  mixlink_args_t        xml_args   = { 0 };
  mixlink_translator_t  translator = { 0 };  
  mixlink_controller_t  controller = { 0 };

  const error_t arg_ret = args_parse( 
    argc, 
    argv, 
    &arguments
  );
  if( 0 != arg_ret ){
    error_print( "args_parse" );
    return EXIT_FAILURE;
  }

  
  const int8_t xml_ret = load_xml( 
    &arguments,
    &xml_args
  );
  if( 0 != xml_ret ){
    error_print( "load_xml" );
    return EXIT_FAILURE;
  }


  const int8_t trs_init_ret = mixlink_translator_init( 
    xml_args.translator, 
    &translator
  );
  if( -1 == trs_init_ret ){
    error_print("mixlink_translator_init");
    goto cleanup;
  }

  const int8_t ctrl_init_ret = mixlink_controller_init( 
    xml_args.controller, 
    &controller
  );
  if( -1 == ctrl_init_ret ){
    error_print("mixlink_controller_init");
    goto cleanup;
  }

  const int8_t stack_init_ret = init_stack( 
    &translator,
    &controller
  );
  if( 0 != stack_init_ret ){
    error_print("init_stack");
    goto cleanup;
  }

  // Run the RX pipeline


  // Run the TX pipeline

  // Run the Loop pipeline
  const int8_t stack_loop_ret = loop_stack( 
    &translator,
    &controller
  );
  if( 0 != stack_loop_ret ){
    error_print("loop_stack");
    goto cleanup;
  }

  sleep( 1 );

  (void) deinit_stack( 
    &translator,
    &controller
  );

  cleanup:
    mixlink_controller_close( &controller );
    mixlink_translator_close( &translator );
    return EXIT_SUCCESS;
}


/*

    val = mixlink_translator_init(
      xml_args.translator,
      &translator
    );
    if( 0 != val ){
      if( EINVAL == errno )
        error_print( "mixlink_translator_init" );
      goto cleanup;
    }

    val = mixlink_controller_init( 
      xml_args.controller,
      &controller
    );
    if( 0 != val ){
      if( EINVAL == errno )
        error_print( "mixlink_controller_init" );
      goto cleanup;
    }

    val = mixlink_translator_opt_init( &translator );
    if( 0 != val ){
      error_print( "mixlink_translator_opt_init" );
      goto cleanup;
    }

    val = mixlink_translator_framer_init( &translator );
    if( 0 != val ){
      error_print( "mixlink_translator_framer_init" );
      goto cleanup;
    }

    val = mixlink_controller_segm_init( &controller );
    if( 0 != val ){
      error_print( "mixlink_controller_segm_init" );
      goto cleanup;
    }

    val = mixlink_controller_framer_init( &controller );
    if( 0 != val ){
      error_print( "mixlink_controller_framer_init" );
      goto cleanup;
    }

    val = mixlink_controller_qos_init( &controller );
    if( 0 != val ){
      error_print( "mixlink_controller_qos_init" );
      goto cleanup;
    }

    val = mixlink_controller_driver_init( &controller );
    if( 0 != val ){
      error_print( "mixlink_controller_driver_init" );
      goto cleanup;
    }

..................................

        uint16_t _indexbuf[ BUFSIZ ];
    // The first segment is indicated to start at position 0
    _indexbuf[0] = 0;
    mixlink_buf16_t indexbuf = {
      .val = &_indexbuf,
      .len = 1,
      .size = BUFSIZ
    };

    // RX Pipeline from serial port to NIC
    uint8_t _rxbuf[ BUFSIZ ];
    mixlink_buf8_t rxbuf = {
      .val = &_rxbuf,
      .len = 0,
      .size = BUFSIZ
    };

    int8_t in1, in2, in3, in4;
    for( ; ; ){
      rxbuf.len = 0;
      in4 = 0;

      do { 
        in3 = 0;
        do { 
          in2 = 0;
          do {
            in1 = 0;
            do { 
              size_t len = mixlink_controller_read( 
                &rxbuf,
                rxbuf.len,
                rxbuf.size,
                &controller
              );
              if( !len ){
                error_print( "mixlink_controller_read" );
                goto cleanup;
              }

              in1 = mixlink_controller_driver_io(
                &rxbuf,
                MIXLINK_DIRECTION_TO_NIC,
                &controller
              );
              if( -1 == in1 ){
                error_print( "mixlink_controller_driver_io" );
                goto cleanup;
              }

            } while( 0 != in1 );

            val = mixlink_controller_framer_io( 
              &rxbuf,
              MIXLINK_DIRECTION_TO_NIC,
              &controller
            );
            if( 0 != val ){
              error_print( "mixlink_controller_framer_io" );
              goto cleanup;
            }

          } while( 0 != in2 );

            val = mixlink_controller_qos_io( 
              &rxbuf,
              MIXLINK_DIRECTION_TO_NIC,
              &controller
            );
            if( 0 != val ){
              error_print( "mixlink_controller_qos_io" );
              goto cleanup;
            }

          } while( 0 != in3 );

          val = mixlink_controller_segm_io( 
            &rxbuf,
            &indexbuf,
            MIXLINK_DIRECTION_TO_NIC,
            &controller
          );

        } while( 0 != in4 );
      
    }
*/
