var gateway = `ws://${window.location.hostname}:${window.location.port}/ws`;

var websocket;
function initWebSocket() {
    console.log('Trying to open a WebSocket Connection...');
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
}

function onOpen(event) {
    console.log('Connection opened');
}

function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
}

function onMessage(event) {
    console.log('ws message:');
    console.log(event);
}

window.addEventListener('load', onLoad);

  function onLoad(event) {
    initWebSocket();
    
  }