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
    
    var data = JSON.parse(event.data)
    console.log(data);
    //document.getElementById("ModeTurbine").innerHTML = data.modeTurbine.name
    Array.from(document.getElementsByClassName("ModeTurbine")).forEach(element => {
        element.innerHTML = data.modeTurbine.name
    });
    Array.from(document.getElementsByClassName("selectModeTurbine")).forEach(element => {
        element.value = data.modeTurbine.id
    })
    Array.from(document.getElementsByClassName("OuvertureVanne")).forEach(element => {
        element.innerHTML = data.OuvertureVanne
    });
    
}

window.addEventListener('load', onLoad);

  function onLoad(event) {
    initWebSocket();
    
}

function sendws(outgoing){
    console.log("outgoing: " + outgoing);
    websocket.send(outgoing);
}