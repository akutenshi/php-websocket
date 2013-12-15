<?php


class WS {

	function __construct() {
		websocket_startup("0.0.0.0", "8081", $this);
	}

	function onInit() {

	}
	function onTick() {
		echo "!";

	}

	function onConnect() {

	}

	function onDisconnect() {

	}

	function onError() {

	}

	function onData() {

	}
}

$ws = new WS();
?>
