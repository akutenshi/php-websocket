dnl $Id$

PHP_ARG_ENABLE(websocket, whether to enable websocket support,
	[  --enable-websocket           Enable websocket support])

if test "$PHP_WEBSOCKET" != "no"; then
  PHP_NEW_EXTENSION(websocket, websocket.c, $ext_shared)
fi
