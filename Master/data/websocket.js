var gateway = `ws://${window.location.hostname}:${window.location.port}/ws`;

var websocket;
function initWebSocket() {
    console.log('Trying to open a WebSocket Connection...');
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
    websocket.onerror = onError;
}

function onError(event) {
    console.log("Erreur" , event);
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
    console.log(event.data);
    try {
        var data =JSON.parse(event.data)
        var data2 = JSON.parse(event.data, function(key,value){
            console.log(key,value);
            if (key == 'confirmationReception') {
                console.log("confirmRecep " + value);
                x = document.getElementById('board-' +value)
                toggleLoading(x);
            }
        })
        console.log(data);
        // if (data.hasOwnProperty("confirmationReception")) {
        //     console.log('confreception' , data[confirmationReception]);
        // }
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
    
    } catch (e) {
        //console.log('parse impossible');
        
    }
    
}

window.addEventListener('load', onLoad);

  function onLoad(event) {
    initWebSocket();
    
}

function sendws(outgoing){
    console.log("outgoing: " + outgoing);
    websocket.send(outgoing);
}