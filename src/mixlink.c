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
 * Functions
 **************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************/

/***************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************
 * Entry point
 **************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************/
int
main( 
  int argc, 
  char ** argv
){
  
	struct argp_arguments arguments = { 0 }; 
  const error_t arg_ret = argp_parse( 
    &argp,
    argc,
    argv,
    0,
    0,
    &arguments
  );
  if( 0 != arg_ret )
    return EXIT_FAILURE;

  
  mixlink_args_t xml_args = { 0 };
  const int8_t xml_ret = xml_retrive_data(
    arguments.path,
    xml_fields,
    XML_ARR_SIZE(xml_fields), 
    (void *) &xml_args,
    sizeof(xml_args)
  );
  if( 0 != xml_ret )
    return EXIT_FAILURE;
  
  /*
  uint8_t data[ ] = { 0x0A, 0x0B, 0x0C, 0x0D }; 
  mixlink_buf8_t buf;
  buf.val = data;
  buf.size = sizeof(data);
  buf.len = sizeof(data);

  mixlink_buf16_t index;

  mixlink_translator_framer_io( &buf, MIXLINK_DIRECTION_FROM_NIC, &translator );
  mixlink_translator_opt_io( &buf, MIXLINK_DIRECTION_FROM_NIC, &translator );

  mixlink_controller_qos_io( &buf, MIXLINK_DIRECTION_FROM_NIC, &controller );
  mixlink_controller_framer_io( &buf, MIXLINK_DIRECTION_FROM_NIC, &controller );
  mixlink_controller_segm_io( &buf, &index, MIXLINK_DIRECTION_FROM_NIC, &controller );
  mixlink_controller_driver_io( &buf, MIXLINK_DIRECTION_FROM_NIC, &controller );
  mixlink_controller_driver_io( &buf, MIXLINK_DIRECTION_TO_NIC, &controller );
  */

  mixlink_translator_t translator = { 0 };  
  mixlink_controller_t controller = { 0 };
  int8_t err = 0;

  for( ; ; ){    

    err = mixlink_translator_init(
      xml_args.translator,
      &translator
    );
    if( 0 != err ){
      if( EINVAL == errno )
        error_print( "mixlink_translator_init" );
      goto cleanup;
    }

    err = mixlink_controller_init( 
      xml_args.controller,
      &controller
    );
    if( 0 != err ){
      if( EINVAL == errno )
        error_print( "mixlink_controller_init" );
      goto cleanup;
    }

    err = mixlink_translator_opt_init( &translator );
    if( 0 != err ){
      error_print( "mixlink_translator_opt_init" );
      goto cleanup;
    }

    err = mixlink_translator_framer_init( &translator );
    if( 0 != err ){
      error_print( "mixlink_translator_framer_init" );
      goto cleanup;
    }

    err = mixlink_controller_segm_init( &controller );
    if( 0 != err ){
      error_print( "mixlink_controller_segm_init" );
      goto cleanup;
    }

    err = mixlink_controller_framer_init( &controller );
    if( 0 != err ){
      error_print( "mixlink_controller_framer_init" );
      goto cleanup;
    }

    err = mixlink_controller_qos_init( &controller );
    if( 0 != err ){
      error_print( "mixlink_controller_qos_init" );
      goto cleanup;
    }

    err = mixlink_controller_driver_init( &controller );
    if( 0 != err ){
      error_print( "mixlink_controller_driver_init" );
      goto cleanup;
    }

    // RX Pipeline from serial port to NIC
    uint8_t _rxbuf[ BUFSIZ ];
    mixlink_buf8_t rxbuf = {
      .val = &_rxbuf,
      .len = 0,
      .size = BUFSIZ
    };

    for( ; ; ){
      size_t len = mixlink_controller_read( 
        &rxbuf,
        0,
        rxbuf.size,
        &controller
      );
      if( !len ){
        error_print( "mixlink_controller_read" );
        goto cleanup;
      }

      err = mixlink_controller_driver_io(
        &rxbuf,
        MIXLINK_DIRECTION_TO_NIC,
        &controller
      );
      if( 0 != err ){
        error_print( "mixlink_controller_driver_io" );
        goto cleanup;
      }

      err = mixlink_controller_framer_io( 
        &rxbuf,
        MIXLINK_DIRECTION_TO_NIC,
        &controller
      );
      if( 0 != err ){
        error_print( "mixlink_controller_framer_io" );
        goto cleanup;
      }

      err = mixlink_controller_qos_io( 
        &rxbuf,
        MIXLINK_DIRECTION_TO_NIC,
        &controller
      );
      if( 0 != err ){
        error_print( "mixlink_controller_qos_io" );
        goto cleanup;
      }

      
    }
  }

  cleanup:
    mixlink_controller_close( &controller );
    mixlink_translator_close( &translator );
    return EXIT_SUCCESS;
}
