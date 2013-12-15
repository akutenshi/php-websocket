/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_websocket.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <sys/event.h>
#include <sys/time.h>


/* If you declare any globals in php_websocket.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(websocket)
*/

/* True global resources - no need for thread safety here */
//static int le_websocket;

/* {{{ websocket_functions[]
 *
 * Every user visible function must have an entry in websocket_functions[].
 */
const zend_function_entry websocket_functions[] = {
	PHP_FE(websocket_startup,	NULL)
	PHP_FE_END	/* Must be the last line in websocket_functions[] */
};
/* }}} */

/* {{{ websocket_module_entry
 */
zend_module_entry websocket_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	"websocket",
	websocket_functions,
	PHP_MINIT(websocket),
	PHP_MSHUTDOWN(websocket),
	PHP_RINIT(websocket),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(websocket),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(websocket),
#if ZEND_MODULE_API_NO >= 20010901
	PHP_WEBSOCKET_VERSION,
#endif
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_WEBSOCKET
ZEND_GET_MODULE(websocket)
#endif

/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("websocket.global_value",      "42", PHP_INI_ALL, OnUpdateLong, global_value, zend_websocket_globals, websocket_globals)
    STD_PHP_INI_ENTRY("websocket.global_string", "foobar", PHP_INI_ALL, OnUpdateString, global_string, zend_websocket_globals, websocket_globals)
PHP_INI_END()
*/
/* }}} */

/* {{{ php_websocket_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_websocket_init_globals(zend_websocket_globals *websocket_globals)
{
	websocket_globals->global_value = 0;
	websocket_globals->global_string = NULL;
}
*/
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(websocket)
{
	/* If you have INI entries, uncomment these lines 
	REGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(websocket)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(websocket)
{
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(websocket)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(websocket)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "websocket support", "enabled");
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/
}
/* }}} */

#define WSFLAGS_SERVERSOCKET 1

struct ws_user {
	int socket;
	int flags;
};


/* Remove the following function when you have successfully modified config.m4
   so that your module can be compiled into PHP, it exists only for testing
   purposes. */

/* Every user-visible function in PHP should document itself in the source */
/* {{{ proto string confirm_websocket_compiled(string arg)
   Return a string to confirm that the module is compiled in */
PHP_FUNCTION(websocket_startup)
{
	char *bindip = NULL;
	int bindip_len, port, res;
	zval *obj, *ret, funccall;

	int serversocket, kq;

	struct sockaddr_in bindaddr;
	struct ws_user me;
	struct kevent ev[32];
	struct timespec ts;
	ts.tv_sec = 0;
	ts.tv_nsec = 100000;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "slo", &bindip, &bindip_len, &port, &obj) == FAILURE) {
		return;
	}
	
	serversocket = socket(AF_INET, SOCK_STREAM, 0);
	if(!serversocket) {
		php_error(E_ERROR, "websocket_startup(): socket() failed - error %i", errno);
		RETURN_FALSE;
	}
	bindaddr.sin_family = AF_INET;
	bindaddr.sin_port   = htons(port);
	bindaddr.sin_addr.s_addr   = inet_addr(bindip);
	if(bind(serversocket, (struct sockaddr*)&bindaddr, sizeof(bindaddr)) != 0) {
		php_error(E_ERROR, "websocket_startup(): bind() failed - error %i", errno);
		RETURN_FALSE;
	}
	if(listen(serversocket, -1) != 0) {
		php_error(E_ERROR, "websocket_startup(): listen() failed - error %i", errno);
		RETURN_FALSE;
	}
	kq = kqueue();
	if(!kq) {
		php_error(E_ERROR, "websocket_startup(): kqueue() failed - error %i", errno);
		RETURN_FALSE;
	}

	me.socket = serversocket;
	me.flags = WSFLAGS_SERVERSOCKET;

	EV_SET(&ev[0], me.socket, EVFILT_READ, EV_ADD | EV_CLEAR, 0, 0, (void*)&me);

	if(kevent(kq, &ev[0], 1, 0, 0, NULL) < 0) {
		php_error(E_ERROR, "websocket_startup(): kqueue add failed - error %i", errno);
		RETURN_FALSE;
	}

	for(;;) {
		res = kevent(kq, 0, 0, ev, 32, &ts);
		if(res < 0)  {
			RETURN_TRUE;
		} else if(res == 0) {
			Z_TYPE(funccall)   = IS_STRING;
			Z_STRVAL(funccall) = "ontick";
			Z_STRLEN(funccall) = 6;

			call_user_function_ex(NULL, &obj, &funccall, &ret, 0, NULL, 0, NULL TSRMLS_CC);
			zval_ptr_dtor(&ret);
		} else {
			RETURN_TRUE;
		}

	}
	RETURN_TRUE;
}
/* }}} */
/* The previous line is meant for vim and emacs, so it can correctly fold and 
   unfold functions in source code. See the corresponding marks just before 
   function definition, where the functions purpose is also documented. Please 
   follow this convention for the convenience of others editing your code.
*/


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
