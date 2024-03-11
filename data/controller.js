
// Setup Elements for Dynamic Updates
// ================================================================================
let divError = document.getElementById("divError");
let divConnected = document.getElementById("polar");
 
// Connect to Server - wsConnection variable comes from the wsConnection.js script 
// which is dynamically generated by the Arduino at startup
// ================================================================================
const webSocket = new WebSocket('ws://'+location.hostname+':81', ['arduino']);

// Handle basic Websocket States
// ================================================================================
webSocket.onopen = function (event) {
    divError.style.display = "none";
    divConnected.style.display = "block";
    console.log("WebSocket Connected!");
};

webSocket.onclose = function (event) {
    divError.style.display = "flex";
    divConnected.style.display = "none";
    console.log("WebSocket Disconnected!")
    location.reload();
}

webSocket.onerror = function (error) {
    divError.style.display = "flex";
    divConnected.style.display = "none";
    console.log('WebSocket Error ', error);
    location.reload();
}


// Handle Incoming Websocket messages
// ================================================================================
webSocket.onmessage = function (event) {
    let msgType = event.data.split('_|_')[0];
    let msgData = event.data.split('_|_')[1];
    switch (msgType) {
        case 'Connected':
            divError.style.display = "none";
            divConnected.style.display = "block";
            let motorSpeed = localStorage.getItem("motorSpeed");
            if(!motorSpeed){
                motorSpeed = 10; 
            }
            console.log(motorSpeed);
            switch(motorSpeed) {
                case 60:
                    set1arc();
                    break;
                case 10:
                    set10arc();
                    break;
                case 1800:
                    set30arc();
                    break;
                default:
                    break;
            }
            
            break;
        case 'motorChange':
            motorChange(msgData);
            break;
        default:
            try {
                document.getElementById(`${msgType}`).value = msgData;
            } catch (error) {
                console.log('Nu am inteles mesajul: ' + event.data);
            }
    };
};


function toggleMotors() {
    let status = localStorage.getItem("motorStatus");
    if(status) {
        webSocketSend('toggleMotors', 0, 0);
        localStorage.setItem("motorStatus", 0);
    } else {
        webSocketSend('toggleMotors', 1, 0);
        localStorage.setItem("motorStatus", 1);
    }    
}


function set1arc(){
    localStorage.setItem("motorSpeed", 60);
    resetButtons();
    document.getElementById('1arc').classList.remove("inactive");    
}

function set10arc(){
    localStorage.setItem("motorSpeed", 10);
    resetButtons();
    document.getElementById('10arc').classList.remove("inactive");    
}

function set30arc(){
    localStorage.setItem("motorSpeed", 1800);
    resetButtons();
    document.getElementById('30arc').classList.remove("inactive");    
}

function resetButtons() {
    document.getElementById('1arc').classList.remove("inactive");
    document.getElementById('10arc').classList.remove("inactive");
    document.getElementById('30arc').classList.remove("inactive");
    document.getElementById('1arc').classList.add("inactive");
    document.getElementById('10arc').classList.add("inactive");
    document.getElementById('30arc').classList.add("inactive");
}

function goALTup() {
    let motorSpeed = localStorage.getItem("motorSpeed");
    let motorStatus = localStorage.getItem("motorStatus");
    if(motorSpeed && motorStatus)
        webSocketSend('goALT', motorSpeed, 1);       

}

function goALTdown() {
    let motorSpeed = localStorage.getItem("motorSpeed");
    let motorStatus = localStorage.getItem("motorStatus");
    if(motorSpeed && motorStatus)
        webSocketSend('goALT', motorSpeed, 0);
}


function goAZleft() {
    let motorSpeed = localStorage.getItem("motorSpeed");
    let motorStatus = localStorage.getItem("motorStatus");
    if(motorSpeed && motorStatus)
        webSocketSend('goAZ', motorSpeed, 1);   
}

function goAZright() {
    let motorSpeed = localStorage.getItem("motorSpeed");
    let motorStatus = localStorage.getItem("motorStatus");
    if(motorSpeed && motorStatus)
        webSocketSend('goAZ', motorSpeed, 0);
}


function motorChange(value) {
    if (value == '0') {      
        document.getElementById('statusLed').classList.remove("led-red");  
        document.getElementById('statusLed').classList.add("led-red");

        document.getElementById('btt_up').classList.add("blur");
        document.getElementById('btt_down').classList.add("blur");
        document.getElementById('btt_left').classList.add("blur");
        document.getElementById('btt_right').classList.add("blur");
        

    } else {        
        document.getElementById('statusLed').classList.remove("led-red");

        document.getElementById('btt_up').classList.remove("blur");
        document.getElementById('btt_down').classList.remove("blur");
        document.getElementById('btt_left').classList.remove("blur");
        document.getElementById('btt_right').classList.remove("blur");
    }
}


// Format and actually send values 
// ================================================================================
function webSocketSend(msgType, msgData, msgDirrection) {
    let messageToSend = msgType + "_|_" + msgData + "_|_" + msgDirrection;
    console.log("Trimit mesajul: " + messageToSend)
    webSocket.send(messageToSend);
};


// Run when page is shown
// ================================================================================
window.addEventListener("pageshow", function () {
    if (webSocket.readyState >= 2) {
        location.reload();
    }
});