// Class reaction work with on click user actions.
function Reaction() {  }
	//function clickFileCheck(clickedElem)
Reaction.clickFileCheck = function (clickedElem)
{
	// Now geting domain and file like this
	var fileName = clickedElem.value;
	var savedVars = $('someInformation').value;
	var separated = savedVars.split(' ');
	if (!clickedElem.checked) {
		//SocketWorker.stopReadSend(separated[0], fileName)) // Sending file names to server like that.
		$('setupAll').checked = false;
	}
	else {
		//alert(separated[0] + ' on adress ' + separated[1] + ' and ' + clickedElem.value);
		if (!workSocket) {
			alert('Socket initialyze now');
			SocketWorker("ws://" + separated[1] + ":10030/"); // Try init socket. // change later
		}
		else {
			SocketWorker.startReadSend(separated[0], fileName) // Sending file names to server like that.
		}
	}
}

Reaction.clickAllFiles = function(clickedElem)
{
	// found our label 
	//var elemId = clickedElem.id;
	//var allLabels = $$('label[for=' + elemId + ']')[0]; // sirios sheet need to remember it
	if (!clickedElem.checked) // if we uncheck box
		return;

	var domainName = $('someInformation');
	var elemValue = clickedElem.value;
	for (var i = 1; i <= elemValue; i++) {
		var logName = $('log' + i);
		if(!logName.checked) {
			logName.checked = true;
			SocketWorker.startReadSend(logName.value) // Sending file names to server like that.
		}
	}/**/
}

// Class entry worker add entryes to list and add some css styles to entryes
function EntryWorker() {  }

EntryWorker.addEntry = function(entry)
{
	var list = $('logList');
	var newOption = document.createElement('option');
	newOption.innerHTML = entry;
	// choosing style of entry
	var rand = Math.random() * 10 + 1;
	if (rand < 3)
		newOption.style.background = 'linear-gradient(to right, red, #ffffff';
	else if (rand < 6)
		newOption.style.background = 'linear-gradient(to right, yellow, #ffffff)';
	else 
		newOption.style.background = 'linear-gradient(to right, green, #ffffff)';
	
	//newOption.className = 'good';
	list.appendChild(newOption);
}

// Class sender work with web socket connections
var workSocket;
function SocketWorker(addr) // Initialyze socket
{ 
	if ("WebSocket" in window) {
		alert("WebSocket is supported by your Browser!");
     	// Start connection
	    workSocket = new WebSocket(addr);
	    workSocket.onopen = function()
	    {
	        // Web Socket is connected, send data using send()
	        alert("Socket is opened");
	        //alert("Start send this: " + host + filename);
	        //workSocket.send('/var/www/vhosts/' + host + '/logs/' + filename);
	        //alert("Message is sent...");
	    };
	    workSocket.onmessage = function (evt) 
	    { 
	    	
	        var received_msg = evt.data;
	        //alert("Message is received: " + received_msg);
	        EntryWorker.addEntry(received_msg);        
	    };
	    workSocket.onclose = function()
	    { 
	        // websocket is closed.
	        alert("Connection is closed..."); 
	    };
	}
	else {
		alert("Web socket is not supported!");
	}
}
SocketWorker.startReadSend = function(host, filename) // function send server file name and start reading logs
{
	//workSocket.send('start'); //Send somthing meens that we need to start reading;
	workSocket.send('/var/www/vhosts/' + host + '/logs/' + filename); // some like this
}
SocketWorker.stopReadSend = function(host, filename) // function send server file name and start reading logs
{
	//workSocket.send('stop'); //Send somthing meens that we need to start reading;
	workSocket.send('/var/www/vhosts/' + host + '/logs/' + filename); // some like this
}
